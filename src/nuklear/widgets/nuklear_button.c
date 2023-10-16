#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ==============================================================
 *
 *                          BUTTON
 *
 * ===============================================================*/
NK_LIB void 
nk_draw_symbol(nk_command_buffer* out, struct nk_rect content, nk_symbol type, float line_width, struct nk_color color)
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
        *state = NK_WIDGET_STATE_HOVERED;

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
nk_draw_button_background(nk_command_buffer *out, struct nk_rect bounds, nk_flags state, const struct nk_style_button *style)
{
    const struct nk_style_item *background = &style->normal;
    if (state & NK_WIDGET_STATE_HOVER)          background = &style->hover;
    else if (state & NK_WIDGET_STATE_ACTIVED)   background = &style->active;

    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, bounds, &background->image, nk_white);
    else
        nk_fill_rect_border(out, bounds, style->rounding, background->color, style->border, style->border_color);
}

NK_LIB void
nk_draw_button_text(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const struct nk_style_button* style,
    const char* txt, int len, nk_flags text_alignment, const struct nk_font* font)
{
    nk_draw_button_background(out, bounds, state, style);

    struct nk_style_text text = { 0 };
    text.alignment = text_alignment;
    if (state & NK_WIDGET_STATE_ACTIVED)    text.color = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.color = style->text_hover;
    else                                    text.color = style->text_normal;

    struct nk_rect content = {
        .x = bounds.x + style->padding.x + style->rounding + style->border,
        .y = bounds.y + style->padding.y + style->rounding + style->border,
        .w = bounds.w - 2 * (style->padding.x + style->rounding + style->border),
        .h = bounds.h - 2 * (style->padding.y + style->rounding + style->border),
    };

    nk_widget_text(out, content, txt, len, &text, font);
}

NK_LIB void
nk_draw_button_symbol(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const struct nk_style_button* style,
    nk_symbol type)
{
    nk_draw_button_background(out, bounds, state, style);

    struct nk_color color;
    if (state & NK_WIDGET_STATE_ACTIVED)    color = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) color = style->text_hover;
    else                                    color = style->text_normal;

    struct nk_rect content = {
        .x = bounds.x + style->padding.x + style->rounding + style->border,
        .y = bounds.y + style->padding.y + style->rounding + style->border,
        .w = bounds.w - 2 * (style->padding.x + style->rounding + style->border),
        .h = bounds.h - 2 * (style->padding.y + style->rounding + style->border),
    };

    nk_draw_symbol(out, content, type, 1, color);
}

NK_LIB void
nk_draw_button_text_symbol(nk_command_buffer* out, struct nk_rect bounds, struct nk_rect label, struct nk_rect icon, nk_flags state,
    const struct nk_style_button* style, const char* str, int len, nk_symbol sym, const struct nk_font* font)
{
    nk_draw_button_background(out, bounds, state, style);

    struct nk_style_text text = { 0 };
    text.alignment = style->text_alignment;
    if (state & NK_WIDGET_STATE_ACTIVED)    text.color = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.color = style->text_hover;
    else                                    text.color = style->text_normal;

    nk_draw_symbol(out, icon, sym, 1, text.color);
    nk_widget_text(out, label, str, len, &text, font);
}

NK_LIB nk_bool
nk_do_button_text(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    const char *string, int len, nk_flags align, nk_button_type behavior,
    const struct nk_style_button *style, const struct nk_input *in, const struct nk_font *font)
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
    const struct nk_style_button *style, const struct nk_input *in)
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
    const struct nk_style_button *style, const struct nk_font *font, const struct nk_input *in)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(font);
    if (!out || !style || !font) return nk_false;

    nk_bool clicked = nk_button_behavior(state, bounds, in, behavior);

    struct nk_rect content = {
        .x = bounds.x + style->padding.x + style->rounding + style->border,
        .y = bounds.y + style->padding.y + style->rounding + style->border,
        .w = bounds.w - 2 * (style->padding.x + style->rounding + style->border),
        .h = bounds.h - 2 * (style->padding.y + style->rounding + style->border),
    };

    struct nk_rect sym = {
        .x = content.x + 2 * style->padding.x,
        .y = content.y + (content.h/2) - font->height/2,
        .w = font->height,
        .h = font->height
    };

    if (align & NK_TEXT_ALIGN_LEFT)
    {
        float x = (content.x + content.w) - (2 * style->padding.x + sym.w);
        sym.x = NK_MAX(x, 0);
    };

    /*
    struct nk_rect icon = {
        .x = bounds.x + 2 * style->padding.x,
        .y = bounds.y + 2 * style->padding.y,
        .w = bounds.h - 2 * style->padding.y,
        .h = bounds.h - 2 * style->padding.y
    };

    if (align & NK_TEXT_ALIGN_LEFT)
    {
        float x = (bounds.x + bounds.w) - (2 * style->padding.x + icon.w);
        icon.x = NK_MAX(x, 0);
    }

    icon.x += style->image_padding.x;
    icon.y += style->image_padding.y;
    icon.w -= 2 * style->image_padding.x;
    icon.h -= 2 * style->image_padding.y;
    */

    /* draw button */
    nk_draw_button_text_symbol(out, bounds, content, sym, *state, style, str, len, symbol, font);

    return clicked;
}

NK_API nk_bool nk_button_text(struct nk_context* ctx, const char* title, int len)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style_button* style = &ctx->style.button;

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
    const struct nk_style_button* style = &ctx->style.button;

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
    const struct nk_style_button* style = &ctx->style.button;

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


NK_API nk_bool
nk_button_push_behavior(struct nk_context* ctx, nk_button_type behavior)
{
    NK_ASSERT(ctx);
    if (!ctx) return nk_false;

    struct nk_config_stack_button_behavior* button_stack = &ctx->stacks.button_behaviors;
    NK_ASSERT(button_stack->head < (int)NK_LEN(button_stack->elements));
    if (button_stack->head >= (int)NK_LEN(button_stack->elements))
        return nk_false;

    struct nk_config_stack_button_behavior_element* element = &button_stack->elements[button_stack->head++];
    element->address = &ctx->button_behavior;
    element->old_value = ctx->button_behavior;
    ctx->button_behavior = behavior;
    return nk_true;
}

NK_API nk_bool
nk_button_pop_behavior(struct nk_context* ctx)
{
    NK_ASSERT(ctx);
    if (!ctx) return nk_false;

    struct nk_config_stack_button_behavior* button_stack = &ctx->stacks.button_behaviors;
    NK_ASSERT(button_stack->head > 0);
    if (button_stack->head < 1)
        return nk_false;

    struct nk_config_stack_button_behavior_element* element = &button_stack->elements[--button_stack->head];
    *element->address = element->old_value;
    return nk_true;
}