#ifndef RENDERER2D_H
#define RENDERER2D_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "Ignis/Ignis.h"
#include "Ignis/Quad.h"

void Renderer2DInit(const char* vert, const char* frag);
void Renderer2DDestroy();

void Renderer2DSetShader(IgnisShader* shader);

void Renderer2DRenderTexture(IgnisTexture2D* texture, float x, float y, float w, float h, const float* view_proj);
void Renderer2DRenderTextureColor(IgnisTexture2D* texture, float x, float y, float w, float h, const float* view_proj, IgnisColorRGBA color);
void Renderer2DRenderTextureModel(IgnisTexture2D* texture, const float* model, const float* view_proj, IgnisColorRGBA color);

void Renderer2DRenderTextureQuad(IgnisTexture2D* texture, IgnisQuad* quad, const float* model, const float* view_proj, IgnisColorRGBA color);

#ifdef __cplusplus
}
#endif

#endif /* !RENDERER2D_H */