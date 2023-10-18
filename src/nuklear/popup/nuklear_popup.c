#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              POPUP
 *
 * ===============================================================*/
NK_API nk_bool
nk_popup_begin(struct nk_context *ctx, enum nk_popup_type type, const char *title, nk_flags flags, struct nk_rect bounds)
{
    NK_ASSERT(ctx);
    NK_ASSERT(title);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return nk_false;

    struct nk_window* win = ctx->current;
    NK_ASSERT(!(win->layout->type & NK_PANEL_SET_POPUP) && "popups are not allowed to have popups");

    struct nk_window* popup = win->popup.win;
    if (!popup)
    {
        popup = (struct nk_window*)nk_create_window(ctx);
        popup->parent = win;
        win->popup.win = popup;
        win->popup.active = 0;
        win->popup.type = NK_PANEL_POPUP;
    }

    /* make sure we have correct popup */
    int title_len = nk_strlen(title);
    nk_hash title_hash = nk_murmur_hash(title, title_len, NK_PANEL_POPUP);
    if (win->popup.name != title_hash)
    {
        if (win->popup.active) return 0;

        nk_zero(popup, sizeof(*popup));
        win->popup.name = title_hash;
        win->popup.active = 1;
        win->popup.type = NK_PANEL_POPUP;
    }

    /* popup position is local to window */
    ctx->current = popup;
    bounds.x += win->layout->clip.x;
    bounds.y += win->layout->clip.y;

    /* setup popup data */
    popup->parent = win;
    popup->bounds = bounds;
    popup->seq = ctx->seq;
    popup->layout = nk_create_panel(ctx);
    popup->flags = flags | NK_WINDOW_BORDER;

    if (type == NK_POPUP_DYNAMIC)
        popup->flags |= NK_WINDOW_DYNAMIC;

    popup->buffer = win->buffer;
    nk_start_popup(ctx, win);
    nk_size allocated = ctx->memory.allocated;
    nk_push_scissor(&popup->buffer, nk_null_rect);

    if (nk_panel_begin(ctx, title, NK_PANEL_POPUP))
    {
        /* popup is running therefore invalidate parent panels */
        struct nk_panel *root = win->layout;
        while (root)
        {
            root->flags |= NK_WINDOW_ROM;
            root->flags &= ~(nk_flags)NK_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        win->popup.active = nk_true;
        popup->layout->offset_x = &popup->scrollbar.x;
        popup->layout->offset_y = &popup->scrollbar.y;
        popup->layout->parent = win->layout;
        return nk_true;
    }

    /* popup was closed/is invalid so cleanup */
    struct nk_panel* root = win->layout;
    while (root)
    {
        root->flags |= NK_WINDOW_REMOVE_ROM;
        root = root->parent;
    }
    win->popup.buf.active = nk_false;
    win->popup.active = nk_false;
    ctx->memory.allocated = allocated;
    ctx->current = win;
    nk_free_panel(ctx, popup->layout);
    popup->layout = NULL;

    return nk_false;
}

NK_LIB nk_bool
nk_nonblock_begin(struct nk_context *ctx, nk_flags flags, struct nk_rect body, struct nk_rect header, enum nk_panel_type panel_type)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return nk_false;

    /* popups cannot have popups */
    struct nk_window* win = ctx->current;
    NK_ASSERT(!(win->layout->type & NK_PANEL_SET_POPUP));

    struct nk_window* popup = win->popup.win;
    if (!popup)
    {
        /* create window for nonblocking popup */
        popup = nk_create_window(ctx);
        popup->parent = win;
        win->popup.win = popup;
        win->popup.type = panel_type;
        nk_command_buffer_init(&popup->buffer, &ctx->memory, NK_CLIPPING_ON);
    }
    else
    {
        /* close the popup if user pressed outside or in the header */
        nk_bool pressed = nk_input_mouse_released(&ctx->input, NK_BUTTON_LEFT);
        nk_bool in_body = nk_input_mouse_hover(&ctx->input, body);
        nk_bool in_header = nk_input_mouse_hover(&ctx->input, header);
        if (pressed && (!in_body || in_header))
        {
            /* remove read only mode from all parent panels */
            struct nk_panel* root = win->layout;
            while (root)
            {
                root->flags |= NK_WINDOW_REMOVE_ROM;
                root = root->parent;
            }
            return nk_false;
        }
    }
    win->popup.header = header;

    popup->bounds = body;
    popup->parent = win;
    popup->layout = nk_create_panel(ctx);
    popup->flags = flags;
    popup->flags |= NK_WINDOW_BORDER;
    popup->flags |= NK_WINDOW_DYNAMIC;
    popup->seq = ctx->seq;
    win->popup.active = 1;
    NK_ASSERT(popup->layout);

    nk_start_popup(ctx, win);
    popup->buffer = win->buffer;
    nk_push_scissor(&popup->buffer, nk_null_rect);
    ctx->current = popup;

    nk_panel_begin(ctx, 0, panel_type);
    win->buffer = popup->buffer;
    popup->layout->parent = win->layout;
    popup->layout->offset_x = &popup->scrollbar.x;
    popup->layout->offset_y = &popup->scrollbar.y;

    /* set read only mode to all parent panels */
    struct nk_panel *root = win->layout;
    while (root)
    {
        root->flags |= NK_WINDOW_ROM;
        root = root->parent;
    }
    return nk_true;
}

NK_API void nk_popup_close(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    if (!ctx || !ctx->current) return;

    struct nk_window* popup = ctx->current;
    NK_ASSERT(popup->parent);
    NK_ASSERT(popup->layout->type & NK_PANEL_SET_POPUP);
    popup->flags |= NK_WINDOW_HIDDEN;
}

NK_API void nk_popup_end(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    struct nk_window* popup = ctx->current;
    struct nk_window* parent = popup->parent;

    if (!parent) return;
    if (popup->flags & NK_WINDOW_HIDDEN)
    {
        struct nk_panel* root = parent->layout;
        while (root)
        {
            root->flags |= NK_WINDOW_REMOVE_ROM;
            root = root->parent;
        }
        parent->popup.active = 0;
    }
    nk_push_scissor(&popup->buffer, nk_null_rect);
    nk_end(ctx);

    parent->buffer = popup->buffer;
    nk_finish_popup(ctx, parent);
    ctx->current = parent;
    nk_push_scissor(&ctx->current->buffer, ctx->current->layout->clip);
}

NK_API void nk_popup_get_scroll(struct nk_context *ctx, nk_uint *offset_x, nk_uint *offset_y)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    struct nk_window* popup = ctx->current;
    if (offset_x) *offset_x = popup->scrollbar.x;
    if (offset_y) *offset_y = popup->scrollbar.y;
}

NK_API void nk_popup_set_scroll(struct nk_context *ctx, nk_uint offset_x, nk_uint offset_y)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    struct nk_window* popup = ctx->current;
    popup->scrollbar.x = offset_x;
    popup->scrollbar.y = offset_y;
}
