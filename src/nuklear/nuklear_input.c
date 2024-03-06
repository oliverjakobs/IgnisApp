#include "nuklear.h"
#include "nuklear_internal.h"

#include <minimal.h>

/* ===============================================================
 *
 *                          INPUT
 *
 * ===============================================================*/
void nk_input_update_mouse(struct nk_input* in, struct nk_vec2 pos, struct nk_vec2 scroll)
{
    /* update scroll */
    in->scroll_delta = scroll;

    /* update mouse motion */
    in->mouse_prev = in->mouse_pos;
    in->mouse_pos = pos;

    in->mouse_delta.x = in->mouse_pos.x - in->mouse_prev.x;
    in->mouse_delta.y = in->mouse_pos.y - in->mouse_prev.y;

    /* update click pos */
    for (int i = 0; i < NK_BUTTON_MAX; ++i)
    {
        if (nk_input_mouse_pressed(in, i))
            in->clicked_pos[i] = pos;
    }
}

static void nk_input_glyph(struct nk_input* in, const nk_glyph glyph)
{
    nk_rune unicode;
    int len = nk_utf_decode(glyph, &unicode, NK_UTF_SIZE);
    if (len && ((in->text_len + len) < NK_INPUT_MAX))
    {
        nk_utf_encode(unicode, &in->text[in->text_len], NK_INPUT_MAX - in->text_len);
        in->text_len += len;
    }
}

void nk_input_update_text(struct nk_input* in, const char* text, int len)
{
    in->text_len = 0;

    nk_glyph glyph;
    for (int i = 0; i < len; ++i)
    {
        glyph[0] = text[i];
        nk_input_glyph(in, glyph);
    }
}

void nk_input_update_text_unicode(struct nk_input* in, const unsigned int* text, int len)
{
    in->text_len = 0;

    nk_glyph rune;
    for (int i = 0; i < len; ++i)
    {
        nk_utf_encode(text[i], rune, NK_UTF_SIZE);
        nk_input_glyph(in, rune);
    }
}

// ----------------------------------------------------------------------------------------
NK_API nk_bool nk_input_click_in_rect(const struct nk_input *i, enum nk_buttons id, struct nk_rect b)
{
    if (!i) return nk_false;
    const struct nk_vec2 pos = i->clicked_pos[id];
    return NK_INBOX(pos.x, pos.y, b.x, b.y, b.w, b.h);
}

NK_API nk_bool nk_input_mouse_prev_hover(const struct nk_input* i, struct nk_rect rect)
{
    if (!i) return nk_false;
    return NK_INBOX(i->mouse_prev.x, i->mouse_prev.y, rect.x, rect.y, rect.w, rect.h);
}

NK_API nk_bool nk_input_mouse_hover(const struct nk_input* i, struct nk_rect rect)
{
    if (!i) return nk_false;
    return NK_INBOX(i->mouse_pos.x, i->mouse_pos.y, rect.x, rect.y, rect.w, rect.h);
}

NK_API nk_bool nk_input_mouse_clicked(const struct nk_input *i, enum nk_buttons id, struct nk_rect rect)
{
    if (!nk_input_mouse_hover(i, rect)) return nk_false;
    return nk_input_click_in_rect(i, id, rect) && nk_input_mouse_released(i, id);
}

NK_API nk_bool nk_input_mouse_moved(const struct nk_input* i)
{
    return (i->mouse_delta.x != 0.0f || i->mouse_delta.y != 0.0f);
}

//-------------------------------------------------------------------------------------
NK_API nk_bool nk_input_mouse_pressed(const struct nk_input *i, enum nk_buttons id)
{
    if (!i) return nk_false;
    // (i->mouse.buttons[id].down && i->mouse.buttons[id].clicked)
    return minimalMousePressed(id);
}

NK_API nk_bool nk_input_mouse_released(const struct nk_input *i, enum nk_buttons id)
{
    if (!i) return nk_false;
    // (!i->mouse.buttons[id].down && i->mouse.buttons[id].clicked)
    return minimalMouseReleased(id);
}

NK_API nk_bool nk_input_mouse_down(const struct nk_input* i, enum nk_buttons id)
{
    if (!i) return nk_false;
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

NK_API nk_bool nk_input_key_pressed(const struct nk_input *i, enum nk_keys key)
{
    if (!i) return nk_false;
    int* key_mod = input_map[key];
    return minimalKeyPressed(key_mod[0]) && minimalKeyModActive(key_mod[1]);
}

NK_API nk_bool nk_input_key_released(const struct nk_input *i, enum nk_keys key)
{
    if (!i) return nk_false;
    int* key_mod = input_map[key];
    return minimalKeyReleased(key_mod[0]) && minimalKeyModActive(key_mod[1]);
}

NK_API nk_bool nk_input_key_down(const struct nk_input *i, enum nk_keys key)
{
    if (!i) return nk_false;
    int* key_mod = input_map[key];
    return minimalKeyDown(key_mod[0]) && minimalKeyModActive(key_mod[1]);
}
