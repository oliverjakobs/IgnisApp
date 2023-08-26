#include "model.h"

int loadAnimationChannelGLTF(AnimationChannel* channel, cgltf_animation_sampler* sampler)
{
    channel->frame_count = sampler->input->count;

    // load input
    channel->times = malloc(channel->frame_count * sizeof(float));
    cgltf_accessor_unpack_floats(sampler->input, channel->times, channel->frame_count);

    // load output
    size_t comps = cgltf_num_components(sampler->output->component_type);
    channel->transforms = malloc(channel->frame_count * comps * sizeof(float));
    cgltf_accessor_unpack_floats(sampler->output, channel->transforms, channel->frame_count * comps);

    return IGNIS_SUCCESS;
}

int loadAnimationChannelBindPose(AnimationChannel* channel, uint8_t comps, float* bind)
{
    channel->transforms = malloc(comps * sizeof(float));
    if (!channel->transforms) return IGNIS_FAILURE;

    for (uint8_t c = 0; c < comps; ++c)
        channel->transforms[c] = bind[c];

    channel->frame_count = 1;
    return IGNIS_SUCCESS;
}

void destroyAnimationChannel(AnimationChannel* channel)
{
    if (channel->times)      free(channel->times);
    if (channel->transforms) free(channel->transforms);
}

size_t getNodeIndex(const cgltf_node* target, const cgltf_data* data)
{
    if (data->skins_count)  return getJointIndex(target, &data->skins[0], data->skins[0].joints_count);
    if (target->mesh)       return getMeshIndex(target->mesh, data->meshes, data->meshes_count);
    return data->meshes_count;
}

int  loadAnimationGLTF(Animation* animation, cgltf_animation* gltf_animation, const cgltf_data* data)
{
    animation->channel_count = data->skins_count ? data->skins[0].joints_count : data->meshes_count;

    animation->time = 0.0f;
    animation->duration = 0.0f;

    animation->translations = calloc(animation->channel_count, sizeof(AnimationChannel));
    animation->rotations    = calloc(animation->channel_count, sizeof(AnimationChannel));
    animation->scales       = calloc(animation->channel_count, sizeof(AnimationChannel));
    if (!animation->translations || !animation->rotations || !animation->scales) return IGNIS_FAILURE;

    for (size_t i = 0; i < gltf_animation->channels_count; ++i)
    {
        size_t index = getNodeIndex(gltf_animation->channels[i].target_node, data);
        if (index >= animation->channel_count) // Animation channel for a node not in the armature
            continue;

        cgltf_animation_channel* channel = &gltf_animation->channels[i];
        switch (channel->target_path)
        {
        case cgltf_animation_path_type_translation:
            loadAnimationChannelGLTF(&animation->translations[index], channel->sampler);
            break;
        case cgltf_animation_path_type_rotation:
            loadAnimationChannelGLTF(&animation->rotations[index], channel->sampler);
            break;
        case cgltf_animation_path_type_scale:
            loadAnimationChannelGLTF(&animation->scales[index], channel->sampler);
            break;
        default:
            IGNIS_WARN("MODEL: Unsupported target_path on channel %d's sampler. Skipping.", i);
            break;
        }

        // update animation duration
        animation->duration = max(animation->duration, channel->sampler->input->max[0]);
    }


    if (!data->skins_count) return IGNIS_SUCCESS;

    // fill missing channels
    for (size_t i = 0; i < animation->channel_count; ++i)
    {
        cgltf_node* node = data->skins[0].joints[i];

        // load translation
        if (animation->translations[i].frame_count == 0 && node->has_translation)
            loadAnimationChannelBindPose(&animation->translations[i], 3, node->translation);

        // load rotation
        if (animation->rotations[i].frame_count == 0 && node->has_rotation)
            loadAnimationChannelBindPose(&animation->rotations[i], 4, node->rotation);

        // load translation
        if (animation->scales[i].frame_count == 0 && node->has_scale)
            loadAnimationChannelBindPose(&animation->scales[i], 3, node->scale);
    }


    return IGNIS_SUCCESS;
}

void destroyAnimation(Animation* animation)
{
    for (size_t i = 0; i < animation->channel_count; ++i)
    {
        destroyAnimationChannel(&animation->translations[i]);
        destroyAnimationChannel(&animation->rotations[i]);
        destroyAnimationChannel(&animation->scales[i]);
    }
    free(animation->translations);
    free(animation->rotations);
    free(animation->scales);
}

int getChannelKeyFrame(AnimationChannel* channel, float time)
{
    if (!channel->times) return 0;

    if (channel->times[0] > time) return -1;
    if (channel->times[channel->frame_count - 1] < time) return channel->frame_count - 1;

    for (size_t i = 0; i < channel->frame_count - 1; ++i)
    {
        if ((channel->times[i] <= time) && (time < channel->times[i + 1]))
            return i;
    }
    return 0;
}

float getChannelTransform(AnimationChannel* channel, float time, uint8_t comps, float* t0, float* t1)
{
    if (!channel->transforms) return 0.0f;

    int frame = getChannelKeyFrame(channel, time);

    size_t offset = frame < 0 ? 0 : frame * comps;
    for (uint8_t i = 0; i < comps; ++i)
        t0[i] = channel->transforms[offset + i];

    if (channel->frame_count <= 1 || !channel->times || frame < 0 || frame >= channel->frame_count - 1)
        return 0.0f;

    offset += comps;
    for (uint8_t i = 0; i < comps; ++i)
        t1[i] = channel->transforms[offset + i];

    return (time - channel->times[frame]) / (channel->times[frame + 1] - channel->times[frame]);
}

int getAnimationTransform(const Animation* animation, size_t index, mat4* transform)
{
    if (index >= animation->channel_count) return IGNIS_FAILURE;

    AnimationChannel* translation = &animation->translations[index];
    AnimationChannel* rotation = &animation->rotations[index];
    AnimationChannel* scale = &animation->scales[index];

    if (!translation->frame_count && !rotation->frame_count && !scale->frame_count) return IGNIS_FAILURE;

    // translation
    vec3 t0 = { 0 }, t1 = { 0 };
    float t = getChannelTransform(translation, animation->time, 3, &t0.x, &t1.x);
    mat4 T = mat4_translation(vec3_lerp(t0, t1, t));

    // rotation
    quat q0 = quat_identity(), q1 = quat_identity();
    t = getChannelTransform(rotation, animation->time, 4, &q0.x, &q1.x);
    mat4 R = mat4_cast(quat_slerp(q0, q1, t));

    // scale
    vec3 s0 = { 1.0f, 1.0f, 1.0f }, s1 = { 1.0f, 1.0f, 1.0f };
    t = getChannelTransform(scale, animation->time, 3, &s0.x, &s1.x);
    mat4 S = mat4_scale(vec3_lerp(s0, s1, t));

    // T * R * S
    *transform = mat4_multiply(T, mat4_multiply(R, S));
    return IGNIS_SUCCESS;
}

void getAnimationJointTransforms(const Model* model, const Animation* animation, mat4* transforms)
{
    transforms[0] = mat4_identity();
    for (size_t i = 0; i < model->joint_count; ++i)
    {
        mat4 local = model->joint_locals[i];
        getAnimationTransform(animation, i, &local);

        uint32_t parent = model->joints[i];
        transforms[i] = mat4_multiply(transforms[parent], local);
    }

    for (size_t i = 0; i < model->joint_count; ++i)
    {
        transforms[i] = mat4_multiply(transforms[i], model->joint_inv_transforms[i]);
    }
}

void getBindPose(const Model* model, mat4* out)
{
    out[0] = mat4_identity();
    for (size_t i = 0; i < model->joint_count; ++i)
    {
        uint32_t parent = model->joints[i];
        out[i] = mat4_multiply(out[parent], model->joint_locals[i]);
    }

    for (size_t i = 0; i < model->joint_count; ++i)
    {
        out[i] = mat4_multiply(out[i], model->joint_inv_transforms[i]);
    }
}

void resetAnimation(Animation* animation)
{
    animation->time = 0.0f;
}

void tickAnimation(Animation* animation, float deltatime)
{
    animation->time += deltatime;
    animation->time = fmodf(animation->time, animation->duration);
}
