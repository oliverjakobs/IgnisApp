#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              TOGGLE
 *
 * ===============================================================*/
NK_LIB nk_bool
nk_toggle_behavior(const struct nk_input *in, struct nk_rect select, nk_flags *state, nk_bool active)
{
    nk_widget_state_reset(state);
    if (nk_button_behavior(state, select, in, NK_BUTTON_DEFAULT))
    {
        *state = NK_WIDGET_STATE_ACTIVE;
        active = !active;
    }

    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, select))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, select))
        *state |= NK_WIDGET_STATE_LEFT;

    return active;
}

NK_LIB void
nk_draw_toggle(struct nk_command_buffer *out,
    nk_flags state, const struct nk_style_toggle *style, nk_bool active,
    const struct nk_rect *label, const struct nk_rect *selector,
    const struct nk_rect *cursors, const char *string, int len, enum nk_toggle_type type,
    const struct nk_user_font *font)
{
    const struct nk_style_item *background;
    const struct nk_style_item *cursor;
    struct nk_text text;
    text.padding.x = 0;
    text.padding.y = 0;
    text.background = style->text_background;

    /* select correct colors/images */
    if (state & NK_WIDGET_STATE_HOVER)
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_hover;
    }
    else if (state & NK_WIDGET_STATE_ACTIVED)
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.text = style->text_active;
    }
    else
    {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.text = style->text_normal;
    }

    /* draw background and cursor */
    if (background->type == NK_STYLE_ITEM_IMAGE)
    {
        nk_draw_image(out, *selector, &background->data.image, nk_white);
    }
    else if (type == NK_TOGGLE_CHECK)
    {
        nk_fill_rect(out, *selector, 0, style->border_color);
        nk_fill_rect(out, nk_shrink_rect(*selector, style->border), 0, background->data.color);
    }
    else if (type == NK_TOGGLE_OPTION)
    {
        nk_fill_circle(out, *selector, style->border_color);
        nk_fill_circle(out, nk_shrink_rect(*selector, style->border), background->data.color);
    }

    if (active)
    {
        if (cursor->type == NK_STYLE_ITEM_IMAGE) nk_draw_image(out, *cursors, &cursor->data.image, nk_white);
        else if (type == NK_TOGGLE_CHECK)        nk_fill_rect(out, *cursors, 0, cursor->data.color);
        else if (type == NK_TOGGLE_OPTION)       nk_fill_circle(out, *cursors, cursor->data.color);
    }
    nk_widget_text(out, *label, string, len, &text, NK_TEXT_LEFT, font);
}

NK_LIB nk_bool
nk_do_toggle(nk_flags *state, struct nk_command_buffer *out, struct nk_rect r, nk_bool *active,
    const char *str, int len, enum nk_toggle_type type,
    const struct nk_style_toggle *style, const struct nk_input *in,
    const struct nk_user_font *font)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(font);
    if (!out || !style || !font || !active)
        return nk_false;

    r.w = NK_MAX(r.w, font->height + 2 * style->padding.x);
    r.h = NK_MAX(r.h, font->height + 2 * style->padding.y);

    /* add additional touch padding for touch screen devices */
    struct nk_rect bounds = {
        .x = r.x - style->touch_padding.x,
        .y = r.y - style->touch_padding.y,
        .w = r.w + 2 * style->touch_padding.x,
        .h = r.h + 2 * style->touch_padding.y
    };

    /* calculate the selector space */
    struct nk_rect select = {
        .x = r.x,
        .y = r.y + (r.h - font->height) / 2.0f,
        .w = font->height,
        .h = font->height
    };

    /* calculate the bounds of the cursor inside the selector */
    struct nk_rect cursor = {
        .x = select.x + style->padding.x + style->border,
        .y = select.y + style->padding.y + style->border,
        .w = select.w - 2 * (style->padding.x + style->border),
        .h = select.h - 2 * (style->padding.y + style->border)
    };

    /* label behind the selector */
    struct nk_rect label = {
        .x = select.x + select.w + style->spacing,
        .y = select.y,
        .w = NK_MAX(r.x + r.w, label.x) - label.x,
        .h = select.w
    };

    /* update selector */
    nk_bool was_active = *active;
    *active = nk_toggle_behavior(in, bounds, state, *active);

    /* draw selector */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_toggle(out, *state, style, *active, &label, &select, &cursor, str, len, type, font);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return (was_active != *active);
}

nk_bool nk_toggle_text(struct nk_context* ctx, enum nk_toggle_type type, const char* text, int len, nk_bool* active)
{
    NK_ASSERT(ctx);
    NK_ASSERT(text);
    NK_ASSERT(active);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !text || !active || !ctx->current || !ctx->current->layout)
        return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return nk_false;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    nk_flags state = 0;
    return  nk_do_toggle(&state, &win->buffer, bounds, active, text, len, type, &style->checkbox, in, style->font);
}

/*----------------------------------------------------------------
 *
 *                          CHECKBOX
 *
 * --------------------------------------------------------------*/
NK_API nk_bool
nk_checkbox_text(struct nk_context *ctx, const char *text, int len, nk_bool *active)
{
    return nk_toggle_text(ctx, NK_TOGGLE_CHECK, text, len, active);
}

NK_API nk_bool nk_checkbox_label(struct nk_context* ctx, const char* label, nk_bool* active)
{
    return nk_checkbox_text(ctx, label, nk_strlen(label), active);
}

NK_API nk_bool
nk_checkbox_flags_text(struct nk_context* ctx, const char* text, int len, nk_flags* flags, nk_flags value)
{
    NK_ASSERT(flags);
    if (!flags) return 0;

    nk_bool active = *flags & value;
    nk_bool toggle = nk_toggle_text(ctx, NK_TOGGLE_CHECK, text, len, &active);
    if (active) *flags |= value;
    else        *flags &= ~value;

    return toggle;
}

NK_API nk_bool
nk_checkbox_flags_label(struct nk_context *ctx, const char *label, nk_flags *flags, nk_flags value)
{
    return nk_checkbox_flags_text(ctx, label, nk_strlen(label), flags, value);
}

/*----------------------------------------------------------------
 *
 *                          RADIO BUTTON
 *
 * --------------------------------------------------------------*/
NK_API nk_bool nk_radio_text(struct nk_context *ctx, const char *text, int len, unsigned int* option, unsigned int value)
{
    nk_bool active = *option == value;
    nk_bool toggle = nk_toggle_text(ctx, NK_TOGGLE_OPTION, text, len, &active);
    if (active) *option = value;
    return toggle;
}

NK_API nk_bool nk_radio_label(struct nk_context *ctx, const char *label, unsigned int* option, unsigned int value)
{
    return nk_radio_text(ctx, label, nk_strlen(label), option, value);
}

