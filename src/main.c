#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

#include "model.h"

static void ignisLogCallback(ignisLogLevel level, const char *desc);
static void printVersionInfo();

IgnisFont font;

float width, height;
mat4 screen_projection;
int view_mode = 0;
int poly_mode = 0;

float camera_rotation = 0.0f;
float camera_radius = 16.0f;
float camera_speed = 1.0f;
float camera_zoom = 4.0f;

IgnisShader shader_model;
IgnisShader shader_skinned;

Model model = { 0 };
Animation* animations = NULL;
size_t animation_count = 0;
size_t animation_index = 0;

int paused = 0;

void loadGLTF(const char* dir, const char* filename)
{
    size_t size = 0;
    const char* path = ignisTextFormat("%s/%s", dir, filename);
    char* filedata = ignisReadFile(path, &size);

    if (!filedata) return;

    cgltf_options options = { 0 };
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse(&options, filedata, size, &data);
    if (result != cgltf_result_success)
    {
        IGNIS_ERROR("MODEL: [%s] Failed to load glTF data", path);
        free(filedata);
        return;
    }

    MINIMAL_INFO("    > Meshes count: %i", data->meshes_count);
    MINIMAL_INFO("    > Materials count: %i", data->materials_count);
    MINIMAL_INFO("    > Buffers count: %i", data->buffers_count);
    MINIMAL_INFO("    > Images count: %i", data->images_count);
    MINIMAL_INFO("    > Textures count: %i", data->textures_count);

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success)
    {
        IGNIS_ERROR("MODEL: [%s] Failed to load mesh/material buffers", path);
        cgltf_free(data);
        free(filedata);
        return;
    }

    loadModelGLTF(&model, data, dir);
    animations = loadAnimationsGLTF(data, &animation_count);

    cgltf_free(data);
    free(filedata);
}

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
    ignisSetLogCallback(ignisLogCallback);

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

    ignisEnableBlend(IGNIS_SRC_ALPHA, IGNIS_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    glEnable(GL_DEPTH_TEST);

    /* font renderer */
    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererInit();
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    ignisPrimitivesRendererInit();

    setViewport((float)w, (float)h);

    /* shader */
    shader_model = ignisCreateShadervf("res/shaders/model.vert", "res/shaders/model.frag");
    shader_skinned = ignisCreateShadervf("res/shaders/skinned.vert", "res/shaders/model.frag");

    /* gltf model */
    //loadModelGLTF(&model, &animation, "res/models/", "Box.gltf");
    //loadModelGLTF(&model, &animation, "res/models/walking_robot", "scene.gltf");
    //loadModelGLTF(&model, &animation, "res/models/robot", "scene.gltf");
    //loadModelGLTF(&model, &animation, "res/models/", "RiggedSimple.gltf");
    //loadModelGLTF(&model, &animation, "res/models/", "RiggedFigure.gltf");
    //loadModelGLTF(&model, &animation, "res/models/", "BoxAnimated.gltf");
    //loadModelGLTF(&model, &animation, "res/models/", "CesiumMilkTruck.gltf");
    loadGLTF("res/models/", "Fox.glb");

    for (int i = 0; i < model.mesh_count; i++) uploadMesh(&model.meshes[i]);

    return MINIMAL_OK;
}

void onDestroy(MinimalApp *app)
{
    destroyModel(&model);
    for (size_t i = 0; i < animation_count; ++i)
        destroyAnimation(&animations[i]);

    ignisDeleteShader(shader_model);
    ignisDeleteShader(shader_skinned);
    
    ignisDeleteFont(&font);

    ignisFontRendererDestroy();
    ignisPrimitivesRendererDestroy();

    ignisDestroy();
}

int onEvent(MinimalApp *app, const MinimalEvent *e)
{
    float w, h;
    if (minimalEventWindowSize(e, &w, &h))
    {
        setViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    switch (minimalEventKeyPressed(e))
    {
    case MINIMAL_KEY_ESCAPE:   minimalClose(app); break;
    case MINIMAL_KEY_F6:       minimalToggleVsync(app); break;
    case MINIMAL_KEY_F7:       minimalToggleDebug(app); break;
    case MINIMAL_KEY_F9:       view_mode = !view_mode; break;
    case MINIMAL_KEY_F10:      poly_mode = !poly_mode; break;
    case MINIMAL_KEY_SPACE:    paused = !paused; break;

    case MINIMAL_KEY_1: if (animation_count >= 0) animation_index = 0; break;
    case MINIMAL_KEY_2: if (animation_count >= 1) animation_index = 1; break;
    case MINIMAL_KEY_3: if (animation_count >= 2) animation_index = 2; break;
    case MINIMAL_KEY_4: if (animation_count >= 3) animation_index = 3; break;
    }

    return MINIMAL_OK;
}

void onUpdate(MinimalApp *app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (minimalKeyDown(MINIMAL_KEY_LEFT))  camera_rotation -= camera_speed * deltatime;
    if (minimalKeyDown(MINIMAL_KEY_RIGHT)) camera_rotation += camera_speed * deltatime;
    if (minimalKeyDown(MINIMAL_KEY_DOWN))  camera_radius += camera_zoom * deltatime;
    if (minimalKeyDown(MINIMAL_KEY_UP))    camera_radius -= camera_zoom * deltatime;

    if (!paused)
    {
        tickAnimation(&animations[animation_index], deltatime);
    }

    //mat4 model = mat4_rotation((vec3) { 0.5f, 1.0f, 0.0f }, (float)glfwGetTime());
    //mat4 view = mat4_translation(vec3_negate(camera_pos));
    mat4 proj = mat4_perspective(degToRad(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    vec3 eye = {
        sinf(camera_rotation) * camera_radius,
        0.0f,
        cosf(camera_rotation) * camera_radius,
    };
    vec3 look_at = { 0.0f, 0.0f, 0.0f };
    vec3 up = { 0.0f, 1.0f, 0.0f };
    mat4 view = mat4_look_at(eye, look_at, up);

    glPolygonMode(GL_FRONT_AND_BACK, poly_mode ? GL_LINE : GL_FILL);

    if (model.joint_count)
    {
        ignisSetUniformMat4(shader_skinned, "proj", 1, proj.v[0]);
        ignisSetUniformMat4(shader_skinned, "view", 1, view.v[0]);
        renderModelSkinned(&model, &animations[animation_index], shader_skinned);
    }
    else
    {
        ignisSetUniformMat4(shader_model, "proj", 1, proj.v[0]);
        ignisSetUniformMat4(shader_model, "view", 1, view.v[0]);
        renderModel(&model, &animations[animation_index], shader_model);
    }

    mat4 view_proj = mat4_multiply(proj, view);
    ignisPrimitivesRendererSetViewProjection(view_proj.v[0]);

    /*
    for (int i = 0; i < model.mesh_count; ++i)
    {
        Mesh* mesh = &model.meshes[i];
        ignisPrimitives3DRenderBox(&mesh->min.x, &mesh->max.x, IGNIS_WHITE);
    }
    */

    ignisPrimitivesRendererFlush();

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

    ignisFontRendererTextFieldBegin(8.0f, 30.0f, 6.0f);

    ignisFontRendererTextFieldLine("Current animation   %d",    animation_index);
    ignisFontRendererTextFieldLine("Animation Duration: %4.2f", animations[animation_index].duration);
    ignisFontRendererTextFieldLine("Animation Time:     %4.2f", animations[animation_index].time);

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

void ignisLogCallback(ignisLogLevel level, const char *desc)
{
    switch (level)
    {
    case IGNIS_LOG_TRACE:    MINIMAL_TRACE("%s", desc); break;
    case IGNIS_LOC_INFO:     MINIMAL_INFO("%s", desc); break;
    case IGNIS_LOG_WARN:     MINIMAL_WARN("%s", desc); break;
    case IGNIS_LOG_ERROR:    MINIMAL_ERROR("%s", desc); break;
    case IGNIS_LOG_CRITICAL: MINIMAL_CRITICAL("%s", desc); break;
    }
}

void printVersionInfo()
{
    MINIMAL_INFO("[OpenGL]  Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL]  Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL]  Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL]  GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis]   Version:      %s", ignisGetVersionString());
    MINIMAL_INFO("[Minimal] Version:      %s", minimalGetVersionString());
}