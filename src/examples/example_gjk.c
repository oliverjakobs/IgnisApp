#include "examples.h"

#include "gjk.h"

IgnisFont font;

float screen_width, screen_height;
mat4 screen_projection;

float width, height;
mat4 view;

gjk_vec2 center;

static void setViewport(float w, float h)
{
    screen_width = w;
    screen_height = h;
    screen_projection = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);

    width = w / 100.0f;
    height = h / 100.0f;
    view = mat4_ortho(-width * .5f, width * .5f, -height * .5f, height * .5f, -1.0f, 1.0f);

    center.x = width * 0.5f;
    center.y = height * 0.5f;
}

gjk_vec2 mouse;

gjk_shape circle;
gjk_shape poly;
gjk_shape triangle;
gjk_vec2 simplex[3];

uint8_t collision = 0;

gjk_vec2 triangle_verts[] =
{
    { 2.0f, 4.0f },
    { 1.2f, 2.0f },
    { 3.0f, 1.0f }
};

/*
gjk_vec2 poly_verts[] =
{
    { 400.0f, 300.0f },
    { 500.0f, 260.0f },
    { 600.0f, 300.0f },
    { 620.0f, 500.0f },
    { 480.0f, 500.0f }
};
*/

gjk_vec2 poly_verts[] =
{
    {  2.0f,  0.0f },
    {  1.0f,  1.75f },
    { -1.0f,  1.75f },
    { -2.0f,  0.0f },
    { -1.0f, -1.75f },
    {  1.0f, -1.75f }
};

gjk_vec2 GetMousePos()
{
    gjk_vec2 pos = { 0 };
    minimalCursorPos(&pos.x, &pos.y);

    pos.x = (pos.x / 100.0f) - width * .5f;
    pos.y = height - (pos.y / 100.0f) - height * .5f;

    return pos;
}

void RenderPoint(gjk_vec2 pos, IgnisColorRGBA color)
{
    ignisPrimitives2DRenderCircle(pos.x, pos.y, .02f, color);
}

void RenderCircle(gjk_shape* circle, IgnisColorRGBA color)
{
    ignisPrimitives2DRenderCircle(circle->center.x, circle->center.y, circle->radius, color);
    RenderPoint(circle->center, color);
}

void RenderPoly(gjk_shape* shape, IgnisColorRGBA color)
{
    ignisPrimitives2DRenderPoly((float*)shape->vertices, shape->count * 2, 0, 0, color);
    RenderPoint(shape->center, color);
}

int onLoadGJK(MinimalApp* app, uint32_t w, uint32_t h)
{
    /* ingis initialization */
    initIgnis();

    minimalEnableDebug(app, 1);

    ignisEnableBlend(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    ignisSetClearColor(IGNIS_DARK_GREY);

    /* font renderer */
    ignisCreateFont(&font, "res/fonts/ProggyTiny.ttf", 24.0);
    ignisFontRendererInit();
    ignisFontRendererBindFontColor(&font, IGNIS_WHITE);

    ignisPrimitivesRendererInit();

    setViewport((float)w, (float)h);

    gjk_circle(&circle, center, 4.0f);
    gjk_poly(&triangle, triangle_verts, 3);
    gjk_poly(&poly, poly_verts, 6);

    return 1;
}

void onDestroyGJK(MinimalApp* app)
{
    ignisDeleteFont(&font);

    ignisFontRendererDestroy();
    ignisPrimitivesRendererDestroy();
}

int onEventGJK(MinimalApp* app, const MinimalEvent* e)
{
    float w, h;
    if (minimalEventWindowSize(e, &w, &h))
    {
        setViewport(w, h);
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    }

    return onEventDefault(app, e);
}

void onTickGJK(MinimalApp* app, float deltatime)
{
    mouse = GetMousePos();
    gjk_set_center(&triangle, mouse);

    collision = gjk_collision(&triangle, &poly, simplex);


    // clear screen
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    ignisPrimitivesRendererSetViewProjection(view.v[0]);

    ignisPrimitives2DRenderLine(center.x, center.y, mouse.x, mouse.y, IGNIS_WHITE);

    RenderPoly(&poly, IGNIS_WHITE);
    RenderPoint(gjk_furthest_point(&poly, mouse), IGNIS_WHITE);

    RenderPoly(&triangle, collision ? IGNIS_RED : IGNIS_WHITE);
    RenderPoint(gjk_furthest_point(&triangle, mouse), IGNIS_WHITE);

    if (collision)
    {
        ignisPrimitives2DRenderPoly((float*)simplex, 6, 0, 0, IGNIS_GREEN);

        epa_edge edge;
        epa_closest_edge(simplex, 3, &edge);

        ignisPrimitives2DRenderLine(edge.p.x, edge.p.y, edge.q.x, edge.q.y, IGNIS_BLUE);

        gjk_vec2 n;
        float d = epa(&triangle, &poly, simplex, &n);
        //Primitives2DRenderLineDir(triangle.center.x, triangle.center.y, n.x, n.y, d, IGNIS_RED);
    }

    ignisPrimitivesRendererFlush();

    // render debug info
    ignisFontRendererSetProjection(screen_projection.v[0]);

    /* fps */
    ignisFontRendererRenderTextFormat(8.0f, 8.0f, "FPS: %d", app->fps);

    if (app->debug)
    {
        /* Settings */
        ignisFontRendererTextFieldBegin(width - 220.0f, 8.0f, 8.0f);

        ignisFontRendererTextFieldLine("F6: Toggle Vsync");
        ignisFontRendererTextFieldLine("F7: Toggle debug mode");
    }

    ignisFontRendererFlush();
}


MinimalApp example_gjk()
{
    return (MinimalApp) {
        .on_load = onLoadGJK,
        .on_destroy = onDestroyGJK,
        .on_event = onEventGJK,
        .on_tick = onTickGJK
    };
}