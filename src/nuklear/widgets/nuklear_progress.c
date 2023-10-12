#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                          PROGRESS
 *
 * ===============================================================*/
NK_LIB nk_size
nk_progress_behavior(nk_flags *state, struct nk_input *in,
    struct nk_rect r, struct nk_rect cursor, nk_size max, nk_size value, nk_bool modifiable)
{
    nk_widget_state_reset(state);
    if (!in || !modifiable) return value;

    if (nk_input_mouse_hover(in, r))
        *state = NK_WIDGET_STATE_HOVERED;

    if (nk_input_click_in_rect(in, NK_BUTTON_LEFT, cursor) && nk_input_mouse_down(in, NK_BUTTON_LEFT))
    {
        struct nk_vec2 pos = in->mouse_pos;

        float ratio = NK_MAX(0, (float)(pos.x - cursor.x)) / (float)cursor.w;
        value = (nk_size)NK_CLAMP(0, (float)max * ratio, (float)max);
        in->clicked_pos[NK_BUTTON_LEFT].x = cursor.x + cursor.w / 2.0f;
        *state |= NK_WIDGET_STATE_ACTIVE;
    }

    /* set progressbar widget state */
    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, r))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, r))
        *state |= NK_WIDGET_STATE_LEFT;

    return value;
}

NK_LIB void
nk_draw_progress(struct nk_command_buffer *out, nk_flags state,
    const struct nk_style_progress *style, const struct nk_rect *bounds,
    const struct nk_rect *scursor, nk_size value, nk_size max)
{
    const struct nk_style_item *background;
    const struct nk_style_item *cursor;

    NK_UNUSED(max);
    NK_UNUSED(value);

    /* select correct colors/images to draw */
    if (state & NK_WIDGET_STATE_ACTIVED) {
        background = &style->active;
        cursor = &style->cursor_active;
    } else if (state & NK_WIDGET_STATE_HOVER){
        background = &style->hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
    }

    /* draw background */
    switch(background->type) {
        case NK_STYLE_ITEM_IMAGE:
            nk_draw_image(out, *bounds, &background->data.image, nk_white);
            break;
        case NK_STYLE_ITEM_COLOR:
            nk_fill_rect(out, *bounds, style->rounding, background->data.color);
            nk_stroke_rect(out, *bounds, style->rounding, style->border, style->border_color);
            break;
    }

    /* draw cursor */
    switch(cursor->type) {
        case NK_STYLE_ITEM_IMAGE:
            nk_draw_image(out, *scursor, &cursor->data.image, nk_white);
            break;
        case NK_STYLE_ITEM_COLOR:
            nk_fill_rect(out, *scursor, style->rounding, cursor->data.color);
            nk_stroke_rect(out, *scursor, style->rounding, style->border, style->border_color);
            break;
    }
}
NK_LIB nk_size
nk_do_progress(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect bounds,
    nk_size value, nk_size max_value, nk_bool modifiable,
    const struct nk_style_progress *style, struct nk_input *in)
{
    NK_ASSERT(style);
    NK_ASSERT(out);
    if (!out || !style) return 0;

    /* calculate progressbar cursor */
    struct nk_rect cursor = {
        .x = bounds.x + style->padding.x + style->border,
        .y = bounds.y + style->padding.y + style->border,
        .w = NK_MAX(bounds.w, 2 * (style->padding.x + style->border)),
        .h = NK_MAX(bounds.h, 2 * (style->padding.y + style->border))
    };
    cursor.w -= 2 * (style->padding.x + style->border);
    cursor.h -= 2 * (style->padding.y + style->border);

    /* update progressbar */
    value = NK_CLAMP(0, value, max_value);
    value = nk_progress_behavior(state, in, bounds, cursor, max_value, value, modifiable);

    cursor.w *= (float)value / (float)max_value;

    /* draw progressbar */
    if (style->draw_begin) style->draw_begin(out, style->userdata);
    nk_draw_progress(out, *state, style, &bounds, &cursor, value, max_value);
    if (style->draw_end) style->draw_end(out, style->userdata);

    return value;
}
NK_API nk_size
nk_progress(struct nk_context *ctx, nk_size cur, nk_size max, nk_bool modifyable)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    enum nk_widget_layout_states layout_state = nk_widget(&bounds, ctx);
    if (!layout_state) return 0;

    const struct nk_input* in = (layout_state == NK_WIDGET_ROM || win->layout->flags & NK_WINDOW_ROM) ? NULL : &ctx->input;

    nk_flags state = 0;
    return nk_do_progress(&state, &win->buffer, bounds, cur, max, modifyable, &style->progress, in);
}

