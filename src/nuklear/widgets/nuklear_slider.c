#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              SLIDER
 *
 * ===============================================================*/
NK_LIB float
nk_slider_behavior(nk_flags *state, struct nk_rect cursor,
    struct nk_input *in, struct nk_rect bounds, float min_value, float value, float max_value, float step)
{
    nk_widget_state_reset(state);

    /* check if cursor is being dragged */
    if (nk_input_click_in_rect(in, NK_BUTTON_LEFT, cursor) && nk_input_mouse_down(in, NK_BUTTON_LEFT))
    {
        const float delta = in->mouse_pos.x - (cursor.x + cursor.w * 0.5f);
        const float pxstep = (bounds.w * step) / (max_value - min_value);

        /* only update value if the next slider step is reached */
        if (NK_ABS(delta) >= pxstep)
        {
            value += (step * (float)((int)(delta / pxstep)));
            value = NK_CLAMP(min_value, value, max_value);
            float ratio = (value - min_value) / step;

            /*
            float ratio = NK_MAX(0, (in->mouse_pos.x - cursor.x)) / cursor.w;
            value = (max_value - min_value) * ratio;
            value = NK_CLAMP(min_value, value, max_value);
            */

            in->clicked_pos[NK_BUTTON_LEFT].x = cursor.x + cursor.w / 2.0f;
        }
        *state = NK_WIDGET_STATE_ACTIVE;
    }

    /* slider widget state */
    if (nk_input_mouse_hover(in, bounds))
        *state |= NK_WIDGET_STATE_HOVER;
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_LEFT;

    return value;
}

NK_LIB void
nk_draw_slider(struct nk_command_buffer *out, nk_flags state,
    const struct nk_style_slider *style, const struct nk_rect *bounds,
    const struct nk_rect *cursor)
{
    /* select correct slider images/colors */
    nk_color bar_color;
    const nk_style_item *cursor_style;
    const nk_style_item *background;
    if (state & NK_WIDGET_STATE_ACTIVE)
    {
        background = &style->active;
        bar_color = style->bar_active;
        cursor_style = &style->cursor_active;
    }
    else if (state & NK_WIDGET_STATE_HOVER)
    {
        background = &style->hover;
        bar_color = style->bar_hover;
        cursor_style = &style->cursor_hover;
    }
    else
    {
        background = &style->normal;
        bar_color = style->bar_normal;
        cursor_style = &style->cursor_normal;
    }

    /* calculate slider background bar */
    struct nk_rect bar = {
        .x = bounds->x,
        .y = (bounds->y + bounds->h/2) - bounds->h/12,
        .w = bounds->w,
        .h = bounds->h/6,
    };

    /* filled background bar style */
    struct nk_rect fill = {
        .x = bar.x,
        .y = bar.y,
        .w = (cursor->x + (cursor->w/2.0f)) - bar.x,
        .h = bar.h
    };

    /* draw background */
    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *bounds, &background->image, nk_white);
    else
        nk_fill_rect_border(out, *bounds, style->rounding, background->color, style->border, style->border_color);

    /* draw slider bar */
    nk_fill_rect(out, bar, style->rounding, bar_color);
    nk_fill_rect(out, fill, style->rounding, style->bar_filled);

    //return;

    /* draw cursor */
    if (cursor_style->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *cursor, &cursor_style->image, nk_white);
    else
        nk_fill_circle(out, *cursor, cursor_style->color);
}

NK_LIB float
nk_do_slider(nk_flags *state, struct nk_command_buffer *out, struct nk_rect bounds,
    float min_value, float value, float max_value, float step, const struct nk_style_slider *style, 
    struct nk_input *in, const struct nk_font *font)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style) return 0;

    /* remove padding from slider bounds */
    bounds.x = bounds.x + style->padding.x;
    bounds.y = bounds.y + style->padding.y;
    bounds.h = NK_MAX(bounds.h, 2*style->padding.y);
    bounds.w = NK_MAX(bounds.w, 2*style->padding.x + style->cursor_size.x);
    bounds.w -= 2 * style->padding.x;
    bounds.h -= 2 * style->padding.y;

    /* optional buttons */
    if (style->show_buttons)
    {
        struct nk_rect button = bounds;

        /* decrement button */
        nk_flags ws;
        if (nk_do_button_symbol(&ws, out, button, style->dec_symbol, NK_BUTTON_DEFAULT, &style->dec_button, in))
            value -= step;

        /* increment button */
        button.x = (bounds.x + bounds.w) - button.w;
        if (nk_do_button_symbol(&ws, out, button, style->inc_symbol, NK_BUTTON_DEFAULT, &style->inc_button, in))
            value += step;

        bounds.x += button.w + style->spacing.x;
        bounds.w -= 2 * (button.w + style->spacing.x);
    }

    /* remove one cursor size to support visual cursor */
    bounds.x += style->cursor_size.x*0.5f;
    bounds.w -= style->cursor_size.x;

    /* make sure the provided values are correct */
    value = NK_CLAMP(min_value, value, max_value);
    /* calculate cursor */
    float cursor_offset = bounds.w * ((value - min_value) / (max_value - min_value));
    struct nk_rect cursor = {
        .x = bounds.x + cursor_offset - style->cursor_size.x * 0.5f,
        .y = bounds.y + bounds.h * 0.5f - style->cursor_size.y * 0.5f,
        .w = style->cursor_size.x,
        .h = style->cursor_size.y
    };


    value = nk_slider_behavior(state, cursor, in, bounds, min_value, value, max_value, step);
    //value = nk_bar_slider_behavior(state, in, bounds, bounds, min_value, value, max_value);

    /* draw slider */
    nk_draw_slider(out, *state, style, &bounds, &cursor);

    return value;
}

NK_API float nk_slider(struct nk_context *ctx, float min_value, float value, float max_value, float step)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return value;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return value;

    nk_flags state = 0;
    return nk_do_slider(&state, &win->buffer, bounds, min_value, value, max_value, step, &style->slider, in, style->font);
}

NK_API int nk_slider_int(struct nk_context *ctx, int min, int val, int max, int step)
{
    return (int)nk_slider(ctx, (float)min, (float)val, (float)max, (float)step);
}

/* ===============================================================
 *
 *                          BAR SLIDER
 *
 * ===============================================================*/
NK_LIB float
nk_bar_slider_behavior(nk_flags* state, struct nk_input* in, struct nk_rect bounds, struct nk_rect cursor, float min_value, float value, float max_value)
{
    nk_widget_state_reset(state);

    if (nk_input_click_in_rect(in, NK_BUTTON_LEFT, bounds) && nk_input_mouse_down(in, NK_BUTTON_LEFT))
    {
        float ratio = NK_MAX(0, (in->mouse_pos.x - cursor.x)) / cursor.w;
        value = (max_value - min_value) * ratio + min_value;
        value = NK_CLAMP(min_value, value, max_value);
        in->clicked_pos[NK_BUTTON_LEFT].x = bounds.x + bounds.w / 2.0f;

        *state |= NK_WIDGET_STATE_ACTIVE;
    }

    /* set progressbar widget state */
    if (nk_input_mouse_hover(in, bounds))
        *state |= NK_WIDGET_STATE_HOVER;

    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_LEFT;

    return value;
}

NK_LIB void
nk_draw_bar_slider(struct nk_command_buffer* out, nk_flags state,
    const struct nk_style_progress* style, const struct nk_rect* bounds, const struct nk_rect* cursor)
{
    const nk_style_item* background;
    const nk_style_item* cursor_style;

    /* select correct colors/images to draw */
    if (state & NK_WIDGET_STATE_ACTIVE)
    {
        background = &style->active;
        cursor_style = &style->cursor_active;
    }
    else if (state & NK_WIDGET_STATE_HOVER)
    {
        background = &style->hover;
        cursor_style = &style->cursor_hover;
    }
    else
    {
        background = &style->normal;
        cursor_style = &style->cursor_normal;
    }

    /* draw background */
    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *bounds, &background->image, nk_white);
    else
        nk_fill_rect_border(out, *bounds, style->rounding, background->color, style->border, style->border_color);

    /* draw cursor */
    if (cursor_style->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *cursor, &cursor_style->image, nk_white);
    else
        nk_fill_rect_border(out, *cursor, style->rounding, cursor_style->color, style->border, style->border_color);
}

NK_LIB nk_size
nk_do_bar_slider(nk_flags* state, struct nk_command_buffer* out, struct nk_rect bounds,
    nk_size value, nk_size max_value, const struct nk_style_progress* style, struct nk_input* in)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style) return value;

    /* calculate bar */
    struct nk_rect bar = {
        .x = bounds.x + style->padding.x + style->border,
        .y = bounds.y + style->padding.y + style->border,
        .w = NK_MAX(bounds.w, 2 * (style->padding.x + style->border)),
        .h = NK_MAX(bounds.h, 2 * (style->padding.y + style->border))
    };
    bar.w -= 2 * (style->padding.x + style->border);
    bar.h -= 2 * (style->padding.y + style->border);

    nk_size min_value = 50;

    /* update slider */
    value = NK_CLAMP(0, value, max_value);
    if (in)
        value = (nk_size)nk_bar_slider_behavior(state, in, bounds, bar, (float)min_value, (float)value, (float)max_value);

    bar.w *= (float)(value - min_value) / (float)(max_value - min_value);

    /* draw bar slider */
    nk_draw_bar_slider(out, *state, style, &bounds, &bar);

    return value;
}

NK_API nk_size nk_bar_slider(struct nk_context* ctx, nk_size value, nk_size max)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return value;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return value;

    nk_flags state = 0;
    return nk_do_bar_slider(&state, &win->buffer, bounds, value, max, &style->progress, in);
}

NK_API nk_size nk_progress(struct nk_context *ctx, nk_size value, nk_size max)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return value;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return value;

    nk_flags state = 0;
    return nk_do_bar_slider(&state, &win->buffer, bounds, value, max, &style->progress, NULL);
}