#ifndef IGNISRENDERER_H
#define IGNISRENDERER_H

#include "Ignis/Ignis.h"
#include "Ignis/Quad.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define RENDERER_VERTICES_PER_QUAD  4
#define RENDERER_INDICES_PER_QUAD   6

/*
 * --------------------------------------------------------------
 *                          BatchRenderer2D
 * --------------------------------------------------------------
 */
#define BATCHRENDERER2D_MAX_QUADS   32
#define BATCHRENDERER2D_VERTEX_SIZE (3 + 2 + 1)

#define BATCHRENDERER2D_INDEX_COUNT (BATCHRENDERER2D_MAX_QUADS * RENDERER_INDICES_PER_QUAD)
#define BATCHRENDERER2D_BUFFER_SIZE (BATCHRENDERER2D_MAX_QUADS * RENDERER_VERTICES_PER_QUAD * BATCHRENDERER2D_VERTEX_SIZE)

#define BATCHRENDERER2D_TEXTURES    8

void BatchRenderer2DInit(const char* vert, const char* frag);
void BatchRenderer2DDestroy();

void BatchRenderer2DStart(const float* mat_view_proj);
void BatchRenderer2DFlush();

void BatchRenderer2DRenderTexture(const IgnisTexture2D* texture, float x, float y, float w, float h);
void BatchRenderer2DRenderTextureFrame(const IgnisTexture2D* texture, float x, float y, float w, float h, size_t frame);
void BatchRenderer2DRenderTextureSrc(const IgnisTexture2D* texture, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h);

/*
 * --------------------------------------------------------------
 *                          FontRenderer
 * --------------------------------------------------------------
 */

#define FONTRENDERER_MAX_QUADS      32
#define FONTRENDERER_VERTEX_SIZE    (2 + 2) /* 2f: vec; 2f: tex */

#define FONTRENDERER_INDEX_COUNT    (FONTRENDERER_MAX_QUADS * RENDERER_INDICES_PER_QUAD)
#define FONTRENDERER_BUFFER_SIZE    (FONTRENDERER_MAX_QUADS * RENDERER_VERTICES_PER_QUAD * FONTRENDERER_VERTEX_SIZE)
#define FONTRENDERER_QUAD_SIZE      (RENDERER_VERTICES_PER_QUAD * FONTRENDERER_VERTEX_SIZE)

#define FONTRENDERER_MAX_LINE_LENGTH    128

void FontRendererInit();
void FontRendererDestroy();

void FontRendererBindFont(IgnisFont* font);
void FontRendererBindFontColor(IgnisFont* font, IgnisColorRGBA color);

void FontRendererStart(const float* mat_proj);
void FontRendererFlush();

void FontRendererRenderText(float x, float y, const char* text);
void FontRendererRenderTextFormat(float x, float y, const char* fmt, ...);

void FontRendererTextFieldBegin(float x, float y, float spacing);
void FontRendererTextFieldLine(const char* fmt, ...);

/*
 * --------------------------------------------------------------
 *                          Primitives2D
 * --------------------------------------------------------------
 */

#define PRIMITIVES2D_MAX_VERTICES           1024
#define PRIMITIVES2D_VERTEX_SIZE            (2 + 4) /* 2f: position; 4f color */

#define PRIMITIVES2D_LINES_BUFFER_SIZE      (PRIMITIVES2D_VERTEX_SIZE * 2 * PRIMITIVES2D_MAX_VERTICES)
#define PRIMITIVES2D_TRIANGLES_BUFFER_SIZE  (PRIMITIVES2D_VERTEX_SIZE * 3 * PRIMITIVES2D_MAX_VERTICES)

 /* Circle */
#define PRIMITIVES2D_PI             3.14159265359f
#define PRIMITIVES2D_K_SEGMENTS     36
#define PRIMITIVES2D_K_INCREMENT    2.0f * PRIMITIVES2D_PI / PRIMITIVES2D_K_SEGMENTS

void Primitives2DInit();
void Primitives2DDestroy();

void Primitives2DStart(const float* mat_view_proj);
void Primitives2DFlush();

void Primitives2DRenderLine(float x1, float y1, float x2, float y2, IgnisColorRGBA color);
void Primitives2DRenderRect(float x, float y, float w, float h, IgnisColorRGBA color);
void Primitives2DRenderPolygon(const float* vertices, size_t count, IgnisColorRGBA color);
void Primitives2DRenderCircle(float x, float y, float radius, IgnisColorRGBA color);

void Primitives2DFillRect(float x, float y, float w, float h, IgnisColorRGBA color);
void Primitives2DFillPolygon(const float* vertices, size_t count, IgnisColorRGBA color);
void Primitives2DFillCircle(float x, float y, float radius, IgnisColorRGBA color);

/*
 * --------------------------------------------------------------
 *                          Renderer2D
 * --------------------------------------------------------------
 */
void Renderer2DInit();
void Renderer2DDestroy();

void Renderer2DSetShader(IgnisShader* shader);

void Renderer2DSetViewProjection(const float* view_proj);

void Renderer2DRenderTexture(const IgnisTexture2D* texture, float x, float y);
void Renderer2DRenderTextureScale(const IgnisTexture2D* texture, float x, float y, float w, float h);
void Renderer2DRenderTextureColor(const IgnisTexture2D* texture, float x, float y, float w, float h, IgnisColorRGBA color);
void Renderer2DRenderTextureModel(const IgnisTexture2D* texture, const float* model, IgnisColorRGBA color);

/*
 * --------------------------------------------------------------
 *                          Utility
 * --------------------------------------------------------------
 */
void GenerateQuadIndices(GLuint* indices, size_t max);
void GetTexture2DSrcRect(const IgnisTexture2D* texture, size_t frame, float* x, float* y, float* w, float* h);

#ifdef __cplusplus
}
#endif

#endif /* !IGNISRENDERER_H */
