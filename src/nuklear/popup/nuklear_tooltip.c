#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              TOOLTIP
 *
 * ===============================================================*/
NK_API nk_bool nk_tooltip_begin(struct nk_context *ctx, float width)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return nk_false;

    /* make sure that no nonblocking popup is currently active */
    struct nk_window* win = ctx->current;
    if (win->popup.win && (win->popup.type & NK_PANEL_SET_NONBLOCK))
        return nk_false;

    struct nk_vec2 mouse = ctx->input.mouse_pos;

    struct nk_rect bounds = {
        .x = (float)nk_ifloorf(mouse.x + 1) - (int)win->layout->clip.x,
        .y = (float)nk_ifloorf(mouse.y + 1) - (int)win->layout->clip.y,
        .w = (float)nk_iceilf(width),
        .h = (float)nk_iceilf(nk_null_rect.h)
    };

    nk_flags flags = NK_WINDOW_NO_SCROLLBAR | NK_WINDOW_BORDER;
    nk_bool ret = nk_popup_begin(ctx, NK_POPUP_DYNAMIC, "__##Tooltip##__", flags, bounds);
    
    if (ret) win->layout->flags &= ~(nk_flags)NK_WINDOW_ROM;

    win->popup.type = NK_PANEL_TOOLTIP;
    ctx->current->layout->type = NK_PANEL_TOOLTIP;

    return ret;
}

NK_API void nk_tooltip_end(struct nk_context *ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;
    ctx->current->seq--;
    nk_popup_close(ctx);
    nk_popup_end(ctx);
}

NK_API void nk_tooltip(struct nk_context *ctx, const char *text)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    NK_ASSERT(text);
    if (!ctx || !ctx->current || !ctx->current->layout || !text) return;

    /* fetch configuration data */
    struct nk_vec2 padding = ctx->style.window.padding;
    const struct nk_font* font = ctx->style.font;

    /* calculate size of the text and tooltip */
    int text_len = nk_strlen(text);
    float text_width = font->width(font->userdata, font->height, text, text_len) + 4 * padding.x;
    float text_height = font->height + 2 * padding.y;

    /* execute tooltip and fill with text */
    if (nk_tooltip_begin(ctx, text_width))
    {
        nk_layout_row_dynamic(ctx, text_height, 1);
        nk_text(ctx, text, text_len, NK_TEXT_LEFT);
        nk_tooltip_end(ctx);
    }
}

NK_API void nk_tooltipf(struct nk_context *ctx, const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nk_tooltipfv(ctx, fmt, args);
    va_end(args);
}

NK_API void nk_tooltipfv(struct nk_context *ctx, const char *fmt, va_list args)
{
    char buf[256];
    nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_tooltip(ctx, buf);
}


