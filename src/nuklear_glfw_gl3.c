#include "nuklear_glfw_gl3.h"

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
    static const GLchar* vertex_shader =
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
    static const GLchar* fragment_shader =
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

NK_API struct nk_context*
nk_glfw3_init(struct nk_glfw* glfw, MinimalWindow* win)
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
    const void* image; int w, h;
    image = nk_font_atlas_bake(&glfw->atlas, &w, &h, NK_FONT_ATLAS_RGBA32);

    // upload atlas
    struct nk_glfw_device* dev = &glfw->ogl;
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
    struct nk_context* ctx = &glfw->ctx;
    MinimalWindow* win = glfw->win;

    nk_input_begin(ctx);
    for (int i = 0; i < glfw->text_len; ++i)
        nk_input_unicode(ctx, glfw->text[i]);

    nk_input_key(ctx, NK_KEY_DEL,           minimalGetKeyState(win, MINIMAL_KEY_DELETE));
    nk_input_key(ctx, NK_KEY_ENTER,         minimalGetKeyState(win, MINIMAL_KEY_ENTER));
    nk_input_key(ctx, NK_KEY_TAB,           minimalGetKeyState(win, MINIMAL_KEY_TAB));
    nk_input_key(ctx, NK_KEY_BACKSPACE,     minimalGetKeyState(win, MINIMAL_KEY_BACKSPACE));
    nk_input_key(ctx, NK_KEY_UP,            minimalGetKeyState(win, MINIMAL_KEY_UP));
    nk_input_key(ctx, NK_KEY_DOWN,          minimalGetKeyState(win, MINIMAL_KEY_DOWN));
    nk_input_key(ctx, NK_KEY_TEXT_START,    minimalGetKeyState(win, MINIMAL_KEY_HOME));
    nk_input_key(ctx, NK_KEY_TEXT_END,      minimalGetKeyState(win, MINIMAL_KEY_END));
    nk_input_key(ctx, NK_KEY_SCROLL_START,  minimalGetKeyState(win, MINIMAL_KEY_HOME));
    nk_input_key(ctx, NK_KEY_SCROLL_END,    minimalGetKeyState(win, MINIMAL_KEY_END));
    nk_input_key(ctx, NK_KEY_SCROLL_DOWN,   minimalGetKeyState(win, MINIMAL_KEY_PAGE_DOWN));
    nk_input_key(ctx, NK_KEY_SCROLL_UP,     minimalGetKeyState(win, MINIMAL_KEY_PAGE_UP));
    nk_input_key(ctx, NK_KEY_SHIFT,         minimalGetKeyState(win, MINIMAL_KEY_LEFT_SHIFT) ||
                                            minimalGetKeyState(win, MINIMAL_KEY_RIGHT_SHIFT));

    if (minimalGetKeyState(win, MINIMAL_KEY_LEFT_CONTROL) || minimalGetKeyState(win, MINIMAL_KEY_RIGHT_CONTROL))
    {
        nk_input_key(ctx, NK_KEY_COPY,            minimalGetKeyState(win, MINIMAL_KEY_C));
        nk_input_key(ctx, NK_KEY_PASTE,           minimalGetKeyState(win, MINIMAL_KEY_V));
        nk_input_key(ctx, NK_KEY_CUT,             minimalGetKeyState(win, MINIMAL_KEY_X));
        nk_input_key(ctx, NK_KEY_TEXT_UNDO,       minimalGetKeyState(win, MINIMAL_KEY_Z));
        nk_input_key(ctx, NK_KEY_TEXT_REDO,       minimalGetKeyState(win, MINIMAL_KEY_R));
        nk_input_key(ctx, NK_KEY_TEXT_WORD_LEFT,  minimalGetKeyState(win, MINIMAL_KEY_LEFT));
        nk_input_key(ctx, NK_KEY_TEXT_WORD_RIGHT, minimalGetKeyState(win, MINIMAL_KEY_RIGHT));
        nk_input_key(ctx, NK_KEY_TEXT_LINE_START, minimalGetKeyState(win, MINIMAL_KEY_B));
        nk_input_key(ctx, NK_KEY_TEXT_LINE_END,   minimalGetKeyState(win, MINIMAL_KEY_E));
    }
    else
    {
        nk_input_key(ctx, NK_KEY_LEFT,  minimalGetKeyState(win, MINIMAL_KEY_LEFT));
        nk_input_key(ctx, NK_KEY_RIGHT, minimalGetKeyState(win, MINIMAL_KEY_RIGHT));
        nk_input_key(ctx, NK_KEY_COPY, 0);
        nk_input_key(ctx, NK_KEY_PASTE, 0);
        nk_input_key(ctx, NK_KEY_CUT, 0);
        nk_input_key(ctx, NK_KEY_SHIFT, 0);
    }

    float x, y;
    minimalGetCursorPos(win, &x, &y);
    nk_input_motion(ctx, (int)x, (int)y);

    nk_input_button(ctx, NK_BUTTON_LEFT,   (int)x, (int)y, minimalGetMouseButtonState(win, MINIMAL_MOUSE_BUTTON_LEFT));
    nk_input_button(ctx, NK_BUTTON_MIDDLE, (int)x, (int)y, minimalGetMouseButtonState(win, MINIMAL_MOUSE_BUTTON_MIDDLE));
    nk_input_button(ctx, NK_BUTTON_RIGHT,  (int)x, (int)y, minimalGetMouseButtonState(win, MINIMAL_MOUSE_BUTTON_RIGHT));
    nk_input_button(ctx, NK_BUTTON_DOUBLE, (int)glfw->double_click_pos.x, (int)glfw->double_click_pos.y, glfw->is_double_click_down);
    nk_input_scroll(ctx, glfw->scroll);
    nk_input_end(&glfw->ctx);
    glfw->text_len = 0;
    glfw->scroll = nk_vec2(0, 0);
}

NK_API void
nk_glfw3_render(struct nk_glfw* glfw, enum nk_anti_aliasing AA)
{
    MinimalWindow* win = glfw->win;
    int width, height;
    int display_width, display_height;
    minimalGetWindowSize(win, &width, &height);
    minimalGetFramebufferSize(win, &display_width, &display_height);

    struct nk_glfw_device* dev = &glfw->ogl;
    struct nk_buffer vbuf, ebuf;
    GLfloat ortho[4][4] = {
        {2.0f, 0.0f, 0.0f, 0.0f},
        {0.0f,-2.0f, 0.0f, 0.0f},
        {0.0f, 0.0f,-1.0f, 0.0f},
        {-1.0f,1.0f, 0.0f, 1.0f},
    };
    ortho[0][0] /= (GLfloat)width;
    ortho[1][1] /= (GLfloat)height;

    /* setup global state */
    glEnable(GL_BLEND);
    glBlendEquation(GL_FUNC_ADD);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_SCISSOR_TEST);
    glActiveTexture(GL_TEXTURE0);


    struct nk_vec2 fb_scale;
    minimalGetWindowContentScale(win, &fb_scale.x, &fb_scale.y);

    /* setup program */
    ignisUseShader(dev->prog);
    ignisSetUniformil(dev->prog, dev->uniform_tex, 0);
    ignisSetUniformMat4l(dev->prog, dev->uniform_proj, 1, &ortho[0][0]);
    glViewport(0, 0, (GLsizei)display_width, (GLsizei)display_height);
    {
        /* convert from command queue into draw list and draw to screen */
        /* allocate vertex and element buffer */
        ignisBindVertexArray(&dev->vao);

        /* load draw vertices & elements directly into vertex + element buffer */
        void* vertices = ignisMapBuffer(&dev->vao.buffers[0], GL_WRITE_ONLY);
        void* elements = ignisMapBuffer(&dev->vao.buffers[1], GL_WRITE_ONLY);
        {
            /* fill convert configuration */
            static const struct nk_draw_vertex_layout_element vertex_layout[] = {
                {NK_VERTEX_POSITION, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, position)},
                {NK_VERTEX_TEXCOORD, NK_FORMAT_FLOAT, NK_OFFSETOF(struct nk_glfw_vertex, uv)},
                {NK_VERTEX_COLOR, NK_FORMAT_R8G8B8A8, NK_OFFSETOF(struct nk_glfw_vertex, col)},
                {NK_VERTEX_LAYOUT_END}
            };

            struct nk_convert_config config = {
                .vertex_layout = vertex_layout,
                .vertex_size = sizeof(struct nk_glfw_vertex),
                .vertex_alignment = NK_ALIGNOF(struct nk_glfw_vertex),
                .tex_null = dev->tex_null,
                .circle_segment_count = 22,
                .curve_segment_count = 22,
                .arc_segment_count = 22,
                .global_alpha = 1.0f,
                .shape_AA = AA,
                .line_AA = AA
            };

            /* setup buffers to load vertices and elements */
            nk_buffer_init_fixed(&vbuf, vertices, (size_t)MAX_VERTEX_BUFFER);
            nk_buffer_init_fixed(&ebuf, elements, (size_t)MAX_ELEMENT_BUFFER);
            nk_convert(&glfw->ctx, &dev->cmds, &vbuf, &ebuf, &config);
        }
        ignisUnmapBuffer(&dev->vao.buffers[0]);
        ignisUnmapBuffer(&dev->vao.buffers[1]);

        /* iterate over and execute each draw command */
        nk_size offset = 0;
        const struct nk_draw_command* cmd;
        nk_draw_foreach(cmd, &glfw->ctx, &dev->cmds)
        {
            if (!cmd->elem_count) continue;
            glBindTexture(GL_TEXTURE_2D, (GLuint)cmd->texture.id);
            IgnisRect rect = {
                .x = cmd->clip_rect.x * fb_scale.x,
                .y = (height - (GLint)(cmd->clip_rect.y + cmd->clip_rect.h)) * fb_scale.y,
                .w = cmd->clip_rect.w * fb_scale.x,
                .h = cmd->clip_rect.h * fb_scale.y
            };
            glScissor((GLint)rect.x, (GLint)rect.y, (GLint)rect.w, (GLint)rect.h);
            glDrawElements(GL_TRIANGLES, (GLsizei)cmd->elem_count, GL_UNSIGNED_SHORT, (const void*)offset);
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