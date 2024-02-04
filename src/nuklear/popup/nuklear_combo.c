#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ==============================================================
 *
 *                          COMBO
 *
 * ===============================================================*/
NK_INTERN nk_bool nk_combo_begin(struct nk_context *ctx, struct nk_window *win, struct nk_vec2 size, nk_bool is_clicked, struct nk_rect header)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout)
        return 0;

    struct nk_rect body = {
        .x = header.x,
        .y = header.y + header.h - ctx->style.window.combo_border,
        .w = size.x,
        .h = size.y
    };

    struct nk_window* popup = win->popup.win;
    nk_hash hash = win->popup.combo_count++;

    if (popup && (win->popup.name != hash || win->popup.type != NK_PANEL_COMBO)) return nk_false;
    if (!popup && !is_clicked) return nk_false;

    if (popup && is_clicked) header = (struct nk_rect){0.0f};

    if (!nk_nonblock_begin(ctx, 0, body, header, NK_PANEL_COMBO))
        return 0;

    win->popup.type = NK_PANEL_COMBO;
    win->popup.name = hash;
    return 1;
}

static void nk_combo_draw_background(struct nk_command_buffer* out, struct nk_rect bounds, nk_flags state, struct nk_style_combo* style)
{
    const nk_style_item* background;
    if (state & NK_WIDGET_STATE_ACTIVE)     background = &style->active;
    else if (state & NK_WIDGET_STATE_HOVER) background = &style->hover;
    else                                    background = &style->normal;

    switch (background->type)
    {
    case NK_STYLE_ITEM_IMAGE:
        nk_draw_image(out, bounds, &background->image, nk_white);
        break;
    case NK_STYLE_ITEM_COLOR:
        nk_fill_rect_border(out, bounds, style->rounding, background->color, style->border, style->border_color);
        break;
    }
}

static nk_bool nk_combo_draw_button(struct nk_command_buffer* out, struct nk_rect bounds, nk_flags state, nk_bool clicked, struct nk_style_combo* style)
{
    nk_symbol sym = NK_SYMBOL_NONE;
    if (state & NK_WIDGET_STATE_ACTIVE)     sym = style->sym_active;
    else if (state & NK_WIDGET_STATE_HOVER) sym = style->sym_hover;
    else                                    sym = style->sym_normal;

    /* represents whether or not the combo's button symbol should be drawn */
    if (sym == NK_SYMBOL_NONE) return nk_false;

    /* calculate button */
    struct nk_rect button = {
        .x = (bounds.x + bounds.w - bounds.h) - style->button_padding.x,
        .y = bounds.y + style->button_padding.y,
        .w = bounds.h - 2 * style->button_padding.y,
        .h = button.w
    };

    /* draw open/close button */
    nk_draw_button_symbol(out, button, state, &style->button, sym);
    return nk_true;
}

NK_API nk_bool nk_combo_begin_text(struct nk_context *ctx, const char *selected, int len, struct nk_vec2 size)
{
    NK_ASSERT(ctx);
    NK_ASSERT(selected);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout || !selected) return 0;

    struct nk_window* win = ctx->current;
    struct nk_style_combo* style = &ctx->style.combo;

    struct nk_rect header;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&header, &layout_state, ctx);
    if (!layout_state) return nk_false;

    nk_flags state = 0;
    nk_bool clicked = nk_button_behavior(&state, header, in, NK_BUTTON_DEFAULT);

    /* draw combo box header background and border */
    nk_combo_draw_background(&win->buffer, header, state, style);

    /* print currently selected text item */
    nk_style_text text = { 0 };
    text.alignment = NK_TEXT_LEFT;
    if (state & NK_WIDGET_STATE_ACTIVE)     text.color = style->label_active;
    else if (state & NK_WIDGET_STATE_HOVER) text.color = style->label_hover;
    else                                    text.color = style->label_normal;

    /* draw selected label */
    struct nk_rect label = {
        .x = header.x + style->content_padding.x,
        .y = header.y + style->content_padding.y,
        .w = header.w - 2 * style->content_padding.x,
        .h = header.h - 2 * style->content_padding.y,
    };

    if (nk_combo_draw_button(&win->buffer, header, state, clicked, style))
    {
        float x = (header.x + header.w - header.h) - style->button_padding.x;
        label.w = x - (style->content_padding.x + style->spacing.x) - label.x;
    }

    nk_widget_text(&win->buffer, label, selected, len, &text, ctx->style.font);

    return nk_combo_begin(ctx, win, size, clicked, header);
}

NK_API nk_bool nk_combo_begin_label(struct nk_context *ctx, const char *selected, struct nk_vec2 size)
{
    return nk_combo_begin_text(ctx, selected, nk_strlen(selected), size);
}

NK_API nk_bool nk_combo_begin_color(struct nk_context *ctx, nk_color color, struct nk_vec2 size)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return 0;

    struct nk_window* win = ctx->current;
    struct nk_style_combo* style = &ctx->style.combo;

    struct nk_rect header;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&header, &layout_state, ctx);
    if (!layout_state) return nk_false;

    nk_flags state = 0;
    nk_bool clicked = nk_button_behavior(&state, header, in, NK_BUTTON_DEFAULT);

    /* draw combo box header background and border */
    nk_combo_draw_background(&win->buffer, header, state, style);

    struct nk_rect bounds = {
        .x = header.x + 2 * style->content_padding.x,
        .y = header.y + 2 * style->content_padding.y,
        .w = header.w - 4 * style->content_padding.x,
        .h = header.h - 4 * style->content_padding.y,
    };

    if (nk_combo_draw_button(&win->buffer, header, state, clicked, style))
    {
        float x = (header.x + header.w - header.h) - style->button_padding.x;
        bounds.w = x - (style->content_padding.x + style->spacing.x) - bounds.x;
    }

    nk_fill_rect(&win->buffer, bounds, 0, color);
    return nk_combo_begin(ctx, win, size, clicked, header);
}

NK_API void nk_combo_end(struct nk_context *ctx)   { nk_contextual_end(ctx); }
NK_API void nk_combo_close(struct nk_context *ctx) { nk_contextual_close(ctx); }

static float nk_combo_max_panel_height(const struct nk_style* style, float height, int count, enum nk_panel_type type)
{
    struct nk_vec2 spacing = style->window.spacing;
    struct nk_vec2 padding = nk_panel_get_padding(style, type);

    return count * (height + spacing.y) + spacing.y * 2 + padding.y * 2;
}

NK_API int nk_combo(struct nk_context *ctx, const char **items, int count, int selected, int item_height, struct nk_vec2 size)
{
    NK_ASSERT(ctx);
    NK_ASSERT(items);
    NK_ASSERT(ctx->current);
    if (!ctx || !items ||!count)
        return selected;

    float max_height = nk_combo_max_panel_height(&ctx->style, (float)item_height, count, ctx->current->layout->type);
    size.y = NK_MIN(size.y, max_height);

    if (nk_combo_begin_label(ctx, items[selected], size))
    {
        nk_layout_row_dynamic(ctx, (float)item_height, 1);
        for (int i = 0; i < count; ++i)
        {
            if (nk_contextual_item_text(ctx, items[i], nk_strlen(items[i]), NK_TEXT_LEFT))
                selected = i;
        }
        nk_contextual_end(ctx);
    }
    return selected;
}

NK_API int nk_combo_separator(struct nk_context *ctx, const char *items, int separator, int selected, int count, int item_height, struct nk_vec2 size)
{
    NK_ASSERT(ctx);
    NK_ASSERT(items);
    if (!ctx || !items)
        return selected;

    float max_height = nk_combo_max_panel_height(&ctx->style, (float)item_height, count, ctx->current->layout->type);
    size.y = NK_MIN(size.y, max_height);

    /* find selected item */
    int length = 0;
    const char* current_item = items;
    for (int i = 0; i < count; ++i)
    {
        const char* iter = current_item;
        while (*iter && *iter != separator) iter++;
        length = (int)(iter - current_item);
        if (i == selected) break;
        current_item = iter + 1;
    }

    if (nk_combo_begin_text(ctx, current_item, length, size))
    {
        current_item = items;
        nk_layout_row_dynamic(ctx, (float)item_height, 1);
        for (int i = 0; i < count; ++i)
        {
            const char* iter = current_item;
            while (*iter && *iter != separator) iter++;
            length = (int)(iter - current_item);
            if (nk_contextual_item_text(ctx, current_item, length, NK_TEXT_LEFT))
                selected = i;
            current_item = current_item + length + 1;
        }
        nk_combo_end(ctx);
    }
    return selected;
}

