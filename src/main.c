#include <Ignis/Ignis.h>

#include "Minimal/Application.h"
#include "Graphics/Renderer.h"
#include "math/math.h"

static void IgnisErrorCallback(ignisErrorLevel level, const char* desc)
{
    switch (level)
    {
    case IGNIS_WARN:     MINIMAL_WARN("%s", desc); break;
    case IGNIS_ERROR:    MINIMAL_ERROR("%s", desc); break;
    case IGNIS_CRITICAL: MINIMAL_CRITICAL("%s", desc); break;
    }
}

int show_info = 0;

float width, height;
mat4 screen_projection;

IgnisFont font;

static void SetViewport(float w, float h)
{
    width = w;
    height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
}


int OnLoad(MinimalApp* app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    // ignisSetAllocator(FrostMemoryGetAllocator(), tb_mem_malloc, tb_mem_realloc, tb_mem_free);
    ignisSetErrorCallback(IgnisErrorCallback);

    int debug = 0;
#ifdef _DEBUG
    debug = 1;
#endif

    if (!ignisInit(debug))
    {
        MINIMAL_ERROR("[IGNIS] Failed to initialize Ignis");
        return MINIMAL_FAIL;
    }

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    /* renderer */
    Renderer2DInit();
    Primitives2DInit();
    FontRendererInit();

    SetViewport((float)w, (float)h);

    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    FontRendererBindFontColor(&font, IGNIS_WHITE);

    MINIMAL_INFO("[GLFW] Version: %s", glfwGetVersionString());
    MINIMAL_INFO("[OpenGL] Version: %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL] Vendor: %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL] Renderer: %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL] GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis] Version: %s", ignisGetVersionString());

    return MINIMAL_OK;
}

void OnDestroy(MinimalApp* app)
{
    ignisDeleteFont(&font);

    FontRendererDestroy();
    Primitives2DDestroy();
    Renderer2DDestroy();
}

int OnEvent(MinimalApp* app, const MinimalEvent* e)
{
    float w, h;
    if (MinimalEventWindowSize(e, &w, &h))
    {
        SetViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    switch (MinimalEventKeyPressed(e))
    {
    case GLFW_KEY_ESCAPE:    MinimalClose(app); break;
    case GLFW_KEY_F6:        MinimalToggleVsync(app); break;
    case GLFW_KEY_F7:        MinimalToggleDebug(app); break;
    case GLFW_KEY_F9:        show_info = !show_info; break;
    }

    return MINIMAL_OK;
}

void OnUpdate(MinimalApp* app, float deltatime)
{
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // render debug info
    FontRendererStart(screen_projection.v);

    /* fps */
    FontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", MinimalGetFps(app));

    if (show_info)
    {
        /* Settings */
        FontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        FontRendererTextFieldLine("F6: Toggle Vsync");
        FontRendererTextFieldLine("F7: Toggle debug mode");

        FontRendererTextFieldLine("F9: Toggle overlay");
    }

    FontRendererFlush();
}

int main()
{
    MinimalApp app = { 
        .on_load = OnLoad,
        .on_destroy = OnDestroy,
        .on_event = OnEvent,
        .on_update = OnUpdate
    };

    if (MinimalLoad(&app, "IgnisApp", 1024, 800, "4.4"))
        MinimalRun(&app);

    MinimalDestroy(&app);

    return 0;
}