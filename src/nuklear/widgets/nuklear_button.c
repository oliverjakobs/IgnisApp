#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ==============================================================
 *
 *                          BUTTON
 *
 * ===============================================================*/
NK_LIB void
nk_draw_symbol(struct nk_command_buffer *out, enum nk_symbol_type type,
    struct nk_rect content, struct nk_color background, struct nk_color foreground,
    float border_width, const struct nk_user_font *font)
{
    switch (type)
    {
    case NK_SYMBOL_X:
    case NK_SYMBOL_UNDERSCORE:
    case NK_SYMBOL_PLUS:
    case NK_SYMBOL_MINUS:
    {
        /* single character text symbol */
        const char *X = (type == NK_SYMBOL_X) ? "x":
            (type == NK_SYMBOL_UNDERSCORE) ? "_":
            (type == NK_SYMBOL_PLUS) ? "+" : "-";

        struct nk_text text = {
            .padding = nk_vec2(0,0),
            .background = background,
            .text = foreground
        };
        nk_widget_text(out, content, X, 1, &text, NK_TEXT_CENTERED, font);
        break;
    }
    case NK_SYMBOL_CIRCLE_SOLID:
        nk_fill_circle(out, content, foreground);
        break;
    case NK_SYMBOL_CIRCLE_OUTLINE:
        nk_stroke_circle(out, content, border_width, foreground);
        break;
    case NK_SYMBOL_RECT_SOLID:
        nk_fill_rect(out, content, 0, foreground);
        break;
    case NK_SYMBOL_RECT_OUTLINE:
        nk_stroke_rect(out, content, 0, border_width, foreground);
        break;
    case NK_SYMBOL_TRIANGLE_UP:
    case NK_SYMBOL_TRIANGLE_DOWN:
    case NK_SYMBOL_TRIANGLE_LEFT:
    case NK_SYMBOL_TRIANGLE_RIGHT:
    {
        enum nk_heading heading = (type == NK_SYMBOL_TRIANGLE_RIGHT) ? NK_RIGHT :
            (type == NK_SYMBOL_TRIANGLE_LEFT) ? NK_LEFT:
            (type == NK_SYMBOL_TRIANGLE_UP) ? NK_UP: NK_DOWN;

        struct nk_vec2 points[3];
        nk_triangle_from_direction(points, content, 0, 0, heading);
        nk_fill_triangle(out, points[0], points[1], points[2], foreground);
        break;
    }
    default:
    case NK_SYMBOL_NONE:
    case NK_SYMBOL_MAX: break;
    }
}

NK_LIB nk_bool nk_button_behavior(nk_flags *state, struct nk_rect r, const struct nk_input *i, enum nk_button_behavior behavior)
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

NK_LIB const struct nk_style_item*
nk_draw_button(struct nk_command_buffer *out, const struct nk_rect *bounds, nk_flags state, const struct nk_style_button *style)
{
    const struct nk_style_item *background = &style->normal;

    if (state & NK_WIDGET_STATE_HOVER)          background = &style->hover;
    else if (state & NK_WIDGET_STATE_ACTIVED)   background = &style->active;

    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *bounds, &background->data.image, nk_white);
    else
        nk_fill_rect_border(out, *bounds, style->rounding, background->data.color, style->border, style->border_color);

    return background;
}

NK_LIB nk_bool
nk_do_button(nk_flags *state, struct nk_command_buffer *out, struct nk_rect r,
    const struct nk_style_button *style, const struct nk_input *in,
    enum nk_button_behavior behavior, struct nk_rect *content)
{
    NK_ASSERT(style);
    NK_ASSERT(state);
    NK_ASSERT(out);
    if (!out || !style || !state) return nk_false;

    /* calculate button content space */
    content->x = r.x + style->padding.x + style->border + style->rounding;
    content->y = r.y + style->padding.y + style->border + style->rounding;
    content->w = r.w - (2 * style->padding.x + style->border + style->rounding*2);
    content->h = r.h - (2 * style->padding.y + style->border + style->rounding*2);

    /* execute button behavior */
    struct nk_rect bounds = {
        .x = r.x - style->touch_padding.x,
        .y = r.y - style->touch_padding.y,
        .w = r.w + 2 * style->touch_padding.x,
        .h = r.h + 2 * style->touch_padding.y
    };
    return nk_button_behavior(state, bounds, in, behavior);
}

NK_LIB void
nk_draw_button_text(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *content, nk_flags state,
    const struct nk_style_button *style, const char *txt, int len,
    nk_flags text_alignment, const struct nk_user_font *font)
{
    const struct nk_style_item *background = nk_draw_button(out, bounds, state, style);

    struct nk_text text;
    text.padding = nk_vec2(0, 0);

    /* select correct colors/images */
    if (background->type == NK_STYLE_ITEM_COLOR) text.background = background->data.color;
    else                                         text.background = style->text_background;

    if (state & NK_WIDGET_STATE_ACTIVED)    text.text = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.text = style->text_hover;
    else                                    text.text = style->text_normal;

    nk_widget_text(out, *content, txt, len, &text, text_alignment, font);
}
NK_LIB nk_bool
nk_do_button_text(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    const char *string, int len, nk_flags align, enum nk_button_behavior behavior,
    const struct nk_style_button *style, const struct nk_input *in, const struct nk_user_font *font)
{
    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(string);
    NK_ASSERT(font);
    if (!out || !style || !font || !string)
        return nk_false;

    struct nk_rect content;
    nk_bool clicked = nk_do_button(state, out, bounds, style, in, behavior, &content);

    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_text(out, &bounds, &content, *state, style, string, len, align, font);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return clicked;
}

NK_LIB void
nk_draw_button_symbol(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *content,
    nk_flags state, const struct nk_style_button *style,
    enum nk_symbol_type type, const struct nk_user_font *font)
{
    /* select correct colors/images */
    const struct nk_style_item* background = nk_draw_button(out, bounds, state, style);

    struct nk_color bg;
    if (background->type == NK_STYLE_ITEM_COLOR) bg = background->data.color;
    else                                         bg = style->text_background;

    struct nk_color fg;
    if (state & NK_WIDGET_STATE_ACTIVED)    fg = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) fg = style->text_hover;
    else                                    fg = style->text_normal;

    nk_draw_symbol(out, type, *content, bg, fg, 1, font);
}

NK_LIB nk_bool
nk_do_button_symbol(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    enum nk_symbol_type symbol, enum nk_button_behavior behavior,
    const struct nk_style_button *style, const struct nk_input *in, const struct nk_user_font *font)
{
    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(font);
    NK_ASSERT(out);
    if (!out || !style || !font || !state) return nk_false;

    struct nk_rect content;
    nk_bool clicked = nk_do_button(state, out, bounds, style, in, behavior, &content);

    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_symbol(out, &bounds, &content, *state, style, symbol, font);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return clicked;
}

NK_LIB void
nk_draw_button_image(struct nk_command_buffer *out,
    const struct nk_rect *bounds, const struct nk_rect *content,
    nk_flags state, const struct nk_style_button *style, const struct nk_image *img)
{
    nk_draw_button(out, bounds, state, style);
    nk_draw_image(out, *content, img, nk_white);
}

NK_LIB nk_bool
nk_do_button_image(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    struct nk_image img, enum nk_button_behavior b, const struct nk_style_button *style, const struct nk_input *in)
{
    NK_ASSERT(state);
    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style || !state) return nk_false;

    struct nk_rect content;
    nk_bool clicked = nk_do_button(state, out, bounds, style, in, b, &content);
    content.x += style->image_padding.x;
    content.y += style->image_padding.y;
    content.w -= 2 * style->image_padding.x;
    content.h -= 2 * style->image_padding.y;

    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_image(out, &bounds, &content, *state, style, &img);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return clicked;
}

NK_LIB void
nk_draw_button_text_symbol(struct nk_command_buffer *out, const struct nk_rect *bounds, const struct nk_rect *label,
    const struct nk_rect *symbol, nk_flags state, const struct nk_style_button *style,
    const char *str, int len, enum nk_symbol_type type, const struct nk_user_font *font)
{

    /* select correct background colors/images */
    const struct nk_style_item* background = nk_draw_button(out, bounds, state, style);

    struct nk_text text;
    text.padding = nk_vec2(0, 0);

    if (background->type == NK_STYLE_ITEM_COLOR) text.background = background->data.color;
    else                                         text.background = style->text_background;

    /* select correct text colors */
    if (state & NK_WIDGET_STATE_ACTIVED)    text.text = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.text = style->text_hover;
    else                                    text.text = style->text_normal;

    nk_draw_symbol(out, type, *symbol, style->text_background, text.text, 0, font);
    nk_widget_text(out, *label, str, len, &text, NK_TEXT_CENTERED, font);
}

NK_LIB nk_bool
nk_do_button_text_symbol(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    enum nk_symbol_type symbol, const char *str, int len, nk_flags align, enum nk_button_behavior behavior,
    const struct nk_style_button *style, const struct nk_user_font *font, const struct nk_input *in)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    NK_ASSERT(font);
    if (!out || !style || !font) return nk_false;

    struct nk_rect content;
    nk_bool clicked = nk_do_button(state, out, bounds, style, in, behavior, &content);

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

    /* draw button */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_text_symbol(out, &bounds, &content, &sym, *state, style, str, len, symbol, font);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return clicked;
}

NK_LIB void
nk_draw_button_text_image(struct nk_command_buffer *out, const struct nk_rect *bounds, const struct nk_rect *label,
    const struct nk_rect *image, nk_flags state, const struct nk_style_button *style,
    const char *str, int len, const struct nk_user_font *font, const struct nk_image *img)
{
    const struct nk_style_item* background = nk_draw_button(out, bounds, state, style);

    struct nk_text text;
    text.padding = nk_vec2(0, 0);

    /* select correct colors */
    if (background->type == NK_STYLE_ITEM_COLOR) text.background = background->data.color;
    else                                         text.background = style->text_background;

    if (state & NK_WIDGET_STATE_ACTIVED)    text.text = style->text_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.text = style->text_hover;
    else                                    text.text = style->text_normal;

    nk_widget_text(out, *label, str, len, &text, NK_TEXT_CENTERED, font);
    nk_draw_image(out, *image, img, nk_white);
}

NK_LIB nk_bool
nk_do_button_text_image(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    struct nk_image img, const char* str, int len, nk_flags align, enum nk_button_behavior behavior,
    const struct nk_style_button *style, const struct nk_user_font *font, const struct nk_input *in)
{
    NK_ASSERT(style);
    NK_ASSERT(state);
    NK_ASSERT(font);
    NK_ASSERT(out);
    if (!out || !font || !style || !str)
        return nk_false;

    struct nk_rect content;
    nk_bool clicked = nk_do_button(state, out, bounds, style, in, behavior, &content);

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

    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_button_text_image(out, &bounds, &content, &icon, *state, style, str, len, font, &img);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return clicked;
}

NK_API nk_bool
nk_button_push_behavior(struct nk_context *ctx, enum nk_button_behavior behavior)
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
nk_button_pop_behavior(struct nk_context *ctx)
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

NK_API nk_bool nk_button_text(struct nk_context* ctx, const char* title, int len)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return nk_false;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    nk_flags state = 0;
    return nk_do_button_text(&state, &win->buffer, bounds, title, len, style->text_alignment, ctx->button_behavior, style, in, ctx->style.font);
}

NK_API nk_bool nk_button_label(struct nk_context *ctx, const char *title)
{
    return nk_button_text(ctx, title, nk_strlen(title));
}

NK_API nk_bool nk_button_symbol(struct nk_context* ctx, enum nk_symbol_type symbol)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return nk_false;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    nk_flags state = 0;
    return nk_do_button_symbol(&state, &win->buffer, bounds, symbol, ctx->button_behavior, style, in, ctx->style.font);
}

NK_API nk_bool nk_button_image(struct nk_context *ctx, struct nk_image img)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return nk_false;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    nk_flags state = 0;
    return nk_do_button_image(&state, &win->buffer, bounds, img, ctx->button_behavior, style, in);
}

NK_API nk_bool nk_button_symbol_text(struct nk_context *ctx, enum nk_symbol_type symbol, const char *text, int len, nk_flags align)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return nk_false;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    nk_flags state = 0;
    return nk_do_button_text_symbol(&state, &win->buffer, bounds, symbol, text, len, align, ctx->button_behavior, style, ctx->style.font, in);
}

NK_API nk_bool nk_button_symbol_label(struct nk_context *ctx, enum nk_symbol_type symbol, const char *label, nk_flags align)
{
    return nk_button_symbol_text(ctx, symbol, label, nk_strlen(label), align);
}

NK_API nk_bool nk_button_image_text(struct nk_context *ctx, struct nk_image img, const char *text, int len, nk_flags align)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_false;

    struct nk_window* win = ctx->current;
    const struct nk_style_button* style = &ctx->style.button;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return nk_false;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? 0 : &ctx->input;

    nk_flags state = 0;
    return nk_do_button_text_image(&state, &win->buffer, bounds, img, text, len, align, ctx->button_behavior, style, ctx->style.font, in);
}

NK_API nk_bool nk_button_image_label(struct nk_context *ctx, struct nk_image img, const char *label, nk_flags align)
{
    return nk_button_image_text(ctx, img, label, nk_strlen(label), align);
}

