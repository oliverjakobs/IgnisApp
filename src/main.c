#include <Ignis/Ignis.h>
#include <minimal/minimal.h>

#include "math/mat4.h"

#include "nuklear_glfw_gl3.h"
#include "demo/demo.h"

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

float width, height;
mat4 screen_projection;

static void setViewport(float w, float h)
{
    width = w;
    height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
}

struct nk_glfw glfw;
struct nk_colorf bg;

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

    MINIMAL_INFO("[OpenGL]  Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL]  Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL]  Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL]  GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis]   Version:      %s", ignisGetVersionString());
    MINIMAL_INFO("[Minimal] Version:      %s", minimalGetVersionString());

    setViewport((float)w, (float)h);

    nk_glfw3_init(&glfw, app->window);
    glfw.ctx.input.handle = &app->input;

    nk_glfw3_load_font_atlas(&glfw);

    return MINIMAL_OK;
}

void onDestroy(MinimalApp* app)
{
    nk_glfw3_shutdown(&glfw);

    ignisRenderer2DDestroy();
}

int onEvent(MinimalApp* app, const MinimalEvent* e)
{
    float w = 0, h = 0;
    if (minimalEventWindowSize(e, &w, &h))
    {
        setViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    if (minimalEventIsType(e, MINIMAL_EVENT_CHAR))
    {
        if (glfw.text_len < NK_GLFW_TEXT_MAX)
            glfw.text[glfw.text_len++] = minimalEventChar(e);
    }

    float x = 0, y = 0;
    if (minimalEventMouseScrolled(e, &x, &y))
    {
        glfw.scroll.x += x;
        glfw.scroll.y += y;
    }

    /*
    // TODO: fix minimal not registering mouse release if moved out of window while pressed
    if (minimalEventMouseButtonReleased(e, MINIMAL_MOUSE_BUTTON_1, NULL, NULL))
        MINIMAL_INFO("Released");
    */

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
    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    nk_glfw3_new_frame(&glfw);

    /* GUI */
    struct nk_context* ctx = &glfw.ctx;
    if (nk_begin(ctx, "Demo", nk_rect(50, 50, 230, 250),
        NK_WINDOW_BORDER | NK_WINDOW_MOVABLE | NK_WINDOW_SCALABLE |
        NK_WINDOW_MINIMIZABLE | NK_WINDOW_TITLE))
    {
        enum { EASY, HARD };
        static int op = EASY;
        static int property = 20;
        static char text[64];
        static int text_len = 0;
        nk_layout_row_static(ctx, 30, 80, 1);
        if (nk_button_label(ctx, "button"))
            fprintf(stdout, "button pressed\n");

        nk_layout_row_dynamic(ctx, 30, 2);
        if (nk_option_label(ctx, "easy", op == EASY)) op = EASY;
        if (nk_option_label(ctx, "hard", op == HARD)) op = HARD;

        nk_layout_row_dynamic(ctx, 25, 1);
        nk_property_int(ctx, "Compression:", 0, &property, 100, 10, 1);

        nk_layout_row_dynamic(ctx, 25, 2);
        nk_label(ctx, "Text:", NK_TEXT_LEFT);
        nk_edit_string(ctx, NK_EDIT_SIMPLE, text, &text_len, 64, nk_filter_default);

        nk_layout_row_dynamic(ctx, 20, 1);
        nk_label(ctx, "background:", NK_TEXT_LEFT);
        nk_layout_row_dynamic(ctx, 25, 1);
        if (nk_combo_begin_color(ctx, nk_rgb_cf(bg), nk_vec2(nk_widget_width(ctx), 400))) {
            nk_layout_row_dynamic(ctx, 120, 1);
            bg = nk_color_picker(ctx, bg, NK_RGBA);
            nk_layout_row_dynamic(ctx, 25, 1);
            bg.r = nk_propertyf(ctx, "#R:", 0, bg.r, 1.0f, 0.01f, 0.005f);
            bg.g = nk_propertyf(ctx, "#G:", 0, bg.g, 1.0f, 0.01f, 0.005f);
            bg.b = nk_propertyf(ctx, "#B:", 0, bg.b, 1.0f, 0.01f, 0.005f);
            bg.a = nk_propertyf(ctx, "#A:", 0, bg.a, 1.0f, 0.01f, 0.005f);
            nk_combo_end(ctx);
        }
    }
    nk_end(ctx);

    overview(ctx);

    nk_glfw3_render(&glfw, NK_ANTI_ALIASING_ON);
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
