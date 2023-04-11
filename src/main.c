#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

#include "model.h"

static void ignisErrorCallback(ignisErrorLevel level, const char *desc);
static void printVersionInfo();

IgnisFont font;

float width, height;
mat4 screen_projection;

IgnisShader shader;

int view_mode = 0;
int poly_mode = 0;

vec3 camera_pos = { 0.0f, 0.0f, 10.0f };
float cameraspeed = 1.0f;

int uploadMesh(Mesh* mesh)
{
    ignisGenerateVertexArray(&mesh->vao);

    GLsizeiptr size2f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 2;
    GLsizeiptr size3f = (GLsizeiptr)ignisGetOpenGLTypeSize(GL_FLOAT) * 3;

    // positions
    ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size3f, mesh->positions, GL_STATIC_DRAW);
    ignisVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

    // texcoords
    ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size2f, mesh->texcoords, GL_STATIC_DRAW);
    ignisVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, 0);

    if (mesh->normals) // normals
    {
        ignisAddArrayBuffer(&mesh->vao, mesh->vertex_count * size3f, mesh->normals, GL_STATIC_DRAW);
        ignisVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 0, 0);
    }
    else
    {
        float value[3] = { 0.0f, 0.0f, 0.0f };
        glVertexAttrib3fv(2, value);
        glDisableVertexAttribArray(2);
    }


    if (mesh->indices)
    {
        ignisLoadElementBuffer(&mesh->vao, mesh->indices, mesh->element_count, GL_STATIC_DRAW);
    }
    return 0;
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
        MINIMAL_ERROR("MODEL: [%s] Failed to load glTF data", path);
        return IGNIS_FAILURE;
    }

    if (data->file_type == cgltf_file_type_glb)       MINIMAL_TRACE("MODEL: [%s] Model basic data (glb) loaded successfully", filename);
    else if (data->file_type == cgltf_file_type_gltf) MINIMAL_TRACE("MODEL: [%s] Model basic data (glTF) loaded successfully", filename);
    else MINIMAL_TRACE("MODEL: [%s] Model format not recognized", path);

    MINIMAL_INFO("    > Meshes count: %i",     data->meshes_count);
    MINIMAL_INFO("    > Materials count: %i",  data->materials_count);
    MINIMAL_INFO("    > Buffers count: %i",    data->buffers_count);
    MINIMAL_INFO("    > Images count: %i",     data->images_count);
    MINIMAL_INFO("    > Textures count: %i",   data->textures_count);

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        MINIMAL_ERROR("MODEL: [%s] Failed to load mesh/material buffers", path);
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
            // NOTE: We only support primitives defined by triangles
            // Other alternatives: points, lines, line_strip, triangle_strip
            if (data->meshes[i].primitives[p].type != cgltf_primitive_type_triangles) continue;

            loadMeshGLTF(&data->meshes[i].primitives[p], &model->meshes[meshIndex]);

            meshIndex++;       // Move to next mesh
        }
    }

    MINIMAL_INFO("Model loaded");
    cgltf_free(data);

    return IGNIS_SUCCESS;
}

Model robot;

static void setViewport(float w, float h)
{
    width = w;
    height = h;
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
}

int onLoad(MinimalApp *app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    // ignisSetAllocator(FrostMemoryGetAllocator(), tb_mem_malloc, tb_mem_realloc, tb_mem_free);
    ignisSetErrorCallback(ignisErrorCallback);

#ifdef _DEBUG
    int debug = 1;
    minimalEnableDebug(app, debug);
#else
    int debug = 0;
#endif

    if (!ignisInit(debug))
    {
        MINIMAL_ERROR("[IGNIS] Failed to initialize Ignis");
        return MINIMAL_FAIL;
    }

    printVersionInfo();

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    glEnable(GL_DEPTH_TEST);

    /* font renderer */
    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererInit();
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    setViewport((float)w, (float)h);

    /* shader */
    shader = ignisCreateShadervf("res/shaders/shader.vert", "res/shaders/shader.frag");

    /* gltf model */
    //loadModel(&robot, "res/models/box", "Box.gltf");
    loadModel(&robot, "res/models/walking_robot", "scene.gltf");

    for (int i = 0; i < robot.mesh_count; i++) uploadMesh(&robot.meshes[i]);

    return MINIMAL_OK;
}

void onDestroy(MinimalApp *app)
{
    ignisDeleteShader(shader);
    
    ignisDeleteFont(&font);

    ignisFontRendererDestroy();

    ignisDestroy();
}

int onEvent(MinimalApp *app, const MinimalEvent* e)
{
    float w, h;
    if (minimalEventWindowSize(e, &w, &h))
    {
        setViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    switch (minimalEventKeyPressed(e))
    {
    case GLFW_KEY_ESCAPE:    minimalClose(app); break;
    case GLFW_KEY_F6:        minimalToggleVsync(app); break;
    case GLFW_KEY_F7:        minimalToggleDebug(app); break;
    case GLFW_KEY_F9:        view_mode = !view_mode; break;
    case GLFW_KEY_F10:       poly_mode = !poly_mode; break;
    }

    return MINIMAL_OK;
}

void onUpdate(MinimalApp *app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


    if (minimalKeyDown(GLFW_KEY_LEFT))  camera_pos.x -= cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_RIGHT)) camera_pos.x += cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_DOWN))  camera_pos.y += cameraspeed * deltatime;
    if (minimalKeyDown(GLFW_KEY_UP))    camera_pos.y -= cameraspeed * deltatime;

    mat4 model = model = mat4_rotation((vec3) { 0.5f, 1.0f, 0.0f }, (float)glfwGetTime());
    mat4 view = view = mat4_translation(vec3_negate(camera_pos));
    mat4 proj = proj = mat4_perspective(degToRad(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    ignisSetUniformMat4(shader, "proj", proj.v[0]);
    ignisSetUniformMat4(shader, "view", view.v[0]);
    ignisSetUniformMat4(shader, "model", model.v[0]);

    //ignisSetUniform3f(shader, "lightPos", &camera_pos.x);

    ignisUseShader(shader);
    glPolygonMode(GL_FRONT_AND_BACK, poly_mode ? GL_LINE : GL_FILL);

    // use material
    IgnisTexture2D* texture = &robot.materials[0].base_texture;
    ignisBindTexture2D(texture, 0);

    ignisSetUniform1i(shader, "base_texture", 0);
    ignisSetUniform3f(shader, "objectColor", &robot.materials[0].color.r);

    for (int i = 0; i < robot.mesh_count; i++)
    {
        Mesh* mesh = &robot.meshes[i];
        ignisBindVertexArray(&mesh->vao);
        glDrawElements(GL_TRIANGLES, mesh->element_count, GL_UNSIGNED_INT, NULL);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // render debug info
    ignisFontRendererSetProjection(screen_projection.v[0]);

    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", minimalGetFps(app));

    if (app->debug)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6:  Toggle Vsync");
        ignisFontRendererTextFieldLine("F7:  Toggle debug mode");
        ignisFontRendererTextFieldLine("F9:  Toggle view mode");
        ignisFontRendererTextFieldLine("F10: Toggle polygon mode");
    }

    ignisFontRendererFlush();
}

int main()
{
    MinimalApp app = {
        .on_load =    onLoad,
        .on_destroy = onDestroy,
        .on_event =   onEvent,
        .on_update =  onUpdate
    };

    if (minimalLoad(&app, "IgnisApp", 1024, 800, "4.4"))
        minimalRun(&app);

    minimalDestroy(&app);

    return 0;
}

void ignisErrorCallback(ignisErrorLevel level, const char *desc)
{
    switch (level)
    {
    case IGNIS_LVL_WARN:     MINIMAL_WARN("%s", desc); break;
    case IGNIS_LVL_ERROR:    MINIMAL_ERROR("%s", desc); break;
    case IGNIS_LVL_CRITICAL: MINIMAL_CRITICAL("%s", desc); break;
    }
}

void printVersionInfo()
{
    MINIMAL_INFO("[GLFW]   Version:      %s", glfwGetVersionString());
    MINIMAL_INFO("[OpenGL] Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL] Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL] Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL] GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis]  Version:      %s", ignisGetVersionString());
}