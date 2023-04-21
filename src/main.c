#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

#include "model.h"

static void ignisErrorCallback(ignisErrorLevel level, const char *desc);
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

IgnisShader shader;
Model robot = { 0 };

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

    ignisPrimitivesRendererInit();

    setViewport((float)w, (float)h);

    /* shader */
    shader = ignisCreateShadervf("res/shaders/shader.vert", "res/shaders/shader.frag");

    /* gltf model */
    //loadModel(&robot, "res/models/", "Box.gltf");
    //loadModel(&robot, "res/models/walking_robot", "scene.gltf");
    loadModel(&robot, "res/models/", "RiggedSimple.gltf");
    //loadModel(&robot, "res/models/", "RiggedFigure.gltf");
    //loadModel(&robot, "res/models/", "BoxAnimated.gltf");

    for (int i = 0; i < robot.mesh_count; i++) uploadMesh(&robot.meshes[i]);

    return MINIMAL_OK;
}

void onDestroy(MinimalApp *app)
{
    destroyModel(&robot);
    ignisDeleteShader(shader);
    
    ignisDeleteFont(&font);

    ignisFontRendererDestroy();
    ignisPrimitivesRendererDestroy();

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

    if (minimalKeyDown(GLFW_KEY_LEFT))  camera_rotation -= camera_speed * deltatime;
    if (minimalKeyDown(GLFW_KEY_RIGHT)) camera_rotation += camera_speed * deltatime;
    if (minimalKeyDown(GLFW_KEY_DOWN))  camera_radius += camera_zoom * deltatime;
    if (minimalKeyDown(GLFW_KEY_UP))    camera_radius -= camera_zoom * deltatime;

    //mat4 model = mat4_rotation((vec3) { 0.5f, 1.0f, 0.0f }, (float)glfwGetTime());
    //mat4 view = mat4_translation(vec3_negate(camera_pos));
    mat4 proj = mat4_perspective(degToRad(45.0f), (float)width / (float)height, 0.1f, 100.0f);

    vec3 eye = {
        sinf(camera_rotation) * camera_radius,
        cosf(camera_rotation) * camera_radius,
        0.0f,
    };
    vec3 look_at = { 0.0f, 0.0f, 0.0f };
    vec3 up = { 0.0f, 0.0f, 1.0f };
    mat4 view = mat4_look_at(eye, look_at, up);
    mat4 model = mat4_identity();

    ignisSetUniformMat4(shader, "proj", 1, proj.v[0]);
    ignisSetUniformMat4(shader, "view", 1, view.v[0]);
    ignisSetUniformMat4(shader, "model", 1, model.v[0]);

    //ignisSetUniform3f(shader, "lightPos", &camera_pos.x);

    ignisUseShader(shader);
    glPolygonMode(GL_FRONT_AND_BACK, poly_mode ? GL_LINE : GL_FILL);

    renderModel(&robot, shader);

    mat4 view_proj = mat4_multiply(proj, view);
    ignisPrimitivesRendererSetViewProjection(view_proj.v[0]);

    for (int i = 0; i < robot.mesh_count; ++i)
    {
        Mesh* mesh = &robot.meshes[i];
        ignisPrimitives3DRenderBox(&mesh->min.x, &mesh->max.x, IGNIS_WHITE);
    }

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