#ifndef BATCHRENDERER2D_H
#define BATCHRENDERER2D_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "Ignis/Ignis.h"

void BatchRenderer2DInit(const char* vert, const char* frag);
void BatchRenderer2DDestroy();

void BatchRenderer2DStart(const float* mat_view_proj);
void BatchRenderer2DFlush();

void BatchRenderer2DRenderTexture(const IgnisTexture2D* texture, float x, float y, float w, float h);
void BatchRenderer2DRenderTextureFrame(const IgnisTexture2D* texture, float x, float y, float w, float h, size_t frame);
void BatchRenderer2DRenderTextureSrc(const IgnisTexture2D* texture, float x, float y, float w, float h, float src_x, float src_y, float src_w, float src_h);

#ifdef __cplusplus
}
#endif

#endif /* !BATCHRENDERER2D_H */
