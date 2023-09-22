#include <Ignis/Ignis.h>

#include <minimal/minimal.h>

#include "math/mat4.h"

#include "watcher.h"
#include "resources.h"

#include "toolbox/tb_file.h"

static void ignisLogCallback(IgnisLogLevel level, const char* desc)
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

IgnisFont font;

float width, height;
mat4 screen_projection;

Resources res;

Watcher* watcher;

const IgnisTexture2D* texture;

static void setViewport(float w, float h)
{
    width = w;
    height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
}

int onLoad(MinimalApp* app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    ignisSetLogCallback(ignisLogCallback);

    int debug = 0;
#ifdef _DEBUG
    debug = 1;
    minimalEnableDebug(app, debug);
#endif

    if (!ignisInit(minimalGetGLProcAddress, debug))
    {
        MINIMAL_ERROR("[IGNIS] Failed to initialize Ignis");
        return MINIMAL_FAIL;
    }

    ignisEnableBlend(IGNIS_SRC_ALPHA, IGNIS_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    /* renderer */
    ignisRenderer2DInit();
    ignisFontRendererInit();
    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    MINIMAL_INFO("[OpenGL]  Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL]  Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL]  Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL]  GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis]   Version:      %s", ignisGetVersionString());
    MINIMAL_INFO("[Minimal] Version:      %s", minimalGetVersionString());

    setViewport((float)w, (float)h);

    ResourcesInit(&res);
    watcher = watcherCreate("./res");

    texture = ResourcesLoadTexture2D(&res, "./res/texture.png");

    return MINIMAL_OK;
}

void onDestroy(MinimalApp* app)
{
    watcherDestroy(watcher);
    ResourcesDestroy(&res);

    ignisDeleteFont(&font);

    ignisFontRendererDestroy();
    ignisRenderer2DDestroy();
}

int onEvent(MinimalApp* app, const MinimalEvent* e)
{
    if (minimalEventIsType(e, WATCHER_EVENT))
    {
        const WatcherEvent* watcher_event = minimalExternalEvent(e);

        const char* dir = "./res";
        char name_buffer[WATCHER_BUFFER_SIZE] = {0};
        size_t size = tb_path_join(name_buffer, WATCHER_BUFFER_SIZE, dir, watcher_event->filename);

        switch (watcher_event->action)
        {
        case WATCHER_ACTION_ADDED:
            printf("       Added: %.*s\n", (uint32_t)size, name_buffer);
            break;
        case WATCHER_ACTION_REMOVED:
            printf("     Removed: %.*s\n", (uint32_t)size, name_buffer);
            break;
        case WATCHER_ACTION_MODIFIED:
            printf("    Modified: %.*s\n", (uint32_t)size, name_buffer);
            texture = ResourcesReloadTexture2D(&res, texture, name_buffer);
            break;
        default:
            printf("Unknown action!\n");
            break;
        }
    }

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

void onTick(MinimalApp* app, float deltatime)
{
    watcherPollEvents(app, watcher);

    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    ignisRenderer2DSetViewProjection(screen_projection.v[0]);
    
    ignisRenderer2DRenderTexture(texture, (width - texture->width) * .5f, (height - texture->height) * .5f);

    // render debug info
    ignisFontRendererSetProjection(screen_projection.v[0]);

    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", app->fps);

    if (app->debug)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6:  Toggle Vsync");
        ignisFontRendererTextFieldLine("F7:  Toggle debug mode");
    }

    ignisFontRendererFlush();
}

int main()
{
    MinimalApp app = { 
        .on_load =    onLoad,
        .on_destroy = onDestroy,
        .on_event =   onEvent,
        .on_tick =    onTick
    };

    if (minimalLoad(&app, "IgnisApp", 1200, 800, "4.4"))
        minimalRun(&app);

    minimalDestroy(&app);

    return 0;
}