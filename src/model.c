#include "model.h"

#define CGLTF_IMPLEMENTATION
#include "cgltf.h"

#include "minimal/utils.h"

// ----------------------------------------------------------------
// utility
// ----------------------------------------------------------------
static size_t getMaterialIndex(const cgltf_material* materials, size_t count, const cgltf_primitive* primitive)
{
    for (size_t i = 0; i < count; ++i)
    {
        if (primitive->material == &materials[i]) return i;
    }
    return 0;
}

size_t getJointIndex(const cgltf_node* target, const cgltf_skin* skin, size_t fallback)
{
    for (size_t i = 0; i < skin->joints_count; ++i)
    {
        if (target == skin->joints[i]) return i;
    }
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

        MINIMAL_INFO("MODEL: Joint (%i) %s; parent: %i", i, node->name, model->joints[i]);
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
int loadModelGLTF(Model* model, Animation* animation, const char* dir, const char* filename)
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
    for (size_t i = 0; i < data->meshes_count; ++i) primitivesCount += data->meshes[i].primitives_count;

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
    for (size_t i = 0; i < data->materials_count; ++i)
    {
        loadMaterialGLTF(&model->materials[i], &data->materials[i], dir);
    }

    // Load meshes data
    for (size_t i = 0, meshIndex = 0; i < data->meshes_count; ++i)
    {
        for (size_t p = 0; p < data->meshes[i].primitives_count; ++p)
        {
            cgltf_primitive* primitive = &data->meshes[i].primitives[p];
            if (primitive->type != cgltf_primitive_type_triangles) continue;

            loadMeshGLTF(&model->meshes[meshIndex], primitive);
            model->mesh_materials[meshIndex] = getMaterialIndex(data->materials, data->materials_count, primitive);

            meshIndex++;
        }
    }

    // Load skin
    if (data->skins_count > 1)
    {
        IGNIS_ERROR("MODEL: can only load one skin (armature) per model, but gltf skins_count == %i", data->skins_count);
        return IGNIS_FAILURE;
    }

    // animation
    if (data->skins_count == 1 && animation)
    {

        loadSkinGLTF(model, &data->skins[0]);
        cgltf_skin skin = data->skins[0];
        MINIMAL_INFO("MODEL: Skin has %i joints", skin.joints_count);
        for (size_t i = 0; i < data->animations_count; ++i)
        {
            loadAnimationGLTF(animation, &data->animations[i], &skin);
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

    destroySkin(model);

    for (int i = 0; i < model->material_count; ++i)
        destroyMaterial(&model->materials[i]);

    free(model->materials);
}


// ----------------------------------------------------------------
// openGL stuff
// ----------------------------------------------------------------
int uploadMesh(Mesh* mesh)
{
    ignisGenerateVertexArray(&mesh->vao, 5);

    GLsizeiptr size2f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 2;
    GLsizeiptr size3f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 3;
    GLsizeiptr size4f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 4;
    GLsizeiptr size4u = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_UNSIGNED_INT) * 4;

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
        ignisVertexAttribIPointer(3, 4, GL_UNSIGNED_INT, 0, 0);
    }
    else
    {
        GLushort value[4] = { 0 };
        glVertexAttribI4uiv(3, value);
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

void bindMaterial(IgnisShader shader, const Material* material)
{
    ignisBindTexture2D(&material->base_texture, 0);

    ignisSetUniformi(shader, "baseTexture", 0);
    ignisSetUniform3f(shader, "baseColor", 1, &material->color.r);
}

void renderMesh(const Mesh* mesh)
{
    ignisBindVertexArray(&mesh->vao);

    if (mesh->vao.element_buffer.name)
        glDrawElements(GL_TRIANGLES, mesh->element_count, GL_UNSIGNED_INT, NULL);
    else
        glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
}

void renderModel(const Model* model, IgnisShader shader)
{
    ignisUseShader(shader);

    mat4 transform = mat4_identity();
    ignisSetUniformMat4(shader, "model", 1, transform.v[0]);

    for (size_t i = 0; i < model->mesh_count; ++i)
    {
        Mesh* mesh = &model->meshes[i];
        uint32_t material = model->mesh_materials[i];

        // bind material
        bindMaterial(shader, &model->materials[material]);

        renderMesh(mesh);
    }
}

void renderModelAnimated(const Model* model, const Animation* animation, IgnisShader shader)
{
    ignisUseShader(shader);

    mat4 transform = mat4_identity();
    ignisSetUniformMat4(shader, "model", 1, transform.v[0]);

    for (size_t i = 0; i < model->mesh_count; ++i)
    {
        Mesh* mesh = &model->meshes[i];
        uint32_t material = model->mesh_materials[i];

        // bind material
        bindMaterial(shader, &model->materials[material]);

        mat4 transforms[32] = { 0 };
        //getBindPose(model, transforms);
        getAnimationPose(model, animation, transforms);
        ignisSetUniformMat4(shader, "jointTransforms", animation->joint_count, transforms[0].v[0]);

        renderMesh(mesh);
    }
}
