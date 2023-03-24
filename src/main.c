#include <ignis/ignis.h>

#include <minimal/application.h>

#include "math/math.h"

static void ignisErrorCallback(ignisErrorLevel level, const char* desc);
static void printVersionInfo();

IgnisFont font;

float width, height;
mat4 screen_projection;

static void setViewport(float w, float h)
{
    width = w;
    height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
}

int onLoad(MinimalApp* app, uint32_t w, uint32_t h)
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

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    /* renderer */
    ignisRenderer2DInit();
    ignisPrimitives2DInit();
    ignisFontRendererInit();
    ignisBatch2DInit("res/shaders/batch.vert", "res/shaders/batch.frag");

    setViewport((float)w, (float)h);

    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    printVersionInfo();

    return MINIMAL_OK;
}

void onDestroy(MinimalApp* app)
{
    ignisDeleteFont(&font);

    ignisBatch2DDestroy();
    ignisFontRendererDestroy();
    ignisPrimitives2DDestroy();
    ignisRenderer2DDestroy();
}

int onEvent(MinimalApp* app, const MinimalEvent* e)
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
    }

    return MINIMAL_OK;
}

void onUpdate(MinimalApp* app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // render debug info
    ignisFontRendererSetProjection(screen_projection.v);

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

void ignisErrorCallback(ignisErrorLevel level, const char* desc)
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