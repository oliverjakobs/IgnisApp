#include "Renderer.h"

/* ---------------------| shader |---------------------------------------------*/
static const char* vert_src = "#version 330 core \n             \
layout(location = 0) in vec2 a_Pos;                             \
layout(location = 1) in vec2 a_TexCoord;                        \
uniform mat4 u_ViewProj;                                        \
uniform mat4 u_Model;                                           \
out vec2 v_TexCoord;                                            \
void main()                                                     \
{                                                               \
    gl_Position = u_ViewProj * u_Model * vec4(a_Pos, 0.0, 1.0); \
    v_TexCoord = a_TexCoord;                                    \
}";

static const char* frag_src = "#version 330 core \n     \
layout(location = 0) out vec4 f_Color;                  \
in vec4 v_Color;                                        \
in vec2 v_TexCoord;                                     \
uniform vec4 u_Color;                                   \
uniform sampler2D u_Texture;                            \
void main()                                             \
{                                                       \
    f_Color = texture(u_Texture, v_TexCoord) * u_Color; \
}";
/* ---------------------| !shader |--------------------------------------------*/

typedef struct
{
    IgnisQuad quad;

    IgnisShader default_shader;
    IgnisShader* shader;

    GLint uniform_location_view_proj;
    GLint uniform_location_color;
    GLint uniform_location_model;
} Renderer2DStorage;

static Renderer2DStorage _render_data;

void Renderer2DInit()
{
    ignisCreateQuadTextured(&_render_data.quad, GL_STATIC_DRAW);

    ignisCreateShaderSrcvf(&_render_data.default_shader, vert_src, frag_src);

    Renderer2DSetShader(NULL);
}

void Renderer2DDestroy()
{
    ignisDeleteShader(&_render_data.default_shader);
    ignisDeleteQuad(&_render_data.quad);
}

void Renderer2DSetShader(IgnisShader* shader)
{
    if (shader) _render_data.shader = shader;
    else        _render_data.shader = &_render_data.default_shader;

    _render_data.uniform_location_view_proj = ignisGetUniformLocation(_render_data.shader, "u_ViewProj");
    _render_data.uniform_location_color = ignisGetUniformLocation(_render_data.shader, "u_Color");
    _render_data.uniform_location_model = ignisGetUniformLocation(_render_data.shader, "u_Model");

    ignisSetUniform1i(_render_data.shader, "u_Texture", 0);
}

void Renderer2DSetViewProjection(const float* view_proj)
{
    ignisSetUniformMat4l(_render_data.shader, _render_data.uniform_location_view_proj, view_proj);
}

void Renderer2DRenderTexture(const IgnisTexture2D* texture, float x, float y)
{
    Renderer2DRenderTextureColor(texture, x, y, (float)texture->width, (float)texture->height, IGNIS_WHITE);
}

void Renderer2DRenderTextureScale(const IgnisTexture2D* texture, float x, float y, float w, float h)
{
    Renderer2DRenderTextureColor(texture, x, y, w, h, IGNIS_WHITE);
}

void Renderer2DRenderTextureColor(const IgnisTexture2D* texture, float x, float y, float w, float h, IgnisColorRGBA color)
{
    /* creating a mat4 that translates and scales */
    float model[16];
    model[0] = w;		model[4] = 0.0f;	model[8] = 0.0f;	model[12] = x;
    model[1] = 0.0f;	model[5] = h;		model[9] = 0.0f;	model[13] = y;
    model[2] = 0.0f;	model[6] = 0.0f;	model[10] = 1.0f;	model[14] = 0.0f;
    model[3] = 0.0f;	model[7] = 0.0f;	model[11] = 0.0f;	model[15] = 1.0f;

    Renderer2DRenderTextureModel(texture, model, color);
}

void Renderer2DRenderTextureModel(const IgnisTexture2D* texture, const float* model, IgnisColorRGBA color)
{
    ignisUseShader(_render_data.shader);

    ignisSetUniformMat4l(_render_data.shader, _render_data.uniform_location_model, model);
    ignisSetUniform4fl(_render_data.shader, _render_data.uniform_location_color, &color.r);

    ignisBindTexture2D(texture, 0);

    ignisDrawQuadElements(&_render_data.quad, GL_TRIANGLES);
}


/* ---------------------| utility |--------------------------------------------*/
void GenerateQuadIndices(GLuint* indices, size_t max)
{
    GLuint offset = 0;
    for (size_t i = 0; i < max - 6; i += 6)
    {
        indices[i + 0] = offset + 0;
        indices[i + 1] = offset + 1;
        indices[i + 2] = offset + 2;

        indices[i + 3] = offset + 2;
        indices[i + 4] = offset + 3;
        indices[i + 5] = offset + 0;

        offset += 4;
    }
}

void GetTexture2DSrcRect(const IgnisTexture2D* texture, size_t frame, float* x, float* y, float* w, float* h)
{
    *w = 1.0f / texture->cols;
    *h = 1.0f / texture->rows;
    *x = (frame % texture->cols) * (*w);
    *y = 1 - *h - ((frame / texture->cols) * (*h));
}
/* ---------------------| !utility |-------------------------------------------*/
