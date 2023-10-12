#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              SLIDER
 *
 * ===============================================================*/
NK_LIB float
nk_slider_behavior(nk_flags *state, struct nk_rect *logical_cursor, struct nk_rect *visual_cursor,
    struct nk_input *in, struct nk_rect bounds, float slider_min, float slider_max, float slider_value,
    float slider_step, float slider_steps)
{
    /* check if visual cursor is being dragged */
    nk_widget_state_reset(state);

    if (nk_input_click_in_rect(in, NK_BUTTON_LEFT, *visual_cursor) && nk_input_mouse_down(in, NK_BUTTON_LEFT))
    {
        const float d = in->mouse_pos.x - (visual_cursor->x + visual_cursor->w * 0.5f);
        const float pxstep = bounds.w / slider_steps;

        /* only update value if the next slider step is reached */
        *state = NK_WIDGET_STATE_ACTIVE;
        if (NK_ABS(d) >= pxstep)
        {
            const float steps = (float)((int)(d / pxstep));
            slider_value += (slider_step * steps);
            slider_value = NK_CLAMP(slider_min, slider_value, slider_max);
            float ratio = (slider_value - slider_min)/slider_step;
            logical_cursor->x = bounds.x + (logical_cursor->w * ratio);
            in->clicked_pos[NK_BUTTON_LEFT].x = logical_cursor->x;
        }
    }

    /* slider widget state */
    if (nk_input_mouse_hover(in, bounds))
        *state = NK_WIDGET_STATE_HOVERED;
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_LEFT;

    return slider_value;
}

NK_LIB void
nk_draw_slider(struct nk_command_buffer *out, nk_flags state,
    const struct nk_style_slider *style, const struct nk_rect *bounds,
    const struct nk_rect *visual_cursor, float min, float value, float max)
{
    struct nk_rect fill;
    struct nk_rect bar;
    const struct nk_style_item *background;

    /* select correct slider images/colors */
    struct nk_color bar_color;
    const struct nk_style_item *cursor;

    NK_UNUSED(min);
    NK_UNUSED(max);
    NK_UNUSED(value);

    if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->active;
        bar_color = style->bar_active;
        cursor = &style->cursor_active;
    } else if (state & NK_WIDGET_STATE_HOVER) {
        background = &style->hover;
        bar_color = style->bar_hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        bar_color = style->bar_normal;
        cursor = &style->cursor_normal;
    }
    /* calculate slider background bar */
    bar.x = bounds->x;
    bar.y = (visual_cursor->y + visual_cursor->h/2) - bounds->h/12;
    bar.w = bounds->w;
    bar.h = bounds->h/6;

    /* filled background bar style */
    fill.w = (visual_cursor->x + (visual_cursor->w/2.0f)) - bar.x;
    fill.x = bar.x;
    fill.y = bar.y;
    fill.h = bar.h;

    /* draw background */
    switch(background->type)
    {
    case NK_STYLE_ITEM_IMAGE:
        nk_draw_image(out, *bounds, &background->data.image, nk_white);
        break;
    case NK_STYLE_ITEM_COLOR:
        nk_fill_rect(out, *bounds, style->rounding, background->data.color);
        nk_stroke_rect(out, *bounds, style->rounding, style->border, style->border_color);
        break;
    }

    /* draw slider bar */
    nk_fill_rect(out, bar, style->rounding, bar_color);
    nk_fill_rect(out, fill, style->rounding, style->bar_filled);

    /* draw cursor */
    if (cursor->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(out, *visual_cursor, &cursor->data.image, nk_white);
    else
        nk_fill_circle(out, *visual_cursor, cursor->data.color);
}

NK_LIB float
nk_do_slider(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    float slider_min, float slider_value, float slider_max, float step,
    const struct nk_style_slider *style, struct nk_input *in,
    const struct nk_user_font *font)
{
    struct nk_rect visual_cursor;
    struct nk_rect logical_cursor;

    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style)
        return 0;

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
        if (nk_do_button_symbol(&ws, out, button, style->dec_symbol, NK_BUTTON_DEFAULT, &style->dec_button, in, font))
            slider_value -= step;

        /* increment button */
        button.x = (bounds.x + bounds.w) - button.w;
        if (nk_do_button_symbol(&ws, out, button, style->inc_symbol, NK_BUTTON_DEFAULT, &style->inc_button, in, font))
            slider_value += step;

        bounds.x += button.w + style->spacing.x;
        bounds.w -= 2 * (button.w + style->spacing.x);
    }

    /* remove one cursor size to support visual cursor */
    bounds.x += style->cursor_size.x*0.5f;
    bounds.w -= style->cursor_size.x;

    /* make sure the provided values are correct */
    slider_value = NK_CLAMP(slider_min, slider_value, slider_max);
    float step_count = (slider_max - slider_min) / step;
    float cursor_offset = (slider_value - slider_min) / step;

    /* calculate cursor
    Basically you have two cursors. One for visual representation and interaction
    and one for updating the actual cursor value. */
    logical_cursor.h = bounds.h;
    logical_cursor.w = bounds.w / step_count;
    logical_cursor.x = bounds.x + (logical_cursor.w * cursor_offset);
    logical_cursor.y = bounds.y;

    visual_cursor.h = style->cursor_size.y;
    visual_cursor.w = style->cursor_size.x;
    visual_cursor.y = (bounds.y + bounds.h*0.5f) - visual_cursor.h*0.5f;
    visual_cursor.x = logical_cursor.x - visual_cursor.w*0.5f;

    slider_value = nk_slider_behavior(state, &logical_cursor, &visual_cursor, in, bounds, slider_min, slider_max, slider_value, step, step_count);
    visual_cursor.x = logical_cursor.x - visual_cursor.w*0.5f;

    /* draw slider */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_slider(out, *state, style, &bounds, &visual_cursor, slider_min, slider_value, slider_max);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return slider_value;
}

NK_API float nk_slider(struct nk_context *ctx, float min, float value, float max, float step)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0.0f;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return 0.0f;
    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? NULL : &ctx->input;

    nk_flags state = 0;
    return nk_do_slider(&state, &win->buffer, bounds, min, value, max, step, &style->slider, in, style->font);
}

NK_API int nk_slider_int(struct nk_context *ctx, int min, int val, int max, int step)
{
    return (int)nk_slider(ctx, (float)min, (float)val, (float)max, (float)step);
}

