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

#include <GLFW/glfw3.h>
#include <ignis/ignis.h>

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

    GLuint font_tex;
};

struct nk_glfw {
    GLFWwindow *win;
    int width, height;
    int display_width, display_height;
    struct nk_glfw_device ogl;
    struct nk_context ctx;
    struct nk_font_atlas atlas;
    struct nk_vec2 fb_scale;
    unsigned int text[NK_GLFW_TEXT_MAX];
    int text_len;
    struct nk_vec2 scroll;
    double last_button_click;
    int is_double_click_down;
    struct nk_vec2 double_click_pos;
};

NK_API struct nk_context*   nk_glfw3_init(struct nk_glfw* glfw, GLFWwindow *win);
NK_API void                 nk_glfw3_shutdown(struct nk_glfw* glfw);
NK_API void                 nk_glfw3_font_stash_begin(struct nk_glfw* glfw);
NK_API void                 nk_glfw3_font_stash_end(struct nk_glfw* glfw);
NK_API void                 nk_glfw3_new_frame(struct nk_glfw* glfw);
NK_API void                 nk_glfw3_render(struct nk_glfw* glfw, enum nk_anti_aliasing AA);

NK_API void                 nk_glfw3_device_destroy(struct nk_glfw_device* dev);
NK_API void                 nk_glfw3_device_create(struct nk_glfw_device* dev);

#endif
/*
 * ==============================================================
 *
 *                          IMPLEMENTATION
 *
 * ===============================================================
 */
#ifdef NK_GLFW_GL3_IMPLEMENTATION

#ifndef NK_GLFW_DOUBLE_CLICK_LO
#define NK_GLFW_DOUBLE_CLICK_LO 0.02
#endif
#ifndef NK_GLFW_DOUBLE_CLICK_HI
#define NK_GLFW_DOUBLE_CLICK_HI 0.2
#endif

struct nk_glfw_vertex {
    float position[2];
    float uv[2];
    nk_byte col[4];
};

#define NK_SHADER_VERSION "#version 330 core\n"

NK_API void
nk_glfw3_device_create(struct nk_glfw_device* dev)
{
    GLint status;
    static const GLchar *vertex_shader =
        NK_SHADER_VERSION
        "layout (location = 0) in vec2 Position;\n"
        "layout (location = 1) in vec2 TexCoord;\n"
        "layout (location = 2) in vec4 Color;\n"
        "out vec2 Frag_UV;\n"
        "out vec4 Frag_Color;\n"
        "uniform mat4 ProjMtx;\n"
        "void main() {\n"
        "   Frag_UV = TexCoord;\n"
        "   Frag_Color = Color;\n"
        "   gl_Position = ProjMtx * vec4(Position.xy, 0, 1);\n"
        "}\n";
    static const GLchar *fragment_shader =
        NK_SHADER_VERSION
        "precision mediump float;\n"
        "uniform sampler2D Texture;\n"
        "in vec2 Frag_UV;\n"
        "in vec4 Frag_Color;\n"
        "out vec4 Out_Color;\n"
        "void main(){\n"
        "   Out_Color = Frag_Color * texture(Texture, Frag_UV.st);\n"
        "}\n";

    nk_buffer_init_default(&dev->cmds);

    dev->prog = ignisCreateShaderSrcvf(vertex_shader, fragment_shader);

    dev->uniform_tex = ignisGetUniformLocation(dev->prog, "Texture");
    dev->uniform_proj = ignisGetUniformLocation(dev->prog, "ProjMtx");

    /* buffer setup */
    ignisGenerateVertexArray(&dev->vao, 2);

    ignisLoadArrayBuffer(&dev->vao, 0, MAX_VERTEX_BUFFER, NULL, IGNIS_STREAM_DRAW);

    IgnisBufferElement layout[] = {
       { IGNIS_FLOAT, 2, GL_FALSE },
       { IGNIS_FLOAT, 2, GL_FALSE },
       { IGNIS_UINT8, 4, GL_TRUE }
    };
    ignisSetVertexLayout(&dev->vao, 0, layout, 3);

    ignisLoadElementBuffer(&dev->vao, 1, NULL, MAX_ELEMENT_BUFFER, IGNIS_STREAM_DRAW);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

NK_API void
nk_glfw3_device_destroy(struct nk_glfw_device* dev)
{
    ignisDeleteShader(dev->prog);
    glDeleteTextures(1, &dev->font_tex);
    ignisDeleteVertexArray(&dev->vao);
    nk_buffer_free(&dev->cmds);
}

NK_API void
nk_glfw3_render(struct nk_glfw* glfw, enum nk_anti_aliasing AA)
{
    struct nk_glfw_device *dev = &glfw->ogl;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)glfw->width;
    ortho[1][1] /= (GLfloat)glfw->height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);

    /* setup program */
    glUseProgram(dev->prog);
    glUniform1i(dev->uniform_tex, 0);
    glUniformMatrix4fv(dev->uniform_proj, 1, GL_FALSE, &ortho[0][0]);
    glViewport(0,0,(GLsizei)glfw->display_width,(GLsizei)glfw->display_height);
    {
        /* convert from command queue into draw list and draw to screen */
        const struct nk_draw_command *cmd;
        void *vertices, *elements;
        nk_size offset = 0;

        /* allocate vertex and element buffer */
        ignisBindVertexArray(&dev->vao);

        /* load draw vertices & elements directly into vertex + element buffer */
        vertices = ignisMapBuffer(&dev->vao.buffers[0], GL_WRITE_ONLY);
        elements = ignisMapBuffer(&dev->vao.buffers[1], GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            struct nk_convert_config config;
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };
            memset(&config, 0, sizeof(config));
            config.vertex_layout = vertex_layout;
            config.vertex_size = sizeof(struct nk_glfw_vertex);
            config.vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex);
            config.tex_null = dev->tex_null;
            config.circle_segment_count = 22;
            config.curve_segment_count = 22;
            config.arc_segment_count = 22;
            config.global_alpha = 1.0f;
            config.shape_AA = AA;
            config.line_AA = AA;

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)MAX_VERTEX_BUFFER);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)MAX_ELEMENT_BUFFER);
            nk_convert(&glfw->ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        ignisUnmapBuffer(&dev->vao.buffers[0]);
        ignisUnmapBuffer(&dev->vao.buffers[1]);

        /* iterate over and execute each draw command */
        nk_draw_foreach(cmd, &glfw->ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            glScissor(
                (GLint)(cmd->clip_rect.x * glfw->fb_scale.x),
                (GLint)((glfw->height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * glfw->fb_scale.y),
                (GLint)(cmd->clip_rect.w * glfw->fb_scale.x),
                (GLint)(cmd->clip_rect.h * glfw->fb_scale.y));
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, (const void*) offset);
            offset += cmd->elem_count * sizeof(nk_draw_index);
        }
        nk_clear(&glfw->ctx);
        nk_buffer_clear(&dev->cmds);
    }

    /* default OpenGL state */
    glUseProgram(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
}

NK_API struct nk_context*
nk_glfw3_init(struct nk_glfw* glfw, GLFWwindow *win)
{
    glfw->win = win;

    nk_init_default(&glfw->ctx, 0);
    glfw->last_button_click = 0;
    nk_glfw3_device_create(&glfw->ogl);

    glfw->is_double_click_down = nk_false;
    glfw->double_click_pos = nk_vec2(0, 0);

    return &glfw->ctx;
}

NK_API
void nk_glfw3_shutdown(struct nk_glfw* glfw)
{
    nk_font_atlas_clear(&glfw->atlas);
    nk_free(&glfw->ctx);
    nk_glfw3_device_destroy(&glfw->ogl);
    memset(glfw, 0, sizeof(*glfw));
}

NK_API void
nk_glfw3_font_stash_begin(struct nk_glfw* glfw)
{
    nk_font_atlas_init_default(&glfw->atlas);
    nk_font_atlas_begin(&glfw->atlas);
}

NK_API void
nk_glfw3_font_stash_end(struct nk_glfw* glfw)
{
    const void *image; int w, h;
    image = nk_font_atlas_bake(&glfw->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    // upload atlas
    struct nk_glfw_device *dev = &glfw->ogl;
    glGenTextures(1, &dev->font_tex);
    glBindTexture(GL_TEXTURE_2D, dev->font_tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, (GLsizei)w, (GLsizei)h, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

    nk_font_atlas_end(&glfw->atlas, nk_handle_id((int)glfw->ogl.font_tex), &glfw->ogl.tex_null);
    if (glfw->atlas.default_font)
        nk_style_set_font(&glfw->ctx, &glfw->atlas.default_font->handle);
}

NK_API void
nk_glfw3_new_frame(struct nk_glfw* glfw)
{
    struct nk_context *ctx = &glfw->ctx;
    struct GLFWwindow *win = glfw->win;

    glfwGetWindowSize(win, &glfw->width, &glfw->height);
    glfwGetFramebufferSize(win, &glfw->display_width, &glfw->display_height);
    glfw->fb_scale.x = (float)glfw->display_width/(float)glfw->width;
    glfw->fb_scale.y = (float)glfw->display_height/(float)glfw->height;

    nk_input_begin(ctx);
    for (int i = 0; i < glfw->text_len; ++i)
        nk_input_unicode(ctx, glfw->text[i]);

    nk_input_key(ctx, NK_KEY_DEL, glfwGetKey(win, GLFW_KEY_DELETE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_ENTER, glfwGetKey(win, GLFW_KEY_ENTER) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TAB, glfwGetKey(win, GLFW_KEY_TAB) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_BACKSPACE, glfwGetKey(win, GLFW_KEY_BACKSPACE) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_UP, glfwGetKey(win, GLFW_KEY_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_DOWN, glfwGetKey(win, GLFW_KEY_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_TEXT_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_START, glfwGetKey(win, GLFW_KEY_HOME) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_END, glfwGetKey(win, GLFW_KEY_END) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN, glfwGetKey(win, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SCROLL_UP, glfwGetKey(win, GLFW_KEY_PAGE_UP) == GLFW_PRESS);
    nk_input_key(ctx, NK_KEY_SHIFT, glfwGetKey(win, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS||
                                    glfwGetKey(win, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS);

    if (glfwGetKey(win, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS ||
        glfwGetKey(win, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        nk_input_key(ctx, NK_KEY_COPY, glfwGetKey(win, GLFW_KEY_C) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_PASTE, glfwGetKey(win, GLFW_KEY_V) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_CUT, glfwGetKey(win, GLFW_KEY_X) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_UNDO, glfwGetKey(win, GLFW_KEY_Z) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_REDO, glfwGetKey(win, GLFW_KEY_R) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, glfwGetKey(win, GLFW_KEY_B) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END, glfwGetKey(win, GLFW_KEY_E) == GLFW_PRESS);
    } else {
        nk_input_key(ctx, NK_KEY_LEFT, glfwGetKey(win, GLFW_KEY_LEFT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_RIGHT, glfwGetKey(win, GLFW_KEY_RIGHT) == GLFW_PRESS);
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    double x, y;
    glfwGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);

    nk_input_button(ctx, NK_BUTTON_LEFT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_RIGHT, (int)x, (int)y, glfwGetMouseButton(win, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);
    nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)glfw->double_click_pos.x, (int)glfw->double_click_pos.y, glfw->is_double_click_down);
    nk_input_scroll(ctx, glfw->scroll);
    nk_input_end(&glfw->ctx);
    glfw->text_len = 0;
    glfw->scroll = nk_vec2(0,0);
}


#endif
