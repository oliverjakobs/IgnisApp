#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              SCROLLBAR
 *
 * ===============================================================*/
NK_LIB float nk_scrollbar_behavior(nk_flags *state, struct nk_input *in,
    int has_scrolling, const struct nk_rect *scroll,
    const struct nk_rect *cursor, const struct nk_rect *empty0,
    const struct nk_rect *empty1, float scroll_offset,
    float target, float scroll_step, enum nk_orientation o)
{
    nk_flags ws = 0;

    nk_widget_state_reset(state);
    if (!in) return scroll_offset;

    if (nk_input_mouse_hover(in, *scroll))
        *state = NK_WIDGET_STATE_HOVER;


    /* update cursor by mouse dragging */
    if (nk_input_mouse_down(in, NK_BUTTON_LEFT) && nk_input_click_in_rect(in, NK_BUTTON_LEFT, *cursor))
    {
        struct nk_vec2 delta = in->mouse_delta;

        *state = NK_WIDGET_STATE_ACTIVE;
        if (o == NK_VERTICAL)
        {
            float d = (delta.y / scroll->h) * target;
            scroll_offset = NK_CLAMP(0, scroll_offset + d, target - scroll->h);
            float cursor_y = scroll->y + ((scroll_offset/target) * scroll->h);
            in->clicked_pos[NK_BUTTON_LEFT].y = cursor_y + cursor->h/2.0f;
        }
        else
        {
            float d = (delta.x / scroll->w) * target;
            scroll_offset = NK_CLAMP(0, scroll_offset + d, target - scroll->w);
            float cursor_x = scroll->x + ((scroll_offset/target) * scroll->w);
            in->clicked_pos[NK_BUTTON_LEFT].x = cursor_x + cursor->w/2.0f;
        }
    }
    /* scroll page up by click on empty space or shortcut */
    else if ((nk_input_key_pressed(in, NK_KEY_SCROLL_UP) && o == NK_VERTICAL && has_scrolling)||
            nk_button_behavior(&ws, *empty0, in, NK_BUTTON_DEFAULT))
    {
        if (o == NK_VERTICAL) scroll_offset = scroll_offset - scroll->h;
        else                  scroll_offset = scroll_offset - scroll->w;
    }
    else if ((nk_input_key_pressed(in, NK_KEY_SCROLL_DOWN) && o == NK_VERTICAL && has_scrolling) ||
        nk_button_behavior(&ws, *empty1, in, NK_BUTTON_DEFAULT))
    {
        /* scroll page down by click on empty space or shortcut */
        if (o == NK_VERTICAL) scroll_offset = scroll_offset + scroll->h;
        else                  scroll_offset = scroll_offset + scroll->w;
    }
    else if (has_scrolling)
    {
        /* update cursor by mouse scrolling */
        float scroll_delta = (o == NK_VERTICAL) ? in->scroll_delta.y : in->scroll_delta.x;
        if (scroll_delta != 0.0f)
        {
            scroll_offset += scroll_step * (-scroll_delta);
        }
        /* update cursor to the beginning  */
        else if (nk_input_key_pressed(in, NK_KEY_SCROLL_START))
        {
            if (o == NK_VERTICAL) scroll_offset = 0;
        }
        /* update cursor to the end */
        else if (nk_input_key_pressed(in, NK_KEY_SCROLL_END))
        {
            if (o == NK_VERTICAL) scroll_offset = target - scroll->h;
        }
    }

    if (o == NK_VERTICAL) scroll_offset = NK_CLAMP(0, scroll_offset, target - scroll->h);
    else                  scroll_offset = NK_CLAMP(0, scroll_offset, target - scroll->w);

    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, *scroll))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, *scroll))
        *state |= NK_WIDGET_STATE_LEFT;

    return scroll_offset;
}

NK_LIB void
nk_draw_scrollbar(struct nk_command_buffer *out, nk_flags state,
    const struct nk_style_scrollbar *style, const struct nk_rect *bounds,
    const struct nk_rect *scroll)
{
    const nk_style_item *background;
    const nk_style_item *cursor;

    /* select correct colors/images to draw */
    if (state & NK_WIDGET_STATE_ACTIVE) {
        background = &style->active;
        cursor = &style->cursor_active;
    } else if (state & NK_WIDGET_STATE_HOVER) {
        background = &style->hover;
        cursor = &style->cursor_hover;
    } else {
        background = &style->normal;
        cursor = &style->cursor_normal;
    }

    /* draw background */
    switch (background->type) {
        case NK_STYLE_ITEM_IMAGE:
            nk_draw_image(out, *bounds, &background->image, nk_white);
            break;
        case NK_STYLE_ITEM_COLOR:
            nk_fill_rect(out, *bounds, style->rounding, background->color);
            nk_stroke_rect(out, *bounds, style->rounding, style->border, style->border_color);
            break;
    }

    /* draw cursor */
    switch (cursor->type) {
        case NK_STYLE_ITEM_IMAGE:
            nk_draw_image(out, *scroll, &cursor->image, nk_white);
            break;
        case NK_STYLE_ITEM_COLOR:
            nk_fill_rect(out, *scroll, style->rounding_cursor, cursor->color);
            nk_stroke_rect(out, *scroll, style->rounding_cursor, style->border_cursor, style->cursor_border_color);
            break;
    }
}
NK_LIB float
nk_do_scrollbarv(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect scroll, int has_scrolling,
    float offset, float target, float step, float button_pixel_inc,
    const struct nk_style_scrollbar *style, struct nk_input *in,
    const struct nk_font *font)
{
    struct nk_rect empty_north;
    struct nk_rect empty_south;
    struct nk_rect cursor;

    float scroll_step;
    float scroll_offset;
    float scroll_off;
    float scroll_ratio;

    NK_ASSERT(out);
    NK_ASSERT(style);
    NK_ASSERT(state);
    if (!out || !style) return 0;

    scroll.w = NK_MAX(scroll.w, 1);
    scroll.h = NK_MAX(scroll.h, 0);
    if (target <= scroll.h) return 0;

    /* optional scrollbar buttons */
    if (style->show_buttons) {
        nk_flags ws;
        float scroll_h;
        struct nk_rect button;

        button.x = scroll.x;
        button.w = scroll.w;
        button.h = scroll.w;

        scroll_h = NK_MAX(scroll.h - 2 * button.h,0);
        scroll_step = NK_MIN(step, button_pixel_inc);

        /* decrement button */
        button.y = scroll.y;
        if (nk_do_button_symbol(&ws, out, button, style->dec_symbol,
            NK_BUTTON_REPEATER, &style->dec_button, in))
            offset = offset - scroll_step;

        /* increment button */
        button.y = scroll.y + scroll.h - button.h;
        if (nk_do_button_symbol(&ws, out, button, style->inc_symbol,
            NK_BUTTON_REPEATER, &style->inc_button, in))
            offset = offset + scroll_step;

        scroll.y = scroll.y + button.h;
        scroll.h = scroll_h;
    }

    /* calculate scrollbar constants */
    scroll_step = NK_MIN(step, scroll.h);
    scroll_offset = NK_CLAMP(0, offset, target - scroll.h);
    scroll_ratio = scroll.h / target;
    scroll_off = scroll_offset / target;

    /* calculate scrollbar cursor bounds */
    cursor.h = NK_MAX((scroll_ratio * scroll.h) - (2*style->border + 2*style->padding.y), 0);
    cursor.y = scroll.y + (scroll_off * scroll.h) + style->border + style->padding.y;
    cursor.w = scroll.w - (2 * style->border + 2 * style->padding.x);
    cursor.x = scroll.x + style->border + style->padding.x;

    /* calculate empty space around cursor */
    empty_north.x = scroll.x;
    empty_north.y = scroll.y;
    empty_north.w = scroll.w;
    empty_north.h = NK_MAX(cursor.y - scroll.y, 0);

    empty_south.x = scroll.x;
    empty_south.y = cursor.y + cursor.h;
    empty_south.w = scroll.w;
    empty_south.h = NK_MAX((scroll.y + scroll.h) - (cursor.y + cursor.h), 0);

    /* update scrollbar */
    scroll_offset = nk_scrollbar_behavior(state, in, has_scrolling, &scroll, &cursor,
        &empty_north, &empty_south, scroll_offset, target, scroll_step, NK_VERTICAL);
    scroll_off = scroll_offset / target;
    cursor.y = scroll.y + (scroll_off * scroll.h) + style->border_cursor + style->padding.y;

    /* draw scrollbar */
    nk_draw_scrollbar(out, *state, style, &scroll, &cursor);

    return scroll_offset;
}
NK_LIB float
nk_do_scrollbarh(nk_flags *state,
    struct nk_command_buffer *out, struct nk_rect scroll, int has_scrolling,
    float offset, float target, float step, float button_pixel_inc,
    const struct nk_style_scrollbar *style, struct nk_input *in,
    const struct nk_font *font)
{
    struct nk_rect cursor;
    struct nk_rect empty_west;
    struct nk_rect empty_east;

    float scroll_step;
    float scroll_offset;
    float scroll_off;
    float scroll_ratio;

    NK_ASSERT(out);
    NK_ASSERT(style);
    if (!out || !style) return 0;

    /* scrollbar background */
    scroll.h = NK_MAX(scroll.h, 1);
    scroll.w = NK_MAX(scroll.w, 2 * scroll.h);
    if (target <= scroll.w) return 0;

    /* optional scrollbar buttons */
    if (style->show_buttons) {
        nk_flags ws;
        float scroll_w;
        struct nk_rect button;
        button.y = scroll.y;
        button.w = scroll.h;
        button.h = scroll.h;

        scroll_w = scroll.w - 2 * button.w;
        scroll_step = NK_MIN(step, button_pixel_inc);

        /* decrement button */
        button.x = scroll.x;
        if (nk_do_button_symbol(&ws, out, button, style->dec_symbol,
            NK_BUTTON_REPEATER, &style->dec_button, in))
            offset = offset - scroll_step;

        /* increment button */
        button.x = scroll.x + scroll.w - button.w;
        if (nk_do_button_symbol(&ws, out, button, style->inc_symbol,
            NK_BUTTON_REPEATER, &style->inc_button, in))
            offset = offset + scroll_step;

        scroll.x = scroll.x + button.w;
        scroll.w = scroll_w;
    }

    /* calculate scrollbar constants */
    scroll_step = NK_MIN(step, scroll.w);
    scroll_offset = NK_CLAMP(0, offset, target - scroll.w);
    scroll_ratio = scroll.w / target;
    scroll_off = scroll_offset / target;

    /* calculate cursor bounds */
    cursor.w = (scroll_ratio * scroll.w) - (2*style->border + 2*style->padding.x);
    cursor.x = scroll.x + (scroll_off * scroll.w) + style->border + style->padding.x;
    cursor.h = scroll.h - (2 * style->border + 2 * style->padding.y);
    cursor.y = scroll.y + style->border + style->padding.y;

    /* calculate empty space around cursor */
    empty_west.x = scroll.x;
    empty_west.y = scroll.y;
    empty_west.w = cursor.x - scroll.x;
    empty_west.h = scroll.h;

    empty_east.x = cursor.x + cursor.w;
    empty_east.y = scroll.y;
    empty_east.w = (scroll.x + scroll.w) - (cursor.x + cursor.w);
    empty_east.h = scroll.h;

    /* update scrollbar */
    scroll_offset = nk_scrollbar_behavior(state, in, has_scrolling, &scroll, &cursor,
        &empty_west, &empty_east, scroll_offset, target, scroll_step, NK_HORIZONTAL);
    scroll_off = scroll_offset / target;
    cursor.x = scroll.x + (scroll_off * scroll.w);

    /* draw scrollbar */
    nk_draw_scrollbar(out, *state, style, &scroll, &cursor);
    return scroll_offset;
}

