#include <Ignis/Ignis.h>

#include <minimal/minimal.h>

#include "iso.h"
#include "camera.h"

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

int show_info = 1;
int show_word_view = 0;

Camera camera;
float cameraspeed = 120.0f;

IgnisFont font;

typedef struct
{
    vec2 position;
    float speed;
} Player;

void renderPlayer(const IsoMap* map, const Player* player)
{
    vec2 pos = worldToScreen(player->position);
    ignisPrimitives2DRenderCircle(pos.x, pos.y, 16, IGNIS_BLACK);
}

static void setViewport(float w, float h)
{
    cameraSetProjectionOrtho(&camera, w, h);
    ignisFontRendererSetProjection(cameraGetProjectionPtr(&camera));
}

IsoMap map;
IgnisTextureAtlas2D tile_texture_atlas;

Player player;

uint32_t tile = UINT32_MAX;

int onLoad(MinimalApp* app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    // ignisSetAllocator(FrostMemoryGetAllocator(), tb_mem_malloc, tb_mem_realloc, tb_mem_free);
    ignisSetLogCallback(ignisLogCallback);

#ifdef _DEBUG
    int debug = 1;
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
    ignisPrimitivesRendererInit();
    ignisFontRendererInit();
    ignisRenderer2DInit();
    ignisBatch2DInit("res/shaders/batch.vert", "res/shaders/batch.frag");

    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    MINIMAL_INFO("[OpenGL]  Version:      %s", ignisGetGLVersion());
    MINIMAL_INFO("[OpenGL]  Vendor:       %s", ignisGetGLVendor());
    MINIMAL_INFO("[OpenGL]  Renderer:     %s", ignisGetGLRenderer());
    MINIMAL_INFO("[OpenGL]  GLSL Version: %s", ignisGetGLSLVersion());
    MINIMAL_INFO("[Ignis]   Version:      %s", ignisGetVersionString());
    MINIMAL_INFO("[Minimal] Version:      %s", minimalGetVersionString());

    ignisCreateTexture2D((IgnisTexture2D*)&tile_texture_atlas, "res/tiles.png", NULL);
    tile_texture_atlas.rows = 1;
    tile_texture_atlas.cols = 4;

    uint32_t grid[] = {
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 1, 1, 2, 1, 1, 1, 1, 1, 3,
        3, 1, 1, 2, 1, 1, 1, 1, 1, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 1, 1, 1, 1, 1, 1, 2, 1, 3,
        3, 1, 1, 1, 1, 1, 1, 1, 1, 3,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3
    };
    isoMapInit(&map, grid, 10, 10, 50, 8.0f);

    player.position = isoMapGetCenter(&map);
    player.speed = 60.0f;

    cameraCreateOrtho(&camera, 0.0f, 0.0f, (float)w, (float)h);
    cameraSetCenterOrtho(&camera, worldToScreen(isoMapGetCenter(&map)));

    setViewport((float)w, (float)h);

    return MINIMAL_OK;
}

void onDestroy(MinimalApp* app)
{
    ignisDeleteFont(&font);

    ignisBatch2DDestroy();
    ignisFontRendererDestroy();
    ignisPrimitivesRendererDestroy();
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
    case MINIMAL_KEY_ESCAPE:    minimalClose(app); break;
    case MINIMAL_KEY_F6:        minimalToggleVsync(app); break;
    case MINIMAL_KEY_F7:        minimalToggleDebug(app); break;
    case MINIMAL_KEY_F9:        show_info = !show_info; break;
    case MINIMAL_KEY_F10:       show_word_view = !show_word_view; break;
    }

    vec2 mouse = { 0 };
    if (minimalEventMouseMoved(e, &mouse.x, &mouse.y))
    {
        tile = getTileIndexAt(&map, screenToWorld(cameraGetMousePos(&camera, mouse)));
    }

    return MINIMAL_OK;
}

void onTick(MinimalApp* app, float deltatime)
{
    // move camera
    vec2 position = camera.position;

    if (minimalKeyDown(MINIMAL_KEY_LEFT))  position.x -= cameraspeed * deltatime;
    if (minimalKeyDown(MINIMAL_KEY_RIGHT)) position.x += cameraspeed * deltatime;
    if (minimalKeyDown(MINIMAL_KEY_DOWN))  position.y += cameraspeed * deltatime;
    if (minimalKeyDown(MINIMAL_KEY_UP))    position.y -= cameraspeed * deltatime;

    cameraSetPositionOrtho(&camera, position);

    // move player
    vec2 velocity;
    velocity.x = (-minimalKeyDown(MINIMAL_KEY_A) + minimalKeyDown(MINIMAL_KEY_D));
    velocity.y = (-minimalKeyDown(MINIMAL_KEY_W) + minimalKeyDown(MINIMAL_KEY_S));

    velocity = vec2_normalize(screenToWorld(velocity));

    player.position = vec2_add(player.position, vec2_mult(velocity, deltatime * player.speed));

    // clear screen
    glClear(GL_COLOR_BUFFER_BIT);

    // render debug info
    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", app->fps);

    if (show_info)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(camera.size.x - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6:  Toggle Vsync");
        ignisFontRendererTextFieldLine("F7:  Toggle debug mode");

        ignisFontRendererTextFieldLine("F9:  Toggle overlay");
        ignisFontRendererTextFieldLine("F10: Toggle view mode");
    }

    ignisFontRendererFlush();

    ignisBatch2DSetViewProjection(cameraGetViewProjectionPtr(&camera));

    renderMap(&map, &tile_texture_atlas);

    ignisBatch2DFlush();

    ignisPrimitivesRendererSetViewProjection(cameraGetViewProjectionPtr(&camera));

    renderPlayer(&map, &player);
    highlightTile(&map, tile);

    if (show_word_view)
    {
        ignisPrimitivesRendererFlush();
        ignisPrimitivesRendererSetViewProjection(cameraGetProjectionPtr(&camera));

        vec2 mouse = { 0 };
        minimalCursorPos(&mouse.x, &mouse.y);

        vec2 mouse_world = screenToWorld(cameraGetMousePos(&camera, mouse));

        ignisPrimitives2DFillCircle(mouse_world.x, mouse_world.y, 3, IGNIS_BLUE);

        highlightTileWorld(&map, tile);
    }

    ignisPrimitivesRendererFlush();
}

int main()
{
    MinimalApp app = { 
        .on_load =    onLoad,
        .on_destroy = onDestroy,
        .on_event =   onEvent,
        .on_tick =  onTick
    };

    if (minimalLoad(&app, "IgnisApp", 1200, 800, "4.4"))
        minimalRun(&app);

    minimalDestroy(&app);

    return 0;
}