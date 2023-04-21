#include "model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "minimal/utils.h"

int loadDefaultMaterial(Material* material)
{
    material->color = IGNIS_WHITE;
    material->base_texture = IGNIS_DEFAULT_TEXTURE2D;
    material->normal = IGNIS_DEFAULT_TEXTURE2D;
    material->occlusion = IGNIS_DEFAULT_TEXTURE2D;
    material->emmisive = IGNIS_DEFAULT_TEXTURE2D;
    return IGNIS_SUCCESS;
}

int loadMaterialGLTF(const cgltf_material* gltf_material, Material* material, const char* dir)
{
    // set defaults
    loadDefaultMaterial(material);

    // Check glTF material flow: PBR metallic/roughness flow
    // NOTE: Alternatively, materials can follow PBR specular/glossiness flow
    if (gltf_material->has_pbr_metallic_roughness)
    {
        // Load base color texture
        if (gltf_material->pbr_metallic_roughness.base_color_texture.texture)
        {
            cgltf_image* image = gltf_material->pbr_metallic_roughness.base_color_texture.texture->image;
            ignisCreateTexture2D(&material->base_texture, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);

        }
        // Load base color factor
        material->color.r = gltf_material->pbr_metallic_roughness.base_color_factor[0];
        material->color.g = gltf_material->pbr_metallic_roughness.base_color_factor[1];
        material->color.b = gltf_material->pbr_metallic_roughness.base_color_factor[2];
        material->color.a = gltf_material->pbr_metallic_roughness.base_color_factor[3];

        // Load metallic/roughness texture
        if (gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture)
        {
            cgltf_image* image = gltf_material->pbr_metallic_roughness.metallic_roughness_texture.texture->image;
            ignisCreateTexture2D(&material->metallic_roughness, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);

            // Load metallic/roughness material properties
            material->roughness = gltf_material->pbr_metallic_roughness.roughness_factor;
            material->metallic = gltf_material->pbr_metallic_roughness.metallic_factor;
        }
    }
    else if (gltf_material->has_pbr_specular_glossiness)
    {
        // Load diffuse texture
        if (gltf_material->pbr_specular_glossiness.diffuse_texture.texture)
        {
            cgltf_image* image = gltf_material->pbr_specular_glossiness.diffuse_texture.texture->image;
            ignisCreateTexture2D(&material->base_texture, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
        }
        // Load diffuse factor
        material->color.r = gltf_material->pbr_specular_glossiness.diffuse_factor[0];
        material->color.g = gltf_material->pbr_specular_glossiness.diffuse_factor[1];
        material->color.b = gltf_material->pbr_specular_glossiness.diffuse_factor[2];
        material->color.a = gltf_material->pbr_specular_glossiness.diffuse_factor[3];
    }

    // Load normal texture
    if (gltf_material->normal_texture.texture)
    {
        cgltf_image* image = gltf_material->normal_texture.texture->image;
        ignisCreateTexture2D(&material->normal, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
    }

    // Load ambient occlusion texture
    if (gltf_material->occlusion_texture.texture)
    {
        cgltf_image* image = gltf_material->occlusion_texture.texture->image;
        ignisCreateTexture2D(&material->occlusion, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
    }

    // Load emissive texture
    if (gltf_material->emissive_texture.texture)
    {
        cgltf_image* image = gltf_material->emissive_texture.texture->image;
        ignisCreateTexture2D(&material->emmisive, ignisTextFormat("%s/%s", dir, image->uri), 0, NULL);
    }

    // Other possible materials not supported by raylib pipeline:
    // has_clearcoat, has_transmission, has_volume, has_ior, has specular, has_sheen
    return IGNIS_SUCCESS;
}

float* loadAttribute_32f(cgltf_accessor* accessor, uint8_t components)
{
    if (accessor->count <= 0 || components <= 0) return NULL;

    float* data = malloc(accessor->count * components * sizeof(float));

    if (!data) return NULL;

    size_t offset = accessor->buffer_view->offset + accessor->offset;
    for (size_t i = 0; i < accessor->count; ++i)
    {
        float* buffer = (float*)((int8_t*)accessor->buffer_view->buffer->data + offset);
        for (uint8_t c = 0; c < components; ++c)
        {
            data[components * i + c] = buffer[c];
        }
        offset += accessor->stride;
    }
    return data;
}

uint16_t* loadAttribute_16u(cgltf_accessor* accessor, uint8_t components)
{
    if (accessor->count <= 0 || components <= 0) return NULL;

    uint16_t* data = malloc(accessor->count * components * sizeof(uint16_t));

    if (!data) return NULL;

    size_t offset = accessor->buffer_view->offset + accessor->offset;
    for (size_t i = 0; i < accessor->count; ++i)
    {
        uint16_t* buffer = (uint16_t*)((int8_t*)accessor->buffer_view->buffer->data + offset);
        for (uint8_t c = 0; c < components; ++c)
        {
            data[components * i + c] = buffer[c];
        }
        offset += accessor->stride;
    }
    return data;
}

int loadMeshGLTF(const cgltf_primitive* primitive, Mesh* mesh)
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
                MINIMAL_INFO("Positions count: %i", accessor->count);
                mesh->vertex_count = (uint32_t)accessor->count;
                mesh->positions = loadAttribute_32f(accessor, 3);

                mesh->min = (vec3){ accessor->min[0], accessor->min[1], accessor->min[2] };
                mesh->max = (vec3){ accessor->max[0], accessor->max[1], accessor->max[2] };
            }
            else IGNIS_WARN("MODEL: Vertices attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_normal:
            if (accessor->type == cgltf_type_vec3 && accessor->component_type == cgltf_component_type_r_32f)
            {
                MINIMAL_INFO("Normals count: %i", accessor->count);
                mesh->normals = loadAttribute_32f(accessor, 3);
            }
            else IGNIS_WARN("MODEL: Normal attribute data format not supported, use vec3 float");
            break;
        case cgltf_attribute_type_tangent:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->tangents = loadAttribute_32f(accessor, 4);
            }
            else IGNIS_WARN("MODEL: Tangent attribute data format not supported, use vec4 float");
            break;
        case cgltf_attribute_type_texcoord:
            if (accessor->type == cgltf_type_vec2 && accessor->component_type == cgltf_component_type_r_32f)
            {
                mesh->texcoords = loadAttribute_32f(accessor, 2);
            }
            else IGNIS_WARN("MODEL: Texcoords attribute data format not supported, use vec2 float");
            break;
        case cgltf_attribute_type_color:
            IGNIS_WARN("MODEL: Color attribute not supported");
            break;
        case cgltf_attribute_type_joints:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_16u)
            {
                MINIMAL_INFO("Joints count: %i", accessor->count);
                mesh->joints = loadAttribute_16u(accessor, 4);
            }
            else IGNIS_WARN("MODEL: Joint attribute data format not supported, use vec4 u16");
            break;
        case cgltf_attribute_type_weights:
            if (accessor->type == cgltf_type_vec4 && accessor->component_type == cgltf_component_type_r_32f)
            {
                MINIMAL_INFO("Weights count: %i", accessor->count);
                mesh->weights = loadAttribute_32f(accessor, 4);
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
            size_t offset = accessor->buffer_view->offset + accessor->offset;
            int8_t* buffer = accessor->buffer_view->buffer->data;
            for (size_t k = 0; k < accessor->count; k++)
            {
                mesh->indices[k] = ((uint32_t*)(buffer + offset))[0];
                offset += accessor->stride;
            }
        }
        else if (accessor->component_type == cgltf_component_type_r_16u)
        {
            size_t offset = accessor->buffer_view->offset + accessor->offset;
            int8_t* buffer = accessor->buffer_view->buffer->data;
            for (size_t k = 0; k < accessor->count; k++)
            {
                mesh->indices[k] = ((uint16_t*)(buffer + offset))[0];
                offset += accessor->stride;
            }
        }
        else IGNIS_WARN("MODEL: Indices data format not supported, use u32");
    }
    else mesh->element_count = mesh->vertex_count;    // Unindexed mesh

    return IGNIS_SUCCESS;
}

int uploadMesh(Mesh* mesh)
{
    ignisGenerateVertexArray(&mesh->vao);

    GLsizeiptr size2f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 2;
    GLsizeiptr size3f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 3;
    GLsizeiptr size4f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 4;
    GLsizeiptr size4u = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_UNSIGNED_SHORT) * 4;

    // positions
    ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size3f, mesh->positions, GL_STATIC_DRAW);
    ignisVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    if (mesh->texcoords) // texcoords
    {
        ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size2f, mesh->texcoords, GL_STATIC_DRAW);
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
        ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size3f, mesh->normals, GL_STATIC_DRAW);
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
        ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size4u, mesh->joints, GL_STATIC_DRAW);
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
        ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size4f, mesh->weights, GL_STATIC_DRAW);
        ignisVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[4] = { 0.0f };
        glVertexAttrib4fv(4, value);
        glDisableVertexAttribArray(4);
    }


    if (mesh->indices)
    {
        ignisLoadElementBuffer(&mesh->vao, mesh->indices, mesh->element_count, GL_STATIC_DRAW);
    }
    return 0;
}

static uint32_t getMaterialIndex(const cgltf_material* materials, size_t count, const cgltf_primitive* primitive)
{
    for (size_t m = 0; m < count; ++m)
    {
        if (&materials[m] == primitive->material)
            return (uint32_t)m;   // +1 with default material
    }
    return 0;
}

static void calcInverseBindTransform(JointInfo* joints, size_t count, mat4* transforms)
{
    for (size_t i = 0; i < count; i++)
    {
        if (joints[i].parent >= 0)
        {
            if (joints[i].parent > i)
            {
                IGNIS_WARN("Assumes bones are toplogically sorted, but bone %d has parent %d. Skipping.", i, joints[i].parent);
                continue;
            }

            transforms[i] = mat4_multiply(transforms[joints[i].parent], joints[i].local);
        }
    }
}

int loadModel(Model* model, const char* dir, const char* filename)
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
        loadMaterialGLTF(&data->materials[i], &model->materials[i], dir);
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

            loadMeshGLTF(primitive, &model->meshes[meshIndex]);

            // Assign to the primitive mesh the corresponding material index
            model->mesh_materials[meshIndex] = getMaterialIndex(data->materials, data->materials_count, primitive);

            meshIndex++;    // Move to next mesh
        }
    }

    if (data->skins_count > 1)
    {
        IGNIS_ERROR("MODEL: can only load one skin (armature) per model, but gltf skins_count == %i", data->skins_count);
    }

    // Load bones
    //----------------------------------------------------------------------------------------------------
    cgltf_skin* skin = &data->skins[0];
    model->joint_count = skin->joints_count;
    model->joints = malloc(skin->joints_count * sizeof(JointInfo));
    model->joint_inv_transform = malloc(skin->joints_count * sizeof(mat4));

    for (uint32_t i = 0; i < skin->joints_count; i++)
    {
        cgltf_node* node = skin->joints[i];

        MINIMAL_INFO("MODEL: Joint (%i) %s", i, node->name);

        // Find parent bone index
        uint32_t parent = 0;

        for (uint32_t j = 0; j < skin->joints_count; j++)
        {
            if (skin->joints[j] == node->parent)
            {
                parent = j;
                break;
            }
        }

        model->joints[i].parent = parent;
        memcpy(model->joints[i].local.v, node->matrix, 4 * 4 * sizeof(float));
    }

    calcInverseBindTransform(model->joints, model->joint_count, model->joint_inv_transform);

    // animation
    if (data->skins_count == 1)
    {
        cgltf_skin skin = data->skins[0];

        for (unsigned int i = 0; i < data->animations_count; i++)
        {
            cgltf_animation* animation = &data->animations[i];

            vec3 translation = { 0.0f };
            quat rotation = quat_identity();
            vec3 scale = { 1.0f, 1.0f, 1.0f };

            MINIMAL_INFO("MODEL: Animation %i (%i channels):", i, animation->channels_count);
            for (unsigned int j = 0; j < animation->channels_count; j++)
            {
                cgltf_animation_channel* channel = &animation->channels[j];
                MINIMAL_INFO("MODEL: Channel (%i) input: %s, output: %s", j, channel->sampler->input->name, channel->sampler->output->name);

                switch (channel->target_path)
                {
                case cgltf_animation_path_type_translation:
                    MINIMAL_INFO("    > Translate channel (%i)", j);
                    break;
                case cgltf_animation_path_type_rotation:
                    MINIMAL_INFO("    > Rotation channel (%i)", j);
                    break;
                case cgltf_animation_path_type_scale:
                    MINIMAL_INFO("    > Scale channel (%i)", j);
                    break;
                case cgltf_animation_path_type_weights:
                    MINIMAL_INFO("    > Weights channel (%i)", j);
                    break;
                default:
                    IGNIS_WARN("MODEL: Unsupported target_path on channel %d's sampler for animation %d. Skipping.", j, i);
                    break;
                }
            }
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
    {
        Mesh* mesh = &model->meshes[i];
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
    free(model->meshes);
    free(model->mesh_materials);

    for (int i = 0; i < model->material_count; ++i)
    {
        Material* material = &model->materials[i];
        if (!ignisIsDefaultTexture2D(material->base_texture)) ignisDeleteTexture2D(&material->base_texture);
        if (!ignisIsDefaultTexture2D(material->normal))       ignisDeleteTexture2D(&material->normal);
        if (!ignisIsDefaultTexture2D(material->occlusion))    ignisDeleteTexture2D(&material->occlusion);
        if (!ignisIsDefaultTexture2D(material->emmisive))     ignisDeleteTexture2D(&material->emmisive);
    }
    free(model->materials);
}

void renderModel(const Model* model, IgnisShader shader)
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

        mat4 transforms[] = {
            mat4_identity(),
            mat4_identity(),
            mat4_identity(),
            mat4_identity()
        };
        ignisSetUniformMat4(shader, "jointTransforms", 4, transforms[0].v[0]);

        ignisBindVertexArray(&mesh->vao);
        //glDrawArrays(GL_TRIANGLES, 0, mesh->element_count);
        glDrawElements(GL_TRIANGLES, mesh->element_count, GL_UNSIGNED_INT, NULL);
    }
}
