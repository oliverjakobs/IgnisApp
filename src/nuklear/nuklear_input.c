#include "nuklear.h"
#include "nuklear_internal.h"

#include <minimal/minimal.h>

/* ===============================================================
 *
 *                          INPUT
 *
 * ===============================================================*/
NK_API void
nk_input_begin(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    if (!ctx) return;
    struct nk_input* in = &ctx->input;
    in->keyboard.text_len = 0;
    in->mouse.scroll_delta = nk_vec2(0,0);
    in->mouse.prev.x = in->mouse.pos.x;
    in->mouse.prev.y = in->mouse.pos.y;
    in->mouse.delta.x = 0;
    in->mouse.delta.y = 0;
}


NK_API void nk_input_motion(struct nk_context *ctx, int x, int y)
{
    struct nk_input *in;
    NK_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;
    in->mouse.pos.x = (float)x;
    in->mouse.pos.y = (float)y;
    in->mouse.delta.x = in->mouse.pos.x - in->mouse.prev.x;
    in->mouse.delta.y = in->mouse.pos.y - in->mouse.prev.y;
}

NK_API void nk_input_button(struct nk_context *ctx, enum nk_buttons id, nk_bool down)
{
    NK_ASSERT(ctx);
    if (!ctx) return;

    struct nk_input* in = &ctx->input;
    if (nk_input_is_mouse_pressed(in, id))
    {
        in->mouse.clicked_pos[id].x = in->mouse.pos.x;
        in->mouse.clicked_pos[id].y = in->mouse.pos.y;
    }
}

NK_API void nk_input_scroll(struct nk_context *ctx, struct nk_vec2 val)
{
    NK_ASSERT(ctx);
    if (!ctx) return;
    ctx->input.mouse.scroll_delta.x += val.x;
    ctx->input.mouse.scroll_delta.y += val.y;
}


NK_API void nk_input_glyph(struct nk_context *ctx, const nk_glyph glyph)
{
    int len = 0;
    nk_rune unicode;
    struct nk_input *in;

    NK_ASSERT(ctx);
    if (!ctx) return;
    in = &ctx->input;

    len = nk_utf_decode(glyph, &unicode, NK_UTF_SIZE);
    if (len && ((in->keyboard.text_len + len) < NK_INPUT_MAX))
    {
        nk_utf_encode(unicode, &in->keyboard.text[in->keyboard.text_len],  NK_INPUT_MAX - in->keyboard.text_len);
        in->keyboard.text_len += len;
    }
}

NK_API void nk_input_char(struct nk_context *ctx, char c)
{
    nk_glyph glyph;
    NK_ASSERT(ctx);
    if (!ctx) return;
    glyph[0] = c;
    nk_input_glyph(ctx, glyph);
}

NK_API void nk_input_unicode(struct nk_context *ctx, nk_rune unicode)
{
    nk_glyph rune;
    NK_ASSERT(ctx);
    if (!ctx) return;
    nk_utf_encode(unicode, rune, NK_UTF_SIZE);
    nk_input_glyph(ctx, rune);
}

// ----------------------------------------------------------------------------------------
NK_API nk_bool nk_input_click_in_rect(const struct nk_input *i, enum nk_buttons id, struct nk_rect b)
{
    if (!i) return nk_false;
    const struct nk_vec2 pos = i->mouse.clicked_pos[id];
    return NK_INBOX(pos.x, pos.y, b.x, b.y, b.w, b.h);
}

NK_API nk_bool nk_input_is_mouse_hovering_rect(const struct nk_input *i, struct nk_rect rect)
{
    if (!i) return nk_false;
    return NK_INBOX(i->mouse.pos.x, i->mouse.pos.y, rect.x, rect.y, rect.w, rect.h);
}

NK_API nk_bool nk_input_is_mouse_prev_hovering_rect(const struct nk_input *i, struct nk_rect rect)
{
    if (!i) return nk_false;
    return NK_INBOX(i->mouse.prev.x, i->mouse.prev.y, rect.x, rect.y, rect.w, rect.h);
}

NK_API nk_bool nk_input_mouse_clicked(const struct nk_input *i, enum nk_buttons id, struct nk_rect rect)
{
    if (!i) return nk_false;
    if (!nk_input_is_mouse_hovering_rect(i, rect)) return nk_false;
    return nk_input_click_in_rect(i, id, rect) && nk_input_is_mouse_released(i, id);
}

//-------------------------------------------------------------------------------------
NK_API nk_bool nk_input_is_mouse_pressed(const struct nk_input *i, enum nk_buttons id)
{
    // (i->mouse.buttons[id].down && i->mouse.buttons[id].clicked)
    return minimalMousePressed(id);
}

NK_API nk_bool nk_input_is_mouse_released(const struct nk_input *i, enum nk_buttons id)
{
    // (!i->mouse.buttons[id].down && i->mouse.buttons[id].clicked)
    return minimalMouseReleased(id);
}

NK_API nk_bool nk_input_is_mouse_down(const struct nk_input* i, enum nk_buttons id)
{
    // i->mouse.buttons[id].down
    return minimalMouseDown(id);
}

int input_map[][2] = {
    [NK_KEY_DEL]            = { MINIMAL_KEY_DELETE, 0 },
    [NK_KEY_ENTER]          = { MINIMAL_KEY_ENTER, 0 },
    [NK_KEY_TAB]            = { MINIMAL_KEY_TAB, 0 },
    [NK_KEY_BACKSPACE]      = { MINIMAL_KEY_BACKSPACE, 0 },
    [NK_KEY_UP]             = { MINIMAL_KEY_UP, 0 },
    [NK_KEY_DOWN]           = { MINIMAL_KEY_DOWN, 0 },
    [NK_KEY_TEXT_START]     = { MINIMAL_KEY_HOME, 0 },
    [NK_KEY_TEXT_END]       = { MINIMAL_KEY_END, 0 },
    [NK_KEY_SCROLL_START]   = { MINIMAL_KEY_HOME, 0 },
    [NK_KEY_SCROLL_END]     = { MINIMAL_KEY_END, 0 },
    [NK_KEY_SCROLL_DOWN]    = { MINIMAL_KEY_PAGE_DOWN, 0 },
    [NK_KEY_SCROLL_UP]      = { MINIMAL_KEY_PAGE_UP, 0 },
    [NK_KEY_COPY]            = { MINIMAL_KEY_C, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_PASTE]           = { MINIMAL_KEY_V, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_CUT]             = { MINIMAL_KEY_X, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_TEXT_UNDO]       = { MINIMAL_KEY_Z, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_TEXT_REDO]       = { MINIMAL_KEY_R, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_TEXT_WORD_LEFT]  = { MINIMAL_KEY_LEFT, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_TEXT_WORD_RIGHT] = { MINIMAL_KEY_RIGHT, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_TEXT_LINE_START] = { MINIMAL_KEY_B, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_TEXT_LINE_END]   = { MINIMAL_KEY_E, MINIMAL_KEY_MOD_CONTROL },
    [NK_KEY_LEFT]            = { MINIMAL_KEY_LEFT, 0 },
    [NK_KEY_RIGHT]           = { MINIMAL_KEY_RIGHT, 0 },
};

NK_API nk_bool nk_input_is_key_pressed(const struct nk_input *i, enum nk_keys key)
{
    int* key_mod = input_map[key];
    return minimalKeyPressed(key_mod[0]) && minimalKeyModActive(key_mod[1]);
}

NK_API nk_bool nk_input_is_key_released(const struct nk_input *i, enum nk_keys key)
{
    int* key_mod = input_map[key];
    return minimalKeyReleased(key_mod[0]) && minimalKeyModActive(key_mod[1]);
}

NK_API nk_bool nk_input_is_key_down(const struct nk_input *i, enum nk_keys key)
{
    int* key_mod = input_map[key];
    return minimalKeyDown(key_mod[0]) && minimalKeyModActive(key_mod[1]);
}

