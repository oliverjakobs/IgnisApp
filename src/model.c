#include "model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "minimal/utils.h"

// ----------------------------------------------------------------
// utility
// ----------------------------------------------------------------
static uint32_t getMaterialIndex(const cgltf_material* target, const cgltf_material* materials, size_t count)
{
    for (uint32_t i = 0; i < count; ++i)
        if (target == &materials[i]) return i;

    return 0;
}

size_t getMeshIndex(const cgltf_mesh* target, const cgltf_mesh* meshes, size_t count)
{
    size_t mesh_index = 0;
    for (size_t i = 0; i < count; ++i)
    {
        if (target == &meshes[i]) return mesh_index;
        mesh_index += meshes[i].primitives_count;
    }
    return 0;
}

uint32_t getJointIndex(const cgltf_node* target, const cgltf_skin* skin, uint32_t fallback)
{
    for (uint32_t i = 0; i < skin->joints_count; ++i)
        if (target == skin->joints[i]) return i;

    return fallback;
}

// ----------------------------------------------------------------
// skin
// ----------------------------------------------------------------
int loadSkinGLTF(Model* model, cgltf_skin* skin)
{
    model->joint_count = skin->joints_count;
    model->joints = malloc(skin->joints_count * sizeof(uint32_t));
    model->joint_locals = malloc(skin->joints_count * sizeof(mat4));
    model->joint_inv_transforms = malloc(skin->joints_count * sizeof(mat4));

    if (!(model->joints && model->joint_locals && model->joint_inv_transforms)) return IGNIS_FAILURE;

    for (size_t i = 0; i < skin->joints_count; ++i)
    {
        cgltf_node* node = skin->joints[i];
        model->joints[i] = getJointIndex(node->parent, skin, 0);
        cgltf_node_transform_local(node, model->joint_locals[i].v[0]);
        cgltf_accessor_read_float(skin->inverse_bind_matrices, i, model->joint_inv_transforms[i].v[0], 16);
    }

    return IGNIS_SUCCESS;
}

void destroySkin(Model* model)
{
    if (model->joints) free(model->joints);
    if (model->joint_locals) free(model->joint_locals);
    if (model->joint_inv_transforms) free(model->joint_inv_transforms);
}

// ----------------------------------------------------------------
// model
// ----------------------------------------------------------------
int loadModelGLTF(Model* model, cgltf_data* data, const char* dir)
{

    // count primitives
    model->mesh_count = 0;
    for (size_t i = 0; i < data->meshes_count; ++i)
        model->mesh_count += data->meshes[i].primitives_count;

    // count mesh instances
    model->instance_count = 0;
    for (size_t i = 0; i < data->nodes_count; ++i)
        if (data->nodes[i].mesh) model->instance_count += data->nodes[i].mesh->primitives_count;

    // allocate memory
    model->meshes = calloc(model->mesh_count, sizeof(Mesh));

    if (!model->meshes) return IGNIS_FAILURE;

    model->material_count = data->materials_count; // extra slot for default material
    model->materials = calloc(model->material_count, sizeof(Material));

    if (!model->materials) return IGNIS_FAILURE;

    model->instances = malloc(model->instance_count * sizeof(uint32_t));
    model->transforms = malloc(model->instance_count * sizeof(mat4));

    if (!model->instances || !model->transforms) return IGNIS_FAILURE;

    // Load materials
    for (size_t i = 0; i < data->materials_count; ++i)
    {
        loadMaterialGLTF(&model->materials[i], &data->materials[i], dir);
    }

    // Load meshes
    size_t mesh_index = 0;
    for (size_t i = 0; i < data->meshes_count; ++i)
    {
        for (size_t p = 0; p < data->meshes[i].primitives_count; ++p)
        {
            cgltf_primitive* primitive = &data->meshes[i].primitives[p];
            uint32_t material = getMaterialIndex(primitive->material, data->materials, data->materials_count);
            loadMeshGLTF(&model->meshes[mesh_index], primitive, (uint32_t)i, material);

            mesh_index++;
        }
    }

    size_t instance_index = 0;
    for (size_t i = 0; i < data->nodes_count; ++i)
    {
        cgltf_mesh* mesh_data = data->nodes[i].mesh;
        if (!mesh_data) continue;

        mat4 transform = mat4_identity();
        cgltf_node_transform_world(&data->nodes[i], transform.v[0]);

        size_t mesh_index = getMeshIndex(mesh_data, data->meshes, data->meshes_count);
        for (size_t p = 0; p < mesh_data->primitives_count; ++p)
        {
            model->instances[instance_index] = mesh_index + p;
            model->transforms[instance_index] = transform;
            instance_index++;
        }
    }

    // Load skin
    if (data->skins_count == 1)
    {
        loadSkinGLTF(model, &data->skins[0]);
    }

    MINIMAL_INFO("Model loaded");
    return IGNIS_SUCCESS;
}

void destroyModel(Model* model)
{
    for (int i = 0; i < model->mesh_count; ++i)
        destroyMesh(&model->meshes[i]);

    free(model->meshes);

    free(model->instances);
    free(model->transforms);

    destroySkin(model);

    for (int i = 0; i < model->material_count; ++i)
        destroyMaterial(&model->materials[i]);

    free(model->materials);
}

Animation* loadAnimationsGLTF(cgltf_data* data, size_t* count)
{
    if (!data->animations_count) return NULL;

    Animation* animations = malloc(data->animations_count * sizeof(Animation));

    if (!animations) return NULL;

    *count = data->animations_count;

    for (size_t i = 0; i < data->animations_count; ++i)
    {
        loadAnimationGLTF(&animations[i], &data->animations[i], data);
    }

    return animations;
}

// ----------------------------------------------------------------
// openGL stuff
// ----------------------------------------------------------------
int uploadMesh(Mesh* mesh)
{
    ignisGenerateVertexArray(&mesh->vao, 6);

    size_t size2f = ignisGetTypeSize(IGNIS_FLOAT) * 2;
    size_t size3f = ignisGetTypeSize(IGNIS_FLOAT) * 3;
    size_t size4f = ignisGetTypeSize(IGNIS_FLOAT) * 4;
    size_t size4u = ignisGetTypeSize(IGNIS_UINT32) * 4;

    // positions
    ignisLoadArrayBuffer(&mesh->vao, 0, mesh->vertex_count * size3f, mesh->positions, IGNIS_STATIC_DRAW);
    ignisVertexAttribPointer(0, 3, IGNIS_FLOAT, GL_FALSE, 0, 0);

    if (mesh->texcoords) // texcoords
    {
        ignisLoadArrayBuffer(&mesh->vao, 1, mesh->vertex_count * size2f, mesh->texcoords, IGNIS_STATIC_DRAW);
        ignisVertexAttribPointer(1, 2, IGNIS_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[2] = { 0.0f };
        glVertexAttrib2fv(2, value);
        glDisableVertexAttribArray(1);
    }

    if (mesh->normals) // normals
    {
        ignisLoadArrayBuffer(&mesh->vao, 2, mesh->vertex_count * size3f, mesh->normals, IGNIS_STATIC_DRAW);
        ignisVertexAttribPointer(2, 3, IGNIS_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[3] = { 0.0f };
        glVertexAttrib3fv(2, value);
        glDisableVertexAttribArray(2);
    }

    if (mesh->joints) // joints
    {
        ignisLoadArrayBuffer(&mesh->vao, 3, mesh->vertex_count * size4u, mesh->joints, IGNIS_STATIC_DRAW);
        ignisVertexAttribIPointer(3, 4, IGNIS_UINT32, 0, 0);
    }
    else
    {
        GLuint value[4] = { 0 };
        glVertexAttribI4uiv(3, value);
        glDisableVertexAttribArray(3);
    }

    if (mesh->weights) // weights
    {
        ignisLoadArrayBuffer(&mesh->vao, 4, mesh->vertex_count * size4f, mesh->weights, IGNIS_STATIC_DRAW);
        ignisVertexAttribPointer(4, 4, IGNIS_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[4] = { 0.0f };
        glVertexAttrib4fv(4, value);
        glDisableVertexAttribArray(4);
    }

    if (mesh->indices)
        ignisLoadElementBuffer(&mesh->vao, 5, mesh->indices, mesh->element_count, IGNIS_STATIC_DRAW);

    return 0;
}

void bindMaterial(IgnisShader shader, const Material* material)
{
    ignisBindTexture2D(&material->base_texture, 0);

    ignisSetUniformi(shader, "baseTexture", 0);
    ignisSetUniform3f(shader, "baseColor", 1, &material->color.r);
}

void renderMesh(const Mesh* mesh)
{
    ignisBindVertexArray(&mesh->vao);

    if (mesh->element_count)
        glDrawElements(mesh->type, mesh->element_count, GL_UNSIGNED_INT, NULL);
    else
        glDrawArrays(mesh->type, 0, mesh->vertex_count);
}

void renderModel(const Model* model, const Animation* animation, IgnisShader shader)
{
    ignisUseShader(shader);

    for (size_t i = 0; i < model->instance_count; ++i)
    {
        Mesh* mesh = &model->meshes[model->instances[i]];

        mat4 transform = model->transforms[i];
        if (getAnimationTransform(animation, mesh->group, &transform))
            transform = mat4_multiply(model->transforms[i], transform);
        ignisSetUniformMat4(shader, "model", 1, transform.v[0]);

        // bind material
        bindMaterial(shader, &model->materials[mesh->material]);

        renderMesh(mesh);
    }
}

void renderModelSkinned(const Model* model, const Animation* animation, IgnisShader shader)
{
    ignisUseShader(shader);

    for (size_t i = 0; i < model->instance_count; ++i)
    {
        Mesh* mesh = &model->meshes[model->instances[i]];

        mat4 transform = model->transforms[i];
        ignisSetUniformMat4(shader, "model", 1, transform.v[0]);

        mat4 transforms[32] = { 0 };
        //getBindPose(model, transforms);
        getAnimationJointTransforms(model, animation, transforms);
        ignisSetUniformMat4(shader, "jointTransforms", model->joint_count, transforms[0].v[0]);

        // bind material
        bindMaterial(shader, &model->materials[mesh->material]);

        renderMesh(mesh);
    }
}
