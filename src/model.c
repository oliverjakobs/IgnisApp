#include "model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "minimal/utils.h"

#define LOAD_ATTRIBUTE(dst, accessor, components, type)             \
size_t offset = accessor->buffer_view->offset + accessor->offset;   \
int8_t* src = accessor->buffer_view->buffer->data;                  \
for (size_t i = 0; i < accessor->count; ++i)                        \
{                                                                   \
    for (uint8_t c = 0; c < components; ++c)                        \
        dst[components * i + c] = ((type*)(src + offset))[c];       \
    offset += accessor->stride;                                     \
}

int loadMeshGLTF(Mesh* mesh, const cgltf_primitive* primitive)
{
    // NOTE: Attributes data could be provided in several data formats (8, 8u, 16u, 32...),
    // Only some formats for each attribute type are supported, read info at the top of this function!
    for (unsigned int j = 0; j < primitive->attributes_count; j++)
    {
        cgltf_accessor* accessor = primitive->attributes[j].data;
        switch (primitive->attributes[j].type)
        {
        case cgltf_attribute_type_position:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->vertex_count = (uint32_t)accessor->count;
                mesh->positions = malloc(accessor->count * 3 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->positions, accessor, 3, float)

                mesh->min = (vec3){ accessor->min[0], accessor->min[1], accessor->min[2] };
                mesh->max = (vec3){ accessor->max[0], accessor->max[1], accessor->max[2] };
            }
            else IGNIS_WARN("MODEL: Vertices attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_normal:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->normals = malloc(accessor->count * 3 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->normals, accessor, 3, float)
            }
            else IGNIS_WARN("MODEL: Normal attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_tangent:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->tangents = malloc(accessor->count * 4 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->tangents, accessor, 4, float)
            }
            else IGNIS_WARN("MODEL: Tangent attribute data format not supported, use vec4 float");
            break;
        case cgltf_attribute_type_texcoord:
            if (accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->texcoords = malloc(accessor->count * 2 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->texcoords, accessor, 2, float)
            }
            else IGNIS_WARN("MODEL: Texcoords attribute data format not supported, use vec2 float");
            break;
        case cgltf_attribute_type_color:
            IGNIS_WARN("MODEL: Color attribute not supported");
            break;
        case cgltf_attribute_type_joints:
            if (accessor->type == cgltf_type_vec4)
            {
                mesh->joints = malloc(accessor->count * 4 * sizeof(uint16_t));
                if (accessor->component_type == cgltf_component_type_r_16u)
                {
                    LOAD_ATTRIBUTE(mesh->joints, accessor, 4, uint16_t)
                }
                else if (accessor->component_type == cgltf_component_type_r_8u)
                {
                    LOAD_ATTRIBUTE(mesh->joints, accessor, 4, uint8_t)
                }
            }
            else IGNIS_WARN("MODEL: Joint attribute data format not supported, use vec4 u16");
            break;
        case cgltf_attribute_type_weights:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->weights = malloc(accessor->count * 4 * sizeof(float));
                LOAD_ATTRIBUTE(mesh->weights, accessor, 4, float)
            }
            else IGNIS_WARN("MODEL: Joint weight attribute data format not supported, use vec4 float");
            break;
        default:
            IGNIS_WARN("MODEL: Unsupported attribute");

        }
    }

    // Load primitive indices data (if provided)
    if (primitive->indices != NULL)
    {
        cgltf_accessor* accessor = primitive->indices;

        mesh->element_count = (uint32_t)accessor->count;
        mesh->indices = malloc(accessor->count * sizeof(uint32_t));

        if (accessor->component_type == cgltf_component_type_r_32u)
        {
            LOAD_ATTRIBUTE(mesh->indices, accessor, 1, uint32_t)
        }
        else if (accessor->component_type == cgltf_component_type_r_16u)
        {
            LOAD_ATTRIBUTE(mesh->indices, accessor, 1, uint16_t)
        }
        else IGNIS_WARN("MODEL: Indices data format not supported, use u32");
    }

    return IGNIS_SUCCESS;
}

void destroyMesh(Mesh* mesh)
{
    ignisDeleteVertexArray(&mesh->vao);
    if (mesh->positions) free(mesh->positions);
    if (mesh->texcoords) free(mesh->texcoords);
    if (mesh->normals)   free(mesh->normals);
    if (mesh->colors)    free(mesh->colors);
    if (mesh->tangents)  free(mesh->tangents);
    if (mesh->joints)    free(mesh->joints);
    if (mesh->weights)   free(mesh->weights);
    if (mesh->indices)   free(mesh->indices);
}

static uint32_t getMaterialIndex(const cgltf_material* materials, size_t count, const cgltf_primitive* primitive)
{
    for (size_t m = 0; m < count; ++m)
    {
        if (&materials[m] == primitive->material)
            return (uint32_t)m;
    }
    return 0;
}

static void calcInverseBindTransform(const uint32_t* joints, size_t count, const mat4* locals, mat4* transforms)
{
    transforms[0] = mat4_identity();
    for (size_t i = 0; i < count; ++i)
    {
        uint32_t parent = joints[i];
        if (parent >= 0)
        {
            if (parent > i)
            {
                IGNIS_WARN("Assumes bones are toplogically sorted, but bone %d has parent %d. Skipping.", i, parent);
                continue;
            }

            transforms[i] = mat4_multiply(transforms[parent], locals[i]);
        }
    }
}

static uint32_t getJointIndex(const cgltf_node* target, const cgltf_skin* skin, uint32_t value)
{
    for (uint32_t k = 0; k < skin->joints_count; k++)
    {
        if (target == skin->joints[k]) return k;
    }
    return value;
}

static mat4 getNodeTransform(const cgltf_node* node)
{
    mat4 transform = mat4_identity();
    if(node->has_matrix)
    {
        memcpy(transform.v, node->matrix, sizeof(mat4));
    }
    else
    {
        // load T * R * S
        mat4 T = mat4_translation((vec3) { node->translation[0], node->translation[1], node->translation[2] });
        mat4 R = mat4_cast((quat) { node->rotation[0], node->rotation[1], node->rotation[2], node->rotation[3] });
        mat4 S = mat4_scale((vec3) { node->scale[0], node->scale[1], node->scale[2] });
        transform = mat4_multiply(mat4_multiply(T, R), S);
    }
    return transform;
}

int loadModelGLTF(Model* model, const cgltf_data* data, const char* dir)
{
    size_t primitivesCount = 0;
    for (size_t i = 0; i < data->meshes_count; i++) primitivesCount += data->meshes[i].primitives_count;

    // Load our model data: meshes and materials
    model->mesh_count = primitivesCount;
    model->meshes = calloc(model->mesh_count, sizeof(Mesh));

    if (!model->meshes) return IGNIS_FAILURE;

    model->material_count = data->materials_count; // extra slot for default material
    model->materials = calloc(model->material_count, sizeof(Material));

    if (!model->materials) return IGNIS_FAILURE;

    model->mesh_materials = calloc(model->mesh_count, sizeof(uint32_t));

    if (!model->mesh_materials) return IGNIS_FAILURE;

    // Load materials data
    //----------------------------------------------------------------------------------------------------
    for (unsigned int i = 0; i < data->materials_count; i++)
    {
        loadMaterialGLTF(&model->materials[i], &data->materials[i], dir);
    }

    // Load meshes data
    //----------------------------------------------------------------------------------------------------
    for (unsigned int i = 0, meshIndex = 0; i < data->meshes_count; i++)
    {
        // NOTE: meshIndex accumulates primitives
        for (unsigned int p = 0; p < data->meshes[i].primitives_count; p++)
        {
            cgltf_primitive* primitive = &data->meshes[i].primitives[p];
            // NOTE: We only support primitives defined by triangles
            // Other alternatives: points, lines, line_strip, triangle_strip
            if (primitive->type != cgltf_primitive_type_triangles) continue;

            loadMeshGLTF(&model->meshes[meshIndex], primitive);

            // Assign to the primitive mesh the corresponding material index
            model->mesh_materials[meshIndex] = getMaterialIndex(data->materials, data->materials_count, primitive);

            meshIndex++;    // Move to next mesh
        }
    }

    if (data->skins_count > 1)
    {
        IGNIS_ERROR("MODEL: can only load one skin (armature) per model, but gltf skins_count == %i", data->skins_count);
        return IGNIS_FAILURE;
    }

    // Load joints
    //----------------------------------------------------------------------------------------------------
    cgltf_skin* skin = &data->skins[0];
    model->joint_count = skin->joints_count;
    model->joints = malloc(skin->joints_count * sizeof(uint32_t));
    model->joint_locals = malloc(skin->joints_count * sizeof(mat4));
    model->joint_inv_transforms = malloc(skin->joints_count * sizeof(mat4));

    model->joint_locals[0] = mat4_identity();
    for (uint32_t i = 0; i < skin->joints_count; i++)
    {
        cgltf_node* node = skin->joints[i];

        MINIMAL_INFO("MODEL: Joint (%i) %s", i, node->name);

        // get joint parent
        uint32_t parent = getJointIndex(node->parent, skin, 0);
        model->joints[i] = parent;
        model->joint_locals[i] = mat4_multiply(model->joint_locals[parent], getNodeTransform(node));
    }

    for (uint32_t i = 0; i < skin->inverse_bind_matrices->count; ++i)
    {
        cgltf_accessor_read_float(skin->inverse_bind_matrices, i, model->joint_inv_transforms[i].v, 16);
    }

    //calcInverseBindTransform(model->joints, model->joint_count, model->joint_locals, model->joint_inv_transforms);

    return IGNIS_SUCCESS;
}

int loadModel(Model* model, Animation* animation, const char* dir, const char* filename)
{
    size_t size = 0;
    const char* path = ignisTextFormat("%s/%s", dir, filename);
    char* filedata = ignisReadFile(path, &size);

    if (!filedata) return IGNIS_FAILURE;

    cgltf_options options = { 0 };
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse(&options, filedata, size, &data);
    if (result != cgltf_result_success)
    {
        IGNIS_ERROR("MODEL: [%s] Failed to load glTF data", path);
        return IGNIS_FAILURE;
    }

    if (data->file_type == cgltf_file_type_glb)       MINIMAL_INFO("MODEL: [%s] Model basic data (glb) loaded successfully", filename);
    else if (data->file_type == cgltf_file_type_gltf) MINIMAL_INFO("MODEL: [%s] Model basic data (glTF) loaded successfully", filename);
    else IGNIS_WARN("MODEL: [%s] Model format not recognized", path);

    MINIMAL_INFO("    > Meshes count: %i", data->meshes_count);
    MINIMAL_INFO("    > Materials count: %i", data->materials_count);
    MINIMAL_INFO("    > Buffers count: %i", data->buffers_count);
    MINIMAL_INFO("    > Images count: %i", data->images_count);
    MINIMAL_INFO("    > Textures count: %i", data->textures_count);

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        IGNIS_ERROR("MODEL: [%s] Failed to load mesh/material buffers", path);
        return IGNIS_FAILURE;
    }

    if (!loadModelGLTF(model, data, dir))
    {
        IGNIS_ERROR("MODEL: [%s] Failed to load model", path);
        return IGNIS_FAILURE;
    }

    // animation
    if (data->skins_count == 1)
    {
        cgltf_skin skin = data->skins[0];
        MINIMAL_INFO("MODEL: Skin has %i joints", skin.joints_count);
        for (unsigned int i = 0; i < data->animations_count; i++)
        {
            animation->joint_count = skin.joints_count;

            TransformSampler* transforms = calloc(skin.joints_count, sizeof(TransformSampler));
            cgltf_accessor* times = NULL;

            MINIMAL_INFO("MODEL: Animation %i (%i channels):", i, data->animations[i].channels_count);
            for (unsigned int j = 0; j < data->animations[i].channels_count; j++)
            {
                uint32_t joint_index = getJointIndex(data->animations[i].channels[j].target_node, &skin, skin.joints_count);
                if (joint_index >= skin.joints_count) // Animation channel for a node not in the armature
                    continue;

                TransformSampler* sampler = &transforms[joint_index];

                cgltf_animation_channel* channel = &data->animations[i].channels[j];
                switch (channel->target_path)
                {
                case cgltf_animation_path_type_translation:
                    MINIMAL_INFO("MODEL: Translation Channel (%i): %i frames", j, channel->sampler->input->count);
                    if (!times) times = channel->sampler->input;
                    else if (times != channel->sampler->input)
                    {
                        IGNIS_WARN("Different input");
                        break;
                    }
                    sampler->translation = channel->sampler->output;
                    break;
                case cgltf_animation_path_type_rotation:
                    MINIMAL_INFO("MODEL: Rotation    Channel (%i): %i frames", j, channel->sampler->input->count);
                    if (!times) times = channel->sampler->input;
                    else if (times != channel->sampler->input)
                    {
                        IGNIS_WARN("Different input");
                        break;
                    }
                    sampler->rotation = channel->sampler->output;
                    break;
                case cgltf_animation_path_type_scale:
                    MINIMAL_INFO("MODEL: Scale       Channel (%i): %i frames", j, channel->sampler->input->count);
                    if (!times) times = channel->sampler->input;
                    else if (times != channel->sampler->input)
                    {
                        IGNIS_WARN("Different input");
                        break;
                    }
                    sampler->scale = channel->sampler->output;
                    break;
                default:
                    IGNIS_WARN("MODEL: Unsupported target_path on channel %d's sampler for animation %d. Skipping.", j, i);
                    break;
                }
            }
            loadAnimation(animation, times, transforms);

            free(transforms);
        }
    }

    MINIMAL_INFO("Model loaded");
    cgltf_free(data);
    free(filedata);

    return IGNIS_SUCCESS;
}

void destroyModel(Model* model)
{
    for (int i = 0; i < model->mesh_count; ++i)
        destroyMesh(&model->meshes[i]);

    free(model->meshes);
    free(model->mesh_materials);

    for (int i = 0; i < model->material_count; ++i)
        destroyMaterial(&model->materials[i]);

    free(model->materials);
}

int loadAnimation(Animation* animation, cgltf_accessor* times, TransformSampler* transforms)
{
    animation->frame_count = times->count;
    animation->times = malloc(times->count * sizeof(float));
    animation->transforms = malloc(times->count * sizeof(mat4*));

    MINIMAL_INFO("Animation duration: %f", animation->duration);

    if (!(animation->times && animation->transforms)) return IGNIS_FAILURE;

    for (size_t frame = 0; frame < animation->frame_count; ++frame)
    {
        cgltf_accessor_read_float(times, frame, &animation->times[frame], 1);
        animation->transforms[frame] = malloc(animation->joint_count * sizeof(mat4));

        for (size_t joint = 0; joint < animation->joint_count; ++joint)
        {
            TransformSampler sampler = transforms[joint];

            vec3 translation = { 0.0f };
            if (sampler.translation) cgltf_accessor_read_float(sampler.translation, frame, &translation.x, 3);

            quat rotation = quat_identity();
            if (sampler.rotation) cgltf_accessor_read_float(sampler.rotation, frame, &rotation.x, 4);

            vec3 scale = { 1.0f, 1.0f, 1.0f };
            if (sampler.scale) cgltf_accessor_read_float(sampler.scale, frame, &scale.x, 3);

            // set T * R * S
            //mat4 T = mat4_identity(); 
            mat4 T = mat4_translation(translation);
            mat4 R = mat4_cast(rotation);
            mat4 S = mat4_scale(scale);
            animation->transforms[frame][joint] = mat4_multiply(mat4_multiply(T, R), S);
        }
    }

    return IGNIS_SUCCESS;
}

void destroyAnimation(Animation* animation)
{
    for (size_t f = 0; f < animation->frame_count; ++f)
    {
        free(animation->transforms[f]);
    }
    free(animation->transforms);
    free(animation->times);
}

void getAnimationPoses(const Model* model, const mat4* in, mat4* out)
{
    mat4 model_transforms[32] = { 0 };
    model_transforms[0] = mat4_identity();

    for (size_t i = 0; i < model->joint_count; ++i)
    {
        mat4 local_transform = mat4_multiply(model->joint_locals[i], in[i]);

        uint32_t parent = model->joints[i];
        model_transforms[i] = mat4_multiply(model_transforms[parent], local_transform);

        //out[i] = mat4_identity();
        out[i] = mat4_multiply(model_transforms[i], model->joint_inv_transforms[i]);
    }
}

int uploadMesh(Mesh* mesh)
{
    ignisGenerateVertexArray(&mesh->vao, 5);

    GLsizeiptr size2f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 2;
    GLsizeiptr size3f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 3;
    GLsizeiptr size4f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 4;
    GLsizeiptr size4u = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_UNSIGNED_SHORT) * 4;

    // positions
    ignisLoadArrayBuffer(&mesh->vao, 0, mesh->vertex_count * size3f, mesh->positions, GL_STATIC_DRAW);
    ignisVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (mesh->texcoords) // texcoords
    {
        ignisLoadArrayBuffer(&mesh->vao, 1, mesh->vertex_count * size2f, mesh->texcoords, GL_STATIC_DRAW);
        ignisVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[2] = { 0.0f };
        glVertexAttrib2fv(2, value);
        glDisableVertexAttribArray(1);
    }

    if (mesh->normals) // normals
    {
        ignisLoadArrayBuffer(&mesh->vao, 2, mesh->vertex_count * size3f, mesh->normals, GL_STATIC_DRAW);
        ignisVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[3] = { 0.0f };
        glVertexAttrib3fv(2, value);
        glDisableVertexAttribArray(2);
    }

    if (mesh->joints) // joints
    {
        ignisLoadArrayBuffer(&mesh->vao, 3, mesh->vertex_count * size4u, mesh->joints, GL_STATIC_DRAW);
        ignisVertexAttribIPointer(3, 4, GL_UNSIGNED_SHORT, 0, 0);
    }
    else
    {
        GLushort value[4] = { 0 };
        glVertexAttribI4usv(3, value);
        glDisableVertexAttribArray(3);
    }

    if (mesh->weights) // weights
    {
        ignisLoadArrayBuffer(&mesh->vao, 4, mesh->vertex_count * size4f, mesh->weights, GL_STATIC_DRAW);
        ignisVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[4] = { 0.0f };
        glVertexAttrib4fv(4, value);
        glDisableVertexAttribArray(4);
    }

    if (mesh->indices)
        ignisLoadElementBuffer(&mesh->vao, mesh->indices, mesh->element_count, GL_STATIC_DRAW);

    return 0;
}

void renderModel(const Model* model, const Animation* animation, IgnisShader shader)
{
    for (int i = 0; i < model->mesh_count; ++i)
    {
        Mesh* mesh = &model->meshes[i];
        uint32_t material = model->mesh_materials[i];

        // bind material
        IgnisTexture2D* texture = &model->materials[material].base_texture;
        ignisBindTexture2D(texture, 0);

        ignisSetUniformi(shader, "baseTexture", 0);
        ignisSetUniform3f(shader, "baseColor", 1, &model->materials[material].color.r);

        mat4 transforms[32] = { 0 };
        getAnimationPoses(model, animation->transforms[animation->current_frame], transforms);
        ignisSetUniformMat4(shader, "jointTransforms", animation->joint_count, transforms[0].v[0]);

        ignisBindVertexArray(&mesh->vao);

        if (mesh->vao.element_buffer.name)
            glDrawElements(GL_TRIANGLES, mesh->element_count, GL_UNSIGNED_INT, NULL);
        else
            glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
    }
}
