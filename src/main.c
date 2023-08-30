#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

static void ignisLogCallback(IgnisLogLevel level, const char *desc);
static void printVersionInfo();

IgnisFont font;

float width, height;
mat4 screen_projection;

const float vertices[] = {
    -0.5f, -0.5f, -0.5f,
     0.5f, -0.5f, -0.5f,
     0.5f,  0.5f, -0.5f,
    -0.5f,  0.5f, -0.5f,
    -0.5f, -0.5f,  0.5f,
     0.5f, -0.5f,  0.5f,
     0.5f,  0.5f,  0.5f,
    -0.5f,  0.5f,  0.5f
};

const GLuint indices[] = {
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4,
    7, 3, 0, 0, 4, 7,
    6, 2, 1, 1, 5, 6,
    0, 1, 5, 5, 4, 0,
    3, 2, 6, 6, 7, 3
};
size_t element_count = 36;

IgnisShader shader;
IgnisVertexArray vao;

static void setViewport(float w, float h)
{
    width = w;
    height = h;
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

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    glEnable(GL_DEPTH_TEST);

    /* font renderer */
    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererInit();
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    setViewport((float)w, (float)h);

    /* cube */
    ignisGenerateVertexArray(&vao, 2);

    ignisLoadArrayBuffer(&vao, 0, sizeof(vertices), vertices, IGNIS_STATIC_DRAW);
    ignisLoadElementBuffer(&vao, 1, indices, element_count, IGNIS_STATIC_DRAW);

    IgnisBufferElement layout[] = {
        { GL_FLOAT, 3, GL_FALSE }
    };
    ignisSetVertexLayout(&vao, 0, layout, 1);


    /* shader */
    shader = ignisCreateShadervf("res/shaders/shader.vert", "res/shaders/shader.frag");

    printVersionInfo();

    return MINIMAL_OK;
}

void onDestroy(MinimalApp *app)
{
    ignisDeleteVertexArray(&vao);
    ignisDeleteShader(shader);

    ignisDeleteFont(&font);

    ignisFontRendererDestroy();
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
    case MINIMAL_KEY_ESCAPE:    minimalClose(app); break;
    case MINIMAL_KEY_F6:        minimalToggleVsync(app); break;
    case MINIMAL_KEY_F7:        minimalToggleDebug(app); break;
    }

    return MINIMAL_OK;
}

void onUpdate(MinimalApp *app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    vec3 camera_pos = (vec3){ 0.0f, 0.0f, 3.0f };

    // create transformations
    mat4 model = mat4_rotation((vec3) { 0.5f, 1.0f, 0.0f }, (float)app->timer.lastframe);
    mat4 view = mat4_translation(vec3_negate(camera_pos));
    mat4 proj = mat4_perspective(degToRad(45.0f), width / height, 0.1f, 100.0f);

    ignisSetUniform3f(shader, "lightPos", 1, &camera_pos.x);

    ignisSetUniformMat4(shader, "proj", 1, proj.v[0]);
    ignisSetUniformMat4(shader, "view", 1, view.v[0]);
    ignisSetUniformMat4(shader, "model", 1, model.v[0]);

    ignisUseShader(shader);

    ignisBindVertexArray(&vao);
    glDrawElements(GL_TRIANGLES, (GLsizei)element_count, GL_UNSIGNED_INT, NULL);

    // render debug info
    ignisFontRendererSetProjection(screen_projection.v[0]);

    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", minimalGetFps(app));

    if (app->debug)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6: Toggle Vsync");
        ignisFontRendererTextFieldLine("F7: Toggle debug mode");
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

void ignisLogCallback(IgnisLogLevel level, const char* desc)
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