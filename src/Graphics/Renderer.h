#ifndef IGNISRENDERER_H
#define IGNISRENDERER_H

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * --------------------------------------------------------------
 *                          BatchRenderer2D
 * --------------------------------------------------------------
 */
#define BATCHRENDERER2D_MAX_QUADS           32
#define BATCHRENDERER2D_VERTEX_SIZE         (3 + 2 + 1)
#define BATCHRENDERER2D_VERTICES_PER_QUAD   4
#define BATCHRENDERER2D_INDICES_PER_QUAD    6

#define BATCHRENDERER2D_INDEX_COUNT     (BATCHRENDERER2D_MAX_QUADS * BATCHRENDERER2D_INDICES_PER_QUAD)
#define BATCHRENDERER2D_VERTEX_COUNT    (BATCHRENDERER2D_MAX_QUADS * BATCHRENDERER2D_VERTICES_PER_QUAD)
#define BATCHRENDERER2D_BUFFER_SIZE     (BATCHRENDERER2D_VERTEX_COUNT * BATCHRENDERER2D_VERTEX_SIZE)

#define BATCHRENDERER2D_TEXTURES    8

#include "BatchRenderer2D.h"

/*
 * --------------------------------------------------------------
 *                          FontRenderer
 * --------------------------------------------------------------
 */
#define FONTRENDERER_MAX_QUADS          32
#define FONTRENDERER_VERTEX_SIZE        4
#define FONTRENDERER_VERTICES_PER_QUAD  4
#define FONTRENDERER_INDICES_PER_QUAD   6

#define FONTRENDERER_INDEX_COUNT        (FONTRENDERER_MAX_QUADS * FONTRENDERER_INDICES_PER_QUAD)
#define FONTRENDERER_VERTEX_COUNT       (FONTRENDERER_MAX_QUADS * FONTRENDERER_VERTICES_PER_QUAD)
#define FONTRENDERER_BUFFER_SIZE        (FONTRENDERER_VERTEX_COUNT * FONTRENDERER_VERTEX_SIZE)

#define FONTRENDERER_MAX_LINE_LENGTH    128

#include "FontRenderer.h"

/*
 * --------------------------------------------------------------
 *                          Primitives2D
 * --------------------------------------------------------------
 */
#define PRIMITIVES2D_LINES_MAX_VERTICES         (2 * 1024)
#define PRIMITIVES2D_TRIANGLES_MAX_VERTICES     (3 * 1024)
#define PRIMITIVES2D_VERTEX_SIZE                (2 + 4)     /* 2f: position; 4f color */

#define PRIMITIVES2D_LINES_BUFFER_SIZE          (PRIMITIVES2D_VERTEX_SIZE * PRIMITIVES2D_LINES_MAX_VERTICES)
#define PRIMITIVES2D_TRIANGLES_BUFFER_SIZE      (PRIMITIVES2D_VERTEX_SIZE * PRIMITIVES2D_TRIANGLES_MAX_VERTICES)

/* Circle */
#define PRIMITIVES2D_PI             3.14159265359f
#define PRIMITIVES2D_K_SEGMENTS     36
#define PRIMITIVES2D_K_INCREMENT    2.0f * PRIMITIVES2D_PI / PRIMITIVES2D_K_SEGMENTS

#include "Primitives2D.h"

/*
 * --------------------------------------------------------------
 *                          Renderer2D
 * --------------------------------------------------------------
 */
#include "Renderer2D.h"


void GenerateIndices(GLuint* indices, size_t max, size_t step);
void GetTexture2DSrcRect(const IgnisTexture2D* texture, size_t frame, float* src_x, float* src_y, float* src_w, float* src_h);

#ifdef __cplusplus
}
#endif

#endif /* !IGNISRENDERER_H */
