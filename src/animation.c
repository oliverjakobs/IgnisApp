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

int loadAnimationGLTF(Animation* animation, cgltf_animation* gltf_animation, cgltf_skin* skin)
{
    animation->channel_count = skin->joints_count;

    animation->time = 0.0f;
    animation->duration = 0.0f;

    animation->translations = calloc(skin->joints_count, sizeof(AnimationChannel));
    animation->rotations = calloc(skin->joints_count, sizeof(AnimationChannel));
    animation->scales = calloc(skin->joints_count, sizeof(AnimationChannel));
    if (!animation->translations || !animation->rotations || !animation->scales) return IGNIS_FAILURE;

    for (size_t i = 0; i < gltf_animation->channels_count; ++i)
    {
        size_t joint_index = getJointIndex(gltf_animation->channels[i].target_node, skin, skin->joints_count);
        if (joint_index >= skin->joints_count) // Animation channel for a node not in the armature
            continue;

        cgltf_animation_channel* channel = &gltf_animation->channels[i];
        switch (channel->target_path)
        {
        case cgltf_animation_path_type_translation:
            loadAnimationChannelGLTF(&animation->translations[joint_index], channel->sampler);
            break;
        case cgltf_animation_path_type_rotation:
            loadAnimationChannelGLTF(&animation->rotations[joint_index], channel->sampler);
            break;
        case cgltf_animation_path_type_scale:
            loadAnimationChannelGLTF(&animation->scales[joint_index], channel->sampler);
            break;
        default:
            IGNIS_WARN("MODEL: Unsupported target_path on channel %d's sampler. Skipping.", i);
            break;
        }

        // update animation duration
        animation->duration = max(animation->duration, channel->sampler->input->max[0]);
    }

    // fill missing channels
    for (size_t i = 0; i < animation->channel_count; ++i)
    {
        cgltf_node* node = skin->joints[i];

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

    size_t frame = getChannelKeyFrame(channel, time);

    size_t offset = frame * comps;
    for (uint8_t i = 0; i < comps; ++i)
        t0[i] = channel->transforms[offset + i];

    if (channel->frame_count <= 1 || !channel->times) return 0.0f;

    offset += comps;
    for (uint8_t i = 0; i < comps; ++i)
        t1[i] = channel->transforms[offset + i];

    return (time - channel->times[frame]) / (channel->times[frame + 1] - channel->times[frame]);
}


void getAnimationPose(const Model* model, const Animation* animation, mat4* out)
{
    out[0] = mat4_identity();
    for (size_t i = 0; i < model->joint_count; ++i)
    {
        AnimationChannel* translation = &animation->translations[i];
        AnimationChannel* rotation = &animation->rotations[i];
        AnimationChannel* scale = &animation->scales[i];

        mat4 transform = model->joint_locals[i];
        if (translation->frame_count || rotation->frame_count || scale->frame_count)
        {
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
            transform = mat4_multiply(T, mat4_multiply(R, S));
        }

        uint32_t parent = model->joints[i];
        out[i] = mat4_multiply(out[parent], transform);
    }

    for (size_t i = 0; i < model->joint_count; ++i)
    {
        out[i] = mat4_multiply(out[i], model->joint_inv_transforms[i]);
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