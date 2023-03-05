#include "Renderer.h"

#include <math.h>

/* ---------------------| shader |---------------------------------------------*/
static const char* vert_src = "#version 330 core \n     \
layout(location = 0) in vec2 a_Pos;                     \
layout(location = 1) in vec4 a_Color;                   \
uniform mat4 u_ViewProj;                                \
out vec4 v_Color;                                       \
void main()                                             \
{                                                       \
    gl_Position = u_ViewProj * vec4(a_Pos, 0.0, 1.0);   \
    v_Color = a_Color;                                  \
}";

static const char* frag_src = "#version 330 core \n\
layout(location = 0) out vec4 f_Color;      \
in vec4 v_Color;                            \
void main()                                 \
{                                           \
    f_Color = v_Color;                      \
}";
/* ---------------------| !shader |--------------------------------------------*/

typedef struct
{
    IgnisVertexArray vao;
    IgnisShader shader;
    GLint uniform_location_view_proj;

    float* vertices;
    GLsizei vertex_count;
} PrimitivesData;

static int PrimitivesDataCreate(PrimitivesData* data, size_t size, IgnisBufferElement* layout, size_t layout_size)
{
    ignisGenerateVertexArray(&data->vao);
    ignisAddArrayBufferLayout(&data->vao, size * sizeof(float), NULL, GL_DYNAMIC_DRAW, 0, layout, layout_size);

    data->vertices = ignisMalloc(size * sizeof(float));
    data->vertex_count = 0;

    ignisCreateShaderSrcvf(&data->shader, vert_src, frag_src);
    data->uniform_location_view_proj = ignisGetUniformLocation(&data->shader, "u_ViewProj");

    return 1;
}

static void PrimitivesDataDestroy(PrimitivesData* data)
{
    ignisDeleteShader(&data->shader);
    ignisDeleteVertexArray(&data->vao);

    ignisFree(data->vertices);
}

static void PrimitivesDataFlush(PrimitivesData* data, GLenum mode)
{
    if (data->vertex_count == 0) return;

    ignisBindVertexArray(&data->vao);
    ignisBufferSubData(&data->vao.array_buffers[0], 0, data->vertex_count * sizeof(float), data->vertices);

    ignisUseShader(&data->shader);

    glDrawArrays(mode, 0, data->vertex_count / PRIMITIVES2D_VERTEX_SIZE);

    data->vertex_count = 0;
}

static void PrimitivesPushVertex(PrimitivesData* data, float x, float y, IgnisColorRGBA color)
{
    data->vertices[data->vertex_count++] = x;
    data->vertices[data->vertex_count++] = y;

    data->vertices[data->vertex_count++] = color.r;
    data->vertices[data->vertex_count++] = color.g;
    data->vertices[data->vertex_count++] = color.b;
    data->vertices[data->vertex_count++] = color.a;
}

static PrimitivesData _lines;
static PrimitivesData _triangles;

void Primitives2DInit()
{
    IgnisBufferElement layout[] = { {GL_FLOAT, 2, GL_FALSE}, {GL_FLOAT, 4, GL_FALSE} };
    size_t layout_size = 2;

    PrimitivesDataCreate(&_lines, PRIMITIVES2D_LINES_BUFFER_SIZE, layout, layout_size);
    PrimitivesDataCreate(&_triangles, PRIMITIVES2D_TRIANGLES_BUFFER_SIZE, layout, layout_size);
}

void Primitives2DDestroy()
{
    PrimitivesDataDestroy(&_lines);
    PrimitivesDataDestroy(&_triangles);
}

void Primitives2DStart(const float* mat_view_proj)
{
    ignisSetUniformMat4l(&_lines.shader, _lines.uniform_location_view_proj, mat_view_proj);
    ignisSetUniformMat4l(&_triangles.shader, _triangles.uniform_location_view_proj, mat_view_proj);
}

void Primitives2DFlush()
{
    PrimitivesDataFlush(&_lines, GL_LINES);
    PrimitivesDataFlush(&_triangles, GL_TRIANGLES);
}

void Primitives2DRenderLine(float x0, float y0, float x1, float y1, IgnisColorRGBA color)
{
    if (_lines.vertex_count + (2 * PRIMITIVES2D_VERTEX_SIZE) > PRIMITIVES2D_LINES_BUFFER_SIZE)
        PrimitivesDataFlush(&_lines, GL_LINES);

    PrimitivesPushVertex(&_lines, x0, y0, color);
    PrimitivesPushVertex(&_lines, x1, y1, color);
}

void Primitives2DRenderRect(float x, float y, float w, float h, IgnisColorRGBA color)
{
    Primitives2DRenderLine(x + 0, y + 0, x + w, y + 0, color);
    Primitives2DRenderLine(x + w, y + 0, x + w, y + h, color);
    Primitives2DRenderLine(x + w, y + h, x + 0, y + h, color);
    Primitives2DRenderLine(x + 0, y + h, x + 0, y + 0, color);
}

void Primitives2DRenderPolygon(const float* vertices, size_t count, IgnisColorRGBA color)
{
    if (!vertices || count < 2) return;

    float p1x = vertices[count - 2];
    float p1y = vertices[count - 1];

    size_t i = 0;
    while (i < count - 1)
    {
        float p2x = vertices[i++];
        float p2y = vertices[i++];

        Primitives2DRenderLine(p1x, p1y, p2x, p2y, color);

        p1x = p2x;
        p1y = p2y;
    }
}

void Primitives2DRenderCircle(float x, float y, float radius, IgnisColorRGBA color)
{
    float sinInc = sinf(PRIMITIVES2D_K_INCREMENT);
    float cosInc = cosf(PRIMITIVES2D_K_INCREMENT);

    float r1x = 1.0f;
    float r1y = 0.0f;

    float v1x = x + radius * r1x;
    float v1y = y + radius * r1y;

    for (int i = 0; i < PRIMITIVES2D_K_SEGMENTS; ++i)
    {
        /* Perform rotation to avoid additional trigonometry. */
        float r2x = cosInc * r1x - sinInc * r1y;
        float r2y = sinInc * r1x + cosInc * r1y;

        float v2x = x + radius * r2x;
        float v2y = y + radius * r2y;

        Primitives2DRenderLine(v1x, v1y, v2x, v2y, color);

        r1x = r2x;
        r1y = r2y;
        v1x = v2x;
        v1y = v2y;
    }
}

void Primitives2DFillTriangle(float x0, float y0, float x1, float y1, float x2, float y2, IgnisColorRGBA color)
{
    if (_triangles.vertex_count + (3 * PRIMITIVES2D_VERTEX_SIZE) > PRIMITIVES2D_TRIANGLES_BUFFER_SIZE)
        PrimitivesDataFlush(&_triangles, GL_TRIANGLES);

    PrimitivesPushVertex(&_triangles, x0, y0, color);
    PrimitivesPushVertex(&_triangles, x1, y1, color);
    PrimitivesPushVertex(&_triangles, x2, y2, color);
}

void Primitives2DFillRect(float x, float y, float w, float h, IgnisColorRGBA color)
{
    Primitives2DFillTriangle(x + 0, y + 0, x + w, y + 0, x + w, y + h, color);
    Primitives2DFillTriangle(x + w, y + h, x + 0, y + h, x + 0, y + 0, color);
}

void Primitives2DFillPolygon(const float* vertices, size_t count, IgnisColorRGBA color)
{
    if (!vertices || count < 2) return;

    float x0 = vertices[0], y0 = vertices[1];
    for (size_t i = 2; i < count - 3; ++i)
    {
        float x1 = vertices[i + 0], y1 = vertices[i + 1];
        float x2 = vertices[i + 2], y2 = vertices[i + 3];
        Primitives2DFillTriangle(x0, y0, x1, y1, x2, y2, color);
    }
}

void Primitives2DFillCircle(float x, float y, float radius, IgnisColorRGBA color)
{
    float sinInc = sinf(PRIMITIVES2D_K_INCREMENT);
    float cosInc = cosf(PRIMITIVES2D_K_INCREMENT);

    float r1x = 1.0f;
    float r1y = 0.0f;

    float v1x = x + radius * r1x;
    float v1y = y + radius * r1y;

    for (int i = 0; i < PRIMITIVES2D_K_SEGMENTS; ++i)
    {
        /* Perform rotation to avoid additional trigonometry. */
        float r2x = cosInc * r1x - sinInc * r1y;
        float r2y = sinInc * r1x + cosInc * r1y;

        float v2x = x + radius * r2x;
        float v2y = y + radius * r2y;

        Primitives2DFillTriangle(x, y, v1x, v1y, v2x, v2y, color);

        r1x = r2x;
        r1y = r2y;
        v1x = v2x;
        v1y = v2y;
    }
}
