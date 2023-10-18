#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              SELECTABLE
 *
 * ===============================================================*/
NK_LIB void
nk_draw_selectable(struct nk_command_buffer *out, nk_flags state, const struct nk_style_selectable *style,
    nk_bool selected, const struct nk_rect bounds, const char *string, int len, nk_flags align, const struct nk_font *font)
{
    nk_style_text text;
    text.padding = style->padding;
    text.alignment = align;

    /* select correct colors/images */
    const nk_style_item* background;
    if (!selected)
    {
        if (state & NK_WIDGET_STATE_ACTIVED)
        {
            background = &style->pressed;
            text.color = style->text_pressed;
        }
        else if (state & NK_WIDGET_STATE_HOVER)
        {
            background = &style->hover;
            text.color = style->text_hover;
        }
        else
        {
            background = &style->normal;
            text.color = style->text_normal;
        }
    }
    else
    {
        if (state & NK_WIDGET_STATE_ACTIVED)
        {
            background = &style->pressed_active;
            text.color = style->text_pressed_active;
        }
        else if (state & NK_WIDGET_STATE_HOVER)
        {
            background = &style->hover_active;
            text.color = style->text_hover_active;
        }
        else
        {
            background = &style->normal_active;
            text.color = style->text_normal_active;
        }
    }

    /* draw selectable background and text */
    if  (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, bounds, &background->image, nk_white);
    else
        nk_fill_rect(out, bounds, style->rounding, background->color);

    nk_widget_text(out, bounds, string, len, &text, font);
}

NK_LIB nk_bool
nk_do_selectable(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    const char *str, int len, nk_flags align, nk_bool selected,
    const struct nk_style_selectable *style, const struct nk_input *in, const struct nk_font *font)
{
    NK_ASSERT(state);
    NK_ASSERT(out);
    NK_ASSERT(str);
    NK_ASSERT(len);
    NK_ASSERT(style);
    NK_ASSERT(font);

    if (!state || !out || !str || !len || !style || !font) return selected;

    /* update button */
    if (nk_button_behavior(state, bounds, in, NK_BUTTON_DEFAULT))
        selected = !selected;

    /* draw selectable */
    nk_draw_selectable(out, *state, style, selected, bounds, str, len, align, font);

    return selected;
}

NK_API nk_bool
nk_selectable_text(struct nk_context *ctx, const char *str, int len, nk_flags align, nk_bool selected)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return selected;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return selected;

    nk_flags state = 0;
    return nk_do_selectable(&state, &win->buffer, bounds, str, len, align, selected, &style->selectable, in, style->font);
}

NK_API nk_bool nk_selectable_label(struct nk_context* ctx, const char* str, nk_flags align, nk_bool selected)
{
    return nk_selectable_text(ctx, str, nk_strlen(str), align, selected);
}

