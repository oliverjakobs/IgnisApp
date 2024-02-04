#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ==============================================================
 *
 *                          BUTTON
 *
 * ===============================================================*/
NK_LIB void 
nk_draw_symbol(nk_command_buffer* out, struct nk_rect content, nk_symbol type, float line_width, nk_color color)
{
    struct nk_vec2 points[3];
    switch (type)
    {
    case NK_SYMBOL_CIRCLE_SOLID:
        nk_fill_circle(out, content, color);
        break;
    case NK_SYMBOL_CIRCLE_OUTLINE:
        nk_stroke_circle(out, content, line_width, color);
        break;
    case NK_SYMBOL_RECT_SOLID:
        nk_fill_rect(out, content, 0, color);
        break;
    case NK_SYMBOL_RECT_OUTLINE:
        nk_stroke_rect(out, content, 0, line_width, color);
        break;
    case NK_SYMBOL_TRIANGLE_UP:
        nk_triangle_from_direction(points, content, 0, 0, NK_UP);
        nk_fill_triangle(out, points[0], points[1], points[2], color);
        break;
    case NK_SYMBOL_TRIANGLE_DOWN:
        nk_triangle_from_direction(points, content, 0, 0, NK_DOWN);
        nk_fill_triangle(out, points[0], points[1], points[2], color);
        break;
    case NK_SYMBOL_TRIANGLE_LEFT:
        nk_triangle_from_direction(points, content, 0, 0, NK_LEFT);
        nk_fill_triangle(out, points[0], points[1], points[2], color);
        break;
    case NK_SYMBOL_TRIANGLE_RIGHT:
        nk_triangle_from_direction(points, content, 0, 0, NK_RIGHT);
        nk_fill_triangle(out, points[0], points[1], points[2], color);
        break;
    case NK_SYMBOL_CHECK:
        nk_stroke_line(out, content.x, content.y, content.x + content.w, content.y + content.h, line_width, color);
        nk_stroke_line(out, content.x, content.y + content.h, content.x + content.w, content.y, line_width, color);
        break;
    }
}

NK_LIB nk_bool nk_button_behavior(nk_flags *state, struct nk_rect r, const struct nk_input *i, nk_button_type behavior)
{
    if (!i) return 0;
    nk_widget_state_reset(state);

    nk_bool clicked = nk_false;
    if (nk_input_mouse_hover(i, r))
    {
        *state = NK_WIDGET_STATE_HOVER;

        if (nk_input_mouse_down(i, NK_BUTTON_LEFT))
            *state = NK_WIDGET_STATE_ACTIVE;

        if (nk_input_click_in_rect(i, NK_BUTTON_LEFT, r))
        {
            if (behavior == NK_BUTTON_DEFAULT)  clicked = nk_input_mouse_released(i, NK_BUTTON_LEFT);
            else                                clicked = nk_input_mouse_down(i, NK_BUTTON_LEFT);
        }
    }

    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(i, r))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(i, r))
        *state |= NK_WIDGET_STATE_LEFT;

    return clicked;
}

static void
nk_draw_button_background(nk_command_buffer *out, struct nk_rect bounds, nk_flags state, const nk_style_button *style)
{
    const nk_style_item *background = &style->bg_normal;
    if (state & NK_WIDGET_STATE_HOVER)        background = &style->bg_hover;
    else if (state & NK_WIDGET_STATE_ACTIVE)  background = &style->bg_active;

    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, bounds, &background->image, nk_white);
    else
        nk_fill_rect_border(out, bounds, style->rounding, background->color, style->border, style->border_color);
}

NK_LIB void
nk_draw_button_text(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const nk_style_button* style,
    const char* txt, int len, nk_flags text_alignment, const struct nk_font* font)
{
    nk_draw_button_background(out, bounds, state, style);

    nk_style_text text = { 0 };
    text.alignment = text_alignment;
    if (state & NK_WIDGET_STATE_ACTIVE)     text.color = style->fg_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.color = style->fg_hover;
    else                                    text.color = style->fg_normal;

    struct nk_rect content = {
        .x = bounds.x + style->padding.x + style->rounding + style->border,
        .y = bounds.y + style->padding.y + style->rounding + style->border,
        .w = bounds.w - 2 * (style->padding.x + style->rounding + style->border),
        .h = bounds.h - 2 * (style->padding.y + style->rounding + style->border),
    };

    nk_widget_text(out, content, txt, len, &text, font);
}

NK_LIB void
nk_draw_button_symbol(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const nk_style_button* style,
    nk_symbol type)
{
    nk_draw_button_background(out, bounds, state, style);

    nk_color color;
    if (state & NK_WIDGET_STATE_ACTIVE)     color = style->fg_active;
    else if (state & NK_WIDGET_STATE_HOVER) color = style->fg_hover;
    else                                    color = style->fg_normal;

    struct nk_rect content = {
        .x = bounds.x + style->padding.x + style->rounding + style->border,
        .y = bounds.y + style->padding.y + style->rounding + style->border,
        .w = bounds.w - 2 * (style->padding.x + style->rounding + style->border),
        .h = bounds.h - 2 * (style->padding.y + style->rounding + style->border),
    };

    nk_draw_symbol(out, content, type, 1, color);
}

NK_LIB void
nk_draw_button_text_symbol(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const nk_style_button* style,
    const char* str, int len, nk_symbol symbol, nk_flags align, const struct nk_font* font)
{
    nk_draw_button_background(out, bounds, state, style);

    nk_style_text text = { 0 };
    text.alignment = style->text_alignment;
    if (state & NK_WIDGET_STATE_ACTIVE)     text.color = style->fg_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.color = style->fg_hover;
    else                                    text.color = style->fg_normal;

    struct nk_rect content = {
        .x = bounds.x + style->padding.x + style->rounding + style->border,
        .y = bounds.y + style->padding.y + style->rounding + style->border,
        .w = bounds.w - 2 * (style->padding.x + style->rounding + style->border),
        .h = bounds.h - 2 * (style->padding.y + style->rounding + style->border),
    };

    struct nk_rect icon = {
        .x = content.x,
        .y = content.y + (content.h - font->height) / 2,
        .w = font->height,
        .h = font->height
    };

    content.w -= icon.w;

    if (align & NK_TEXT_ALIGN_RIGHT)  icon.x += content.w;
    else                              content.x += icon.w;

    nk_draw_symbol(out, icon, symbol, 1, text.color);
    nk_widget_text(out, content, str, len, &text, font);
}

NK_LIB nk_bool
nk_do_button_text(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    const char *string, int len, nk_flags align, nk_button_type behavior,
    const nk_style_button *style, const struct nk_input *in, const struct nk_font *font)
{
    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(string);
    NK_ASSERT(font);
    if (!out || !style || !font || !string) return nk_false;

    nk_bool clicked = nk_button_behavior(state, bounds, in, behavior);

    nk_draw_button_text(out, bounds, *state, style, string, len, align, font);

    return clicked;
}

NK_LIB nk_bool
nk_do_button_symbol(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    nk_symbol symbol, nk_button_type behavior,
    const nk_style_button *style, const struct nk_input *in)
{
    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style || !state) return nk_false;

    nk_bool clicked = nk_button_behavior(state, bounds, in, behavior);

    nk_draw_button_symbol(out, bounds, *state, style, symbol);

    return clicked;
}

NK_LIB nk_bool
nk_do_button_text_symbol(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    nk_symbol symbol, const char *str, int len, nk_flags align, nk_button_type behavior,
    const nk_style_button *style, const struct nk_font *font, const struct nk_input *in)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(font);
    if (!out || !style || !font) return nk_false;

    nk_bool clicked = nk_button_behavior(state, bounds, in, behavior);

    /* draw button */
    nk_draw_button_text_symbol(out, bounds, *state, style, str, len, symbol, align, font);

    return clicked;
}

NK_API nk_bool nk_button_text(struct nk_context* ctx, const char* title, int len)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return nk_false;

    nk_flags state = 0;
    return nk_do_button_text(&state, &win->buffer, bounds, title, len, style->text_alignment, ctx->button_behavior, style, in, ctx->style.font);
}

NK_API nk_bool nk_button_label(struct nk_context *ctx, const char *title)
{
    return nk_button_text(ctx, title, nk_strlen(title));
}

NK_API nk_bool nk_button_symbol(struct nk_context* ctx, nk_symbol symbol)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return nk_false;

    nk_flags state = 0;
    return nk_do_button_symbol(&state, &win->buffer, bounds, symbol, ctx->button_behavior, style, in);
}

NK_API nk_bool nk_button_symbol_text(struct nk_context *ctx, nk_symbol symbol, const char *text, int len, nk_flags align)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return nk_false;

    nk_flags state = 0;
    return nk_do_button_text_symbol(&state, &win->buffer, bounds, symbol, text, len, align, ctx->button_behavior, style, ctx->style.font, in);
}

NK_API nk_bool nk_button_symbol_label(struct nk_context *ctx, nk_symbol symbol, const char *label, nk_flags align)
{
    return nk_button_symbol_text(ctx, symbol, label, nk_strlen(label), align);
}
