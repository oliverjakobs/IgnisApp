#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              TOGGLE
 *
 * ===============================================================*/
NK_LIB void
nk_draw_toggle(nk_command_buffer *out, struct nk_rect bounds, nk_flags state, const nk_style_toggle *style,
    nk_bool active, const char *string, int len, nk_toggle_type type, const nk_font *font)
{
    nk_style_text text = { 0 };
    text.alignment = NK_TEXT_LEFT;

    const nk_style_item* background;
    const nk_style_item* cursor;
    /* select correct colors/images */
    if (state & NK_WIDGET_STATE_HOVER)
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.color = style->text_hover;
    }
    else if (state & NK_WIDGET_STATE_ACTIVE)
    {
        background = &style->hover;
        cursor = &style->cursor_hover;
        text.color = style->text_active;
    }
    else
    {
        background = &style->normal;
        cursor = &style->cursor_normal;
        text.color = style->text_normal;
    }

    bounds.w = NK_MAX(bounds.w, font->height + 2 * style->padding.x);
    bounds.h = NK_MAX(bounds.h, font->height + 2 * style->padding.y);

    /* calculate the selector space */
    struct nk_rect select = {
        .x = bounds.x,
        .y = bounds.y + (bounds.h - font->height) / 2.0f,
        .w = font->height,
        .h = font->height
    };

    /* calculate the bounds of the cursor inside the selector */
    struct nk_rect check = {
        .x = select.x + style->padding.x + style->border,
        .y = select.y + style->padding.y + style->border,
        .w = select.w - 2 * (style->padding.x + style->border),
        .h = select.h - 2 * (style->padding.y + style->border)
    };

    /* label behind the selector */
    struct nk_rect label = {
        .x = select.x + select.w + style->spacing,
        .y = select.y,
        .w = NK_MAX(bounds.x + bounds.w, label.x) - label.x,
        .h = select.w
    };

    /* draw background and cursor */
    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, select, &background->image, nk_white);
    else if (type == NK_TOGGLE_CHECK)
        nk_fill_rect_border(out, select, 0, background->color, style->border, style->border_color);
    else if (type == NK_TOGGLE_OPTION)
        nk_fill_circle_border(out, select, background->color, style->border, style->border_color);

    if (active)
    {
        if (cursor->type == NK_STYLE_ITEM_IMAGE) nk_draw_image(out, check, &cursor->image, nk_white);
        else if (type == NK_TOGGLE_CHECK)        nk_draw_symbol(out, check, NK_SYMBOL_CHECK, 3, cursor->color);
        else if (type == NK_TOGGLE_OPTION)       nk_fill_circle(out, check, cursor->color);
    }
    nk_widget_text(out, label, string, len, &text, font);
}

nk_bool nk_toggle_text(struct nk_context* ctx, nk_toggle_type type, const char* text, int len, nk_bool active)
{
    NK_ASSERT(ctx);
    NK_ASSERT(text);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !text || !ctx->current || !ctx->current->layout) return active;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    const struct nk_input *in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return active;

    /* update toggle */
    nk_flags state = 0;
    if (nk_button_behavior(&state, bounds, in, NK_BUTTON_DEFAULT))
        active = !active;

    /* draw toggle */
    nk_draw_toggle(&win->buffer, bounds, state, &style->checkbox, active, text, len, type, style->font);

    return active;
}

/*----------------------------------------------------------------
 *
 *                          CHECKBOX
 *
 * --------------------------------------------------------------*/
NK_API nk_bool
nk_checkbox_text(struct nk_context *ctx, const char *text, int len, nk_bool active)
{
    return nk_toggle_text(ctx, NK_TOGGLE_CHECK, text, len, active);
}

NK_API nk_bool nk_checkbox_label(struct nk_context* ctx, const char* label, nk_bool active)
{
    return nk_toggle_text(ctx, NK_TOGGLE_CHECK, label, nk_strlen(label), active);
}

NK_API nk_flags
nk_checkbox_flags_text(struct nk_context* ctx, const char* text, int len, nk_flags flags, nk_flags value)
{
    nk_bool active = nk_toggle_text(ctx, NK_TOGGLE_CHECK, text, len, flags & value);
    if (active) flags |= value;
    else        flags &= ~value;

    return flags;
}

NK_API nk_flags
nk_checkbox_flags_label(struct nk_context *ctx, const char *label, nk_flags flags, nk_flags value)
{
    return nk_checkbox_flags_text(ctx, label, nk_strlen(label), flags, value);
}

/*----------------------------------------------------------------
 *
 *                          RADIO BUTTON
 *
 * --------------------------------------------------------------*/
NK_API nk_uint nk_radio_text(struct nk_context *ctx, const char *text, int len, nk_uint option, nk_uint value)
{
    if (nk_toggle_text(ctx, NK_TOGGLE_OPTION, text, len, option == value)) return value;
    return option;
}

NK_API nk_uint nk_radio_label(struct nk_context *ctx, const char *label, nk_uint option, nk_uint value)
{
    return nk_radio_text(ctx, label, nk_strlen(label), option, value);
}

