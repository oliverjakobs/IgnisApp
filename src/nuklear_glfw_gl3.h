/*
 * Nuklear - 1.32.0 - public domain
 * no warrenty implied; use at your own risk.
 * authored from 2015-2016 by Micha Mettke
 */
/*
 * ==============================================================
 *
 *                              API
 *
 * ===============================================================
 */
#ifndef NK_GLFW_GL3_H_
#define NK_GLFW_GL3_H_

#include "nuklear/nuklear.h"

#include <ignis/ignis.h>
#include <minimal.h>

#ifndef NK_GLFW_TEXT_MAX
#define NK_GLFW_TEXT_MAX 256
#endif

#define MAX_VERTEX_BUFFER 512 * 1024
#define MAX_ELEMENT_BUFFER 128 * 1024

struct nk_glfw_device {
    struct nk_buffer cmds;
    struct nk_draw_null_texture tex_null;

    IgnisShader prog;
    IgnisVertexArray vao;
    GLint uniform_tex;
    GLint uniform_proj;
};

struct nk_glfw {
    MinimalWindow *win;

    struct nk_glfw_device ogl;
    struct nk_context ctx;
    IgnisFontAtlas atlas;

    unsigned int text[NK_GLFW_TEXT_MAX];
    int text_len;
    struct nk_vec2 scroll;
};

NK_API struct nk_context*   nk_glfw3_init(struct nk_glfw* glfw, MinimalWindow* win);
NK_API void                 nk_glfw3_shutdown(struct nk_glfw* glfw);
NK_API void                 nk_glfw3_load_font_atlas(struct nk_glfw* glfw);
NK_API void                 nk_glfw3_new_frame(struct nk_glfw* glfw, float deltatime);
NK_API void                 nk_glfw3_render(struct nk_glfw* glfw);

NK_API void                 nk_glfw3_device_destroy(struct nk_glfw_device* dev);
NK_API void                 nk_glfw3_device_create(struct nk_glfw_device* dev);

uint8_t loadDefaultFont(IgnisFontConfig* config, float height);


#ifndef NK_GLFW_DOUBLE_CLICK_LO
#define NK_GLFW_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_HI
#define NK_GLFW_DOUBLE_CLICK_HI 0.2
#endif

#endif
