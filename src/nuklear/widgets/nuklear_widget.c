#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              WIDGET
 *
 * ===============================================================*/
NK_API struct nk_rect nk_widget_bounds(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return nk_rect(0,0,0,0);

    struct nk_rect bounds;
    nk_layout_peek(&bounds, ctx);
    return bounds;
}

NK_API float nk_widget_width(struct nk_context *ctx)  { return nk_widget_bounds(ctx).w; }
NK_API float nk_widget_height(struct nk_context *ctx) { return nk_widget_bounds(ctx).h; }

NK_API nk_widget_layout_state nk_widget(struct nk_rect *bounds, const struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return NK_WIDGET_INVALID;

    /* allocate space and check if the widget needs to be updated and drawn */
    nk_panel_alloc_space(bounds, ctx);

    /*  if one of these triggers you forgot to add an `if` condition around either
        a window, group, popup, combobox or contextual menu `begin` and `end` block.
        Example:
            if (nk_begin(...) {...} nk_end(...); or
            if (nk_group_begin(...) { nk_group_end(...);} */
    struct nk_panel* layout = ctx->current->layout;
    NK_ASSERT(!(layout->flags & NK_WINDOW_MINIMIZED));
    NK_ASSERT(!(layout->flags & NK_WINDOW_HIDDEN));
    NK_ASSERT(!(layout->flags & NK_WINDOW_CLOSED));

    /* need to convert to int here to remove floating point errors */
    bounds->x = (float)((int)bounds->x);
    bounds->y = (float)((int)bounds->y);
    bounds->w = (float)((int)bounds->w);
    bounds->h = (float)((int)bounds->h);

    struct nk_rect c = {
        .x = (float)((int)layout->clip.x),
        .y = (float)((int)layout->clip.y),
        .w = (float)((int)layout->clip.w),
        .h = (float)((int)layout->clip.h)
    };

    struct nk_rect v;
    nk_unify(&v, &c, bounds->x, bounds->y, bounds->x + bounds->w, bounds->y + bounds->h);

    if (!NK_INTERSECT(c.x, c.y, c.w, c.h, bounds->x, bounds->y, bounds->w, bounds->h))
        return NK_WIDGET_INVALID;

    struct nk_vec2 pos = ctx->input.mouse_pos;
    if (!NK_INBOX(pos.x, pos.y, v.x, v.y, v.w, v.h))
        return NK_WIDGET_ROM;

    return NK_WIDGET_VALID;
}

NK_API struct nk_input* 
nk_widget_input(struct nk_rect *bounds, nk_widget_layout_state *state, struct nk_context *ctx)
{
    *state = nk_widget(bounds, ctx);
    if (ctx->current->layout->flags & NK_WINDOW_ROM) return NULL;
    if (*state == NK_WIDGET_INVALID || *state == NK_WIDGET_ROM) return NULL;

    return &ctx->input;
}

NK_API void nk_spacing(struct nk_context *ctx, int cols)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return;

    /* spacing over row boundaries */
    struct nk_window* win = ctx->current;
    struct nk_row_layout* row = &win->layout->row;
    int index = (row->index + cols) % row->columns;
    int rows = (row->index + cols) / row->columns;

    if (rows)
    {
        for (int i = 0; i < rows; ++i)
            nk_panel_alloc_row(ctx, win);
        cols = index;
    }

    /* non table layout need to allocate space */
    struct nk_rect none;
    if (row->type != NK_LAYOUT_DYNAMIC_FIXED && row->type != NK_LAYOUT_STATIC_FIXED)
    {
        for (int i = 0; i < cols; ++i)
            nk_panel_alloc_space(&none, ctx);
    }
    row->index = index;
}

NK_API void nk_label_image(struct nk_context* ctx, nk_image img, nk_color col)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    struct nk_rect bounds = { 0 };
    if (!nk_widget(&bounds, ctx)) return;

    nk_draw_image(&ctx->current->buffer, bounds, &img, col);
}

NK_API void nk_label_color(struct nk_context *ctx, nk_color color)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state = nk_widget(&bounds, ctx);

    nk_fill_rect(&ctx->current->buffer, bounds, 0.0f, color);
}