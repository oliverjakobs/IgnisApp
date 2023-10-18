#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                              PANEL
 *
 * ===============================================================*/
NK_LIB void* nk_create_panel(struct nk_context *ctx)
{
    struct nk_page_element *elem;
    elem = nk_create_page_element(ctx);
    if (!elem) return 0;
    nk_zero_struct(*elem);
    return &elem->data.pan;
}

NK_LIB void nk_free_panel(struct nk_context *ctx, struct nk_panel *pan)
{
    union nk_page_data *pd = NK_CONTAINER_OF(pan, union nk_page_data, pan);
    struct nk_page_element *pe = NK_CONTAINER_OF(pd, struct nk_page_element, data);
    nk_free_page_element(ctx, pe);
}

NK_LIB nk_bool nk_panel_has_header(nk_flags flags, const char *title)
{
    nk_bool active = 0;
    active = (flags & (NK_WINDOW_CLOSABLE|NK_WINDOW_MINIMIZABLE));
    active = active || (flags & NK_WINDOW_TITLE);
    active = active && !(flags & NK_WINDOW_HIDDEN) && title;
    return active;
}

NK_LIB struct nk_vec2 nk_panel_get_padding(const struct nk_style *style, enum nk_panel_type type)
{
    switch (type)
    {
    default:
    case NK_PANEL_WINDOW:       return style->window.padding;
    case NK_PANEL_GROUP:        return style->window.group_padding;
    case NK_PANEL_POPUP:        return style->window.popup_padding;
    case NK_PANEL_CONTEXTUAL:   return style->window.contextual_padding;
    case NK_PANEL_COMBO:        return style->window.combo_padding;
    case NK_PANEL_MENU:         return style->window.menu_padding;
    case NK_PANEL_TOOLTIP:      return style->window.menu_padding;
    }
}

NK_LIB float nk_panel_get_border(const struct nk_style *style, nk_flags flags, enum nk_panel_type type)
{
    if (!(flags & NK_WINDOW_BORDER)) return 0.0f;

    switch (type)
    {
    default:
    case NK_PANEL_WINDOW:       return style->window.border;
    case NK_PANEL_GROUP:        return style->window.group_border;
    case NK_PANEL_POPUP:        return style->window.popup_border;
    case NK_PANEL_CONTEXTUAL:   return style->window.contextual_border;
    case NK_PANEL_COMBO:        return style->window.combo_border;
    case NK_PANEL_MENU:         return style->window.menu_border;
    case NK_PANEL_TOOLTIP:      return style->window.menu_border;
    }
}

NK_LIB nk_color nk_panel_get_border_color(const struct nk_style *style, enum nk_panel_type type)
{
    switch (type)
    {
    default:
    case NK_PANEL_WINDOW:       return style->window.border_color;
    case NK_PANEL_GROUP:        return style->window.group_border_color;
    case NK_PANEL_POPUP:        return style->window.popup_border_color;
    case NK_PANEL_CONTEXTUAL:   return style->window.contextual_border_color;
    case NK_PANEL_COMBO:        return style->window.combo_border_color;
    case NK_PANEL_MENU:         return style->window.menu_border_color;
    case NK_PANEL_TOOLTIP:      return style->window.menu_border_color;
    }
}

NK_LIB nk_bool nk_panel_is_sub(enum nk_panel_type type)       { return (type & NK_PANEL_SET_SUB); }
NK_LIB nk_bool nk_panel_is_nonblock(enum nk_panel_type type)  { return (type & NK_PANEL_SET_NONBLOCK); }

static void nk_panel_do_titlebar(nk_flags* state, struct nk_command_buffer* out, struct nk_rect bounds, const char* title, nk_bool active, const struct nk_input* in, const struct nk_style_window_header* style, const struct nk_font* font)
{
    nk_style_text text;
    text.padding = style->label_padding;
    text.alignment = NK_TEXT_LEFT;

    /* select correct header background and text color */
    const nk_style_item* background = NULL;
    if (active)
    {
        background = &style->active;
        text.color = style->label_active;
    }
    else if (nk_input_mouse_hover(in, bounds))
    {
        background = &style->hover;
        text.color = style->label_hover;
    }
    else
    {
        background = &style->normal;
        text.color = style->label_normal;
    }

    /* draw header background */
    bounds.h += 1.0f;

    switch (background->type)
    {
    case NK_STYLE_ITEM_IMAGE:
        nk_draw_image(out, bounds, &background->image, nk_white);
        break;
    case NK_STYLE_ITEM_COLOR:
        nk_fill_rect(out, bounds, 0, background->color);
        break;
    }

    // add padding
    bounds.x += style->padding.x;
    bounds.y += style->padding.y;
    bounds.w -= 2 * style->padding.x;
    bounds.h -= 2 * style->padding.y;

    struct nk_rect button = bounds;
    button.w = button.h;

    /* window close button */
    if (*state & NK_WINDOW_CLOSABLE)
    {
        struct nk_rect button = bounds;
        button.w = button.h;

        if (style->align == NK_HEADER_RIGHT) button.x = bounds.x + (bounds.w - button.w);
        else                                 bounds.x += button.w + style->spacing.x;
        bounds.w -= button.w + style->spacing.x;

        nk_flags ws = 0;
        char sym = 'x';
        if (nk_do_button_text(&ws, out, button, &sym, 1, NK_TEXT_CENTERED, NK_BUTTON_DEFAULT, &style->close_button, in, font)
            && !(*state & NK_WINDOW_ROM))
        {
            *state |= NK_WINDOW_HIDDEN;
            *state &= (nk_flags)~NK_WINDOW_MINIMIZED;
        }
    }

    /* window minimize button */
    if (*state & NK_WINDOW_MINIMIZABLE)
    {
        if (style->align == NK_HEADER_RIGHT) button.x = bounds.x + (bounds.w - button.w);
        else                                 bounds.x += button.w + style->spacing.x;
        bounds.w -= button.w + style->spacing.x;

        nk_flags ws = 0;
        char sym = (*state & NK_WINDOW_MINIMIZED) ? '+' : '-';
        if (nk_do_button_text(&ws, out, button, &sym, 1, NK_TEXT_CENTERED, NK_BUTTON_DEFAULT, &style->minimize_button, in, font)
            && !(*state & NK_WINDOW_ROM))
        {
            *state = (*state & NK_WINDOW_MINIMIZED) ?
            *state & (nk_flags)~NK_WINDOW_MINIMIZED :
            *state | NK_WINDOW_MINIMIZED;
        }
    }

    /* window header title */
    nk_widget_text(out, bounds, title, nk_strlen(title), &text, font);
}

NK_LIB nk_bool nk_panel_begin(struct nk_context *ctx, const char *title, enum nk_panel_type panel_type)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return 0;
    nk_zero(ctx->current->layout, sizeof(*ctx->current->layout));

    if ((ctx->current->flags & NK_WINDOW_HIDDEN) || (ctx->current->flags & NK_WINDOW_CLOSED))
    {
        nk_zero(ctx->current->layout, sizeof(struct nk_panel));
        ctx->current->layout->type = panel_type;
        return 0;
    }

    /* pull state into local stack */
    const struct nk_style* style = &ctx->style;
    const struct nk_font* font = style->font;
    struct nk_window* win = ctx->current;
    struct nk_panel* layout = win->layout;
    struct nk_command_buffer* out = &win->buffer;
    struct nk_input* in = (win->flags & NK_WINDOW_NO_INPUT) ? NULL : &ctx->input;

    /* pull style configuration into local stack */
    struct nk_vec2 scrollbar_size = style->window.scrollbar_size;
    struct nk_vec2 panel_padding = nk_panel_get_padding(style, panel_type);

    /* window movement */
    if ((win->flags & NK_WINDOW_MOVABLE) && !(win->flags & NK_WINDOW_ROM))
    {
        /* calculate draggable window space */
        struct nk_rect header = {
            .x = win->bounds.x,
            .y = win->bounds.y,
            .w = win->bounds.w,
            .h = panel_padding.y,
        };
        if (nk_panel_has_header(win->flags, title))
        {
            header.h = font->height + 2.0f * style->window.header.padding.y;
            header.h += 2.0f * style->window.header.label_padding.y;
        }

        /* window movement by dragging */
        if (nk_input_mouse_down(in, NK_BUTTON_LEFT) && nk_input_click_in_rect(in, NK_BUTTON_LEFT, header))
        {
            struct nk_vec2 delta = in->mouse_delta;

            win->bounds.x = win->bounds.x + delta.x;
            win->bounds.y = win->bounds.y + delta.y;
            in->clicked_pos[NK_BUTTON_LEFT].x += delta.x;
            in->clicked_pos[NK_BUTTON_LEFT].y += delta.y;
            ctx->style.cursor_active = ctx->style.cursors[NK_CURSOR_MOVE];
        }
    }

    /* setup panel */
    layout->type = panel_type;
    layout->flags = win->flags;
    layout->bounds = win->bounds;
    layout->bounds.x += panel_padding.x;
    layout->bounds.w -= 2 * panel_padding.x;
    if (win->flags & NK_WINDOW_BORDER) {
        layout->border = nk_panel_get_border(style, win->flags, panel_type);
        layout->bounds = nk_shrink_rect(layout->bounds, layout->border);
    } else layout->border = 0;
    layout->at_y = layout->bounds.y;
    layout->at_x = layout->bounds.x;
    layout->max_x = 0;
    layout->header_height = 0;
    layout->footer_height = 0;
    nk_layout_reset_min_row_height(ctx);
    layout->row.index = 0;
    layout->row.columns = 0;
    layout->row.ratio = 0;
    layout->row.item_width = 0;
    layout->row.tree_depth = 0;
    layout->row.height = panel_padding.y;
    layout->has_scrolling = nk_true;
    if (!(win->flags & NK_WINDOW_NO_SCROLLBAR))
        layout->bounds.w -= scrollbar_size.x;
    if (!nk_panel_is_nonblock(panel_type)) {
        layout->footer_height = 0;
        if (!(win->flags & NK_WINDOW_NO_SCROLLBAR) || win->flags & NK_WINDOW_SCALABLE)
            layout->footer_height = scrollbar_size.y;
        layout->bounds.h -= layout->footer_height;
    }

    /* panel header */
    if (nk_panel_has_header(win->flags, title))
    {
        /* calculate header bounds */
        struct nk_rect header;
        header.x = win->bounds.x;
        header.y = win->bounds.y;
        header.w = win->bounds.w;
        header.h = font->height + 2.0f * style->window.header.padding.y;
        header.h += (2.0f * style->window.header.label_padding.y);

        /* shrink panel by header */
        layout->header_height = header.h;
        layout->bounds.y += header.h;
        layout->bounds.h -= header.h;
        layout->at_y += header.h;
        nk_panel_do_titlebar(&layout->flags, &win->buffer, header, title, ctx->active == win, in, &style->window.header, style->font);
    }

    /* draw window background */
    if (!(layout->flags & NK_WINDOW_MINIMIZED) && !(layout->flags & NK_WINDOW_DYNAMIC))
    {
        struct nk_rect body = {
            .x = win->bounds.x,
            .y = (win->bounds.y + layout->header_height),
            .w = win->bounds.w,
            .h = (win->bounds.h - layout->header_height)
        };

        switch(style->window.fixed_background.type)
        {
            case NK_STYLE_ITEM_IMAGE:
                nk_draw_image(out, body, &style->window.fixed_background.image, nk_white);
                break;
            case NK_STYLE_ITEM_COLOR:
                nk_fill_rect(out, body, 0, style->window.fixed_background.color);
                break;
        }
    }

    /* set clipping rectangle */
    nk_unify(&layout->clip, &win->buffer.clip, layout->bounds.x, layout->bounds.y,
        layout->bounds.x + layout->bounds.w, layout->bounds.y + layout->bounds.h);
    nk_push_scissor(out, layout->clip);
    return !(layout->flags & NK_WINDOW_HIDDEN) && !(layout->flags & NK_WINDOW_MINIMIZED);
}


NK_LIB void
nk_panel_end(struct nk_context *ctx)
{
    struct nk_input *in;
    struct nk_window *window;
    struct nk_panel *layout;
    const struct nk_style *style;
    struct nk_command_buffer *out;

    struct nk_vec2 scrollbar_size;
    struct nk_vec2 panel_padding;

    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    window = ctx->current;
    layout = window->layout;
    style = &ctx->style;
    out = &window->buffer;
    in = (layout->flags & NK_WINDOW_ROM || layout->flags & NK_WINDOW_NO_INPUT) ? 0 :&ctx->input;
    if (!nk_panel_is_sub(layout->type))
        nk_push_scissor(out, nk_null_rect);

    /* cache configuration data */
    scrollbar_size = style->window.scrollbar_size;
    panel_padding = nk_panel_get_padding(style, layout->type);

    /* update the current cursor Y-position to point over the last added widget */
    layout->at_y += layout->row.height;

    /* dynamic panels */
    if (layout->flags & NK_WINDOW_DYNAMIC && !(layout->flags & NK_WINDOW_MINIMIZED))
    {
        /* update panel height to fit dynamic growth */
        struct nk_rect empty_space;
        if (layout->at_y < (layout->bounds.y + layout->bounds.h))
            layout->bounds.h = layout->at_y - layout->bounds.y;

        /* fill top empty space */
        empty_space.x = window->bounds.x;
        empty_space.y = layout->bounds.y;
        empty_space.h = panel_padding.y;
        empty_space.w = window->bounds.w;
        nk_fill_rect(out, empty_space, 0, style->window.background);

        /* fill left empty space */
        empty_space.x = window->bounds.x;
        empty_space.y = layout->bounds.y;
        empty_space.w = panel_padding.x + layout->border;
        empty_space.h = layout->bounds.h;
        nk_fill_rect(out, empty_space, 0, style->window.background);

        /* fill right empty space */
        empty_space.x = layout->bounds.x + layout->bounds.w;
        empty_space.y = layout->bounds.y;
        empty_space.w = panel_padding.x + layout->border;
        empty_space.h = layout->bounds.h;
        if (*layout->offset_y == 0 && !(layout->flags & NK_WINDOW_NO_SCROLLBAR))
            empty_space.w += scrollbar_size.x;
        nk_fill_rect(out, empty_space, 0, style->window.background);

        /* fill bottom empty space */
        if (layout->footer_height > 0) {
            empty_space.x = window->bounds.x;
            empty_space.y = layout->bounds.y + layout->bounds.h;
            empty_space.w = window->bounds.w;
            empty_space.h = layout->footer_height;
            nk_fill_rect(out, empty_space, 0, style->window.background);
        }
    }

    /* scrollbars */
    if (!(layout->flags & NK_WINDOW_NO_SCROLLBAR) &&
        !(layout->flags & NK_WINDOW_MINIMIZED) &&
        window->scrollbar_hiding_timer < NK_SCROLLBAR_HIDING_TIMEOUT)
    {
        struct nk_rect scroll;
        int scroll_has_scrolling;
        float scroll_target;
        float scroll_offset;
        float scroll_step;
        float scroll_inc;

        /* mouse wheel scrolling */
        if (nk_panel_is_sub(layout->type))
        {
            /* sub-window mouse wheel scrolling */
            struct nk_window *root_window = window;
            struct nk_panel *root_panel = window->layout;
            while (root_panel->parent)
                root_panel = root_panel->parent;
            while (root_window->parent)
                root_window = root_window->parent;

            /* only allow scrolling if parent window is active */
            scroll_has_scrolling = 0;
            if ((root_window == ctx->active) && layout->has_scrolling)
            {
                /* and panel is being hovered and inside clip rect*/
                if (nk_input_mouse_hover(in, layout->bounds) &&
                    NK_INTERSECT(layout->bounds.x, layout->bounds.y, layout->bounds.w, layout->bounds.h,
                        root_panel->clip.x, root_panel->clip.y, root_panel->clip.w, root_panel->clip.h))
                {
                    /* deactivate all parent scrolling */
                    root_panel = window->layout;
                    while (root_panel->parent) {
                        root_panel->has_scrolling = nk_false;
                        root_panel = root_panel->parent;
                    }
                    root_panel->has_scrolling = nk_false;
                    scroll_has_scrolling = nk_true;
                }
            }
        }
        else if (!nk_panel_is_sub(layout->type))
        {
            /* window mouse wheel scrolling */
            scroll_has_scrolling = (window == ctx->active) && layout->has_scrolling;
            window->scrolled = (in && (in->scroll_delta.y > 0 || in->scroll_delta.x > 0) && scroll_has_scrolling);

        }
        else
        {
            scroll_has_scrolling = nk_false;
        }

        {
            /* vertical scrollbar */
            nk_flags state = 0;
            scroll.x = layout->bounds.x + layout->bounds.w + panel_padding.x;
            scroll.y = layout->bounds.y;
            scroll.w = scrollbar_size.x;
            scroll.h = layout->bounds.h;

            scroll_offset = (float)*layout->offset_y;
            scroll_step = scroll.h * 0.10f;
            scroll_inc = scroll.h * 0.01f;
            scroll_target = (float)(int)(layout->at_y - scroll.y);
            scroll_offset = nk_do_scrollbarv(&state, out, scroll, scroll_has_scrolling,
                scroll_offset, scroll_target, scroll_step, scroll_inc,
                &ctx->style.scrollv, in, style->font);
            *layout->offset_y = (nk_uint)scroll_offset;
            if (in && scroll_has_scrolling)
                in->scroll_delta.y = 0;
        }
        {
            /* horizontal scrollbar */
            nk_flags state = 0;
            scroll.x = layout->bounds.x;
            scroll.y = layout->bounds.y + layout->bounds.h;
            scroll.w = layout->bounds.w;
            scroll.h = scrollbar_size.y;

            scroll_offset = (float)*layout->offset_x;
            scroll_target = (float)(int)(layout->max_x - scroll.x);
            scroll_step = layout->max_x * 0.05f;
            scroll_inc = layout->max_x * 0.005f;
            scroll_offset = nk_do_scrollbarh(&state, out, scroll, scroll_has_scrolling,
                scroll_offset, scroll_target, scroll_step, scroll_inc,
                &ctx->style.scrollh, in, style->font);
            *layout->offset_x = (nk_uint)scroll_offset;
        }
    }

    /* hide scroll if no user input */
    if (window->flags & NK_WINDOW_SCROLL_AUTO_HIDE)
    {
        nk_bool has_input = nk_input_mouse_moved(&ctx->input) || ctx->input.scroll_delta.y != 0;
        nk_bool hovered = nk_window_is_hovered(ctx);

        if ((!has_input && hovered) || !hovered)
            window->scrollbar_hiding_timer += ctx->delta_time_seconds;
        else
            window->scrollbar_hiding_timer = 0;
    } 
    else window->scrollbar_hiding_timer = 0;

    /* window border */
    if (layout->flags & NK_WINDOW_BORDER)
    {
        nk_color border_color = nk_panel_get_border_color(style, layout->type);
        const float padding_y = (layout->flags & NK_WINDOW_MINIMIZED)
            ? (style->window.border + window->bounds.y + layout->header_height)
            : ((layout->flags & NK_WINDOW_DYNAMIC)
                ? (layout->bounds.y + layout->bounds.h + layout->footer_height)
                : (window->bounds.y + window->bounds.h));
        struct nk_rect b = window->bounds;
        b.h = padding_y - window->bounds.y;
        nk_stroke_rect(out, b, 0, layout->border, border_color);
    }

    /* scaler */
    if ((layout->flags & NK_WINDOW_SCALABLE) && in && !(layout->flags & NK_WINDOW_MINIMIZED))
    {
        /* calculate scaler bounds */
        struct nk_rect scaler;
        scaler.w = scrollbar_size.x;
        scaler.h = scrollbar_size.y;
        scaler.y = layout->bounds.y + layout->bounds.h;

        if (layout->flags & NK_WINDOW_SCALE_LEFT)
            scaler.x = layout->bounds.x - panel_padding.x * 0.5f;
        else
            scaler.x = layout->bounds.x + layout->bounds.w + panel_padding.x;

        if (layout->flags & NK_WINDOW_NO_SCROLLBAR)
            scaler.x -= scaler.w;

        /* draw scaler */
        const nk_style_item *item = &style->window.scaler;
        if (item->type == NK_STYLE_ITEM_IMAGE)
        {
            nk_draw_image(out, scaler, &item->image, nk_white);
        }
        else
        {
            struct nk_vec2 a = nk_vec2(scaler.x + scaler.w, scaler.y);
            struct nk_vec2 b = nk_vec2(scaler.x, scaler.y + scaler.h);
            struct nk_vec2 c = nk_vec2(scaler.x + scaler.w, scaler.y + scaler.h);
            if (layout->flags & NK_WINDOW_SCALE_LEFT)
            {
                a.x -= scaler.w;
            }
            nk_fill_triangle(out, a, b, c, item->color);
        }

        /* do window scaling */
        if (!(window->flags & NK_WINDOW_ROM))
        {
            struct nk_vec2 window_size = style->window.min_size;

            if (nk_input_mouse_down(in, NK_BUTTON_LEFT) && nk_input_click_in_rect(in, NK_BUTTON_LEFT, scaler))
            {
                struct nk_vec2 mouse = in->mouse_pos;
                struct nk_vec2 delta = in->mouse_delta;

                if (layout->flags & NK_WINDOW_SCALE_LEFT)
                {
                    delta.x = -delta.x;
                    window->bounds.x += delta.x;
                }
                /* dragging in x-direction  */
                if (window->bounds.w + delta.x >= window_size.x)
                {
                    if ((delta.x < 0) || (delta.x > 0 && mouse.x >= scaler.x))
                    {
                        window->bounds.w = window->bounds.w + delta.x;
                        scaler.x += delta.x;
                    }
                }
                /* dragging in y-direction (only possible if static window) */
                if (!(layout->flags & NK_WINDOW_DYNAMIC))
                {
                    if (window_size.y < window->bounds.h + delta.y)
                    {
                        if ((delta.y < 0) || (delta.y > 0 && mouse.y >= scaler.y))
                        {
                            window->bounds.h = window->bounds.h + delta.y;
                            scaler.y += delta.y;
                        }
                    }
                }
                ctx->style.cursor_active = ctx->style.cursors[NK_CURSOR_RESIZE_TOP_RIGHT_DOWN_LEFT];
                in->clicked_pos[NK_BUTTON_LEFT].x = scaler.x + scaler.w/2.0f;
                in->clicked_pos[NK_BUTTON_LEFT].y = scaler.y + scaler.h/2.0f;
            }
        }
    }

    if (!nk_panel_is_sub(layout->type))
    {
        if (layout->flags & NK_WINDOW_HIDDEN) /* window is hidden so clear command buffer  */
            nk_command_buffer_reset(&window->buffer);
        else /* window is visible and not tab */
            nk_finish(ctx, window);
    }

    /* NK_WINDOW_REMOVE_ROM flag was set so remove NK_WINDOW_ROM */
    if (layout->flags & NK_WINDOW_REMOVE_ROM)
    {
        layout->flags &= ~(nk_flags)NK_WINDOW_ROM;
        layout->flags &= ~(nk_flags)NK_WINDOW_REMOVE_ROM;
    }
    window->flags = layout->flags;

    /* property garbage collector */
    if (window->property.active && window->property.old != window->property.seq &&
        window->property.active == window->property.prev) {
        nk_zero(&window->property, sizeof(window->property));
    } else {
        window->property.old = window->property.seq;
        window->property.prev = window->property.active;
        window->property.seq = 0;
    }
    /* edit garbage collector */
    if (window->edit.active && window->edit.old != window->edit.seq &&
       window->edit.active == window->edit.prev) {
        nk_zero(&window->edit, sizeof(window->edit));
    } else {
        window->edit.old = window->edit.seq;
        window->edit.prev = window->edit.active;
        window->edit.seq = 0;
    }
    /* contextual garbage collector */
    if (window->popup.active_con && window->popup.con_old != window->popup.con_count) {
        window->popup.con_count = 0;
        window->popup.con_old = 0;
        window->popup.active_con = 0;
    } else {
        window->popup.con_old = window->popup.con_count;
        window->popup.con_count = 0;
    }
    window->popup.combo_count = 0;
    /* helper to make sure you have a 'nk_tree_push' for every 'nk_tree_pop' */
    NK_ASSERT(!layout->row.tree_depth);
}

