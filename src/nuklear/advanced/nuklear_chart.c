#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ==============================================================
 *
 *                          CHART
 *
 * ===============================================================*/
NK_API nk_bool
nk_chart_begin_colored(struct nk_context *ctx, enum nk_chart_type type,
    nk_color color, nk_color highlight, int count, float min_value, float max_value)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return 0;

    struct nk_rect bounds = { 0 };
    if (!nk_widget(&bounds, ctx))
    {
        struct nk_chart* chart = &ctx->current->layout->chart;
        nk_zero(chart, sizeof(*chart));
        return nk_false;
    }

    struct nk_window* win = ctx->current;
    const struct nk_style_chart* style = &ctx->style.chart;

    /* setup basic generic chart  */
    struct nk_chart* chart = &win->layout->chart;
    nk_zero(chart, sizeof(*chart));
    chart->x = bounds.x + style->padding.x;
    chart->y = bounds.y + style->padding.y;
    chart->w = bounds.w - 2 * style->padding.x;
    chart->h = bounds.h - 2 * style->padding.y;
    chart->w = NK_MAX(chart->w, 2 * style->padding.x);
    chart->h = NK_MAX(chart->h, 2 * style->padding.y);

    /* add first slot into chart */
    struct nk_chart_slot *slot = &chart->slots[chart->slot++];
    slot->type = type;
    slot->count = count;
    slot->color = color;
    slot->highlight = highlight;
    slot->min = NK_MIN(min_value, max_value);
    slot->max = NK_MAX(min_value, max_value);
    slot->range = slot->max - slot->min;

    /* draw chart background */
    const nk_style_item* background = &style->background;
    if (background->type == NK_STYLE_ITEM_IMAGE)
        nk_draw_image(&win->buffer, bounds, &background->image, nk_white);
    else
        nk_fill_rect_border(&win->buffer, bounds, style->rounding, style->background.color, style->border, style->border_color);

    return nk_true;
}

NK_API nk_bool
nk_chart_begin(struct nk_context *ctx, const enum nk_chart_type type, int count, float min_value, float max_value)
{
    return nk_chart_begin_colored(ctx, type, ctx->style.chart.color, ctx->style.chart.selected_color, count, min_value, max_value);
}

NK_API void nk_chart_end(struct nk_context* ctx)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    if (!ctx || !ctx->current) return;

    struct nk_chart* chart = &ctx->current->layout->chart;
    nk_memset(chart, 0, sizeof(*chart));
    return;
}

NK_API void
nk_chart_add_slot_colored(struct nk_context *ctx, const enum nk_chart_type type,
    nk_color color, nk_color highlight,
    int count, float min_value, float max_value)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    NK_ASSERT(ctx->current->layout->chart.slot < NK_CHART_MAX_SLOT);
    if (!ctx || !ctx->current || !ctx->current->layout) return;
    if (ctx->current->layout->chart.slot >= NK_CHART_MAX_SLOT) return;

    /* add another slot into the graph */
    struct nk_chart *chart = &ctx->current->layout->chart;
    struct nk_chart_slot *slot = &chart->slots[chart->slot++];
    slot->type = type;
    slot->count = count;
    slot->color = color;
    slot->highlight = highlight;
    slot->min = NK_MIN(min_value, max_value);
    slot->max = NK_MAX(min_value, max_value);
    slot->range = slot->max - slot->min;
}

NK_API void
nk_chart_add_slot(struct nk_context *ctx, const enum nk_chart_type type, int count, float min_value, float max_value)
{
    nk_chart_add_slot_colored(ctx, type, ctx->style.chart.color, ctx->style.chart.selected_color, count, min_value, max_value);
}

NK_INTERN nk_flags
nk_chart_push_line(struct nk_context *ctx, struct nk_window *win, struct nk_chart *g, float value, int slot)
{
    NK_ASSERT(slot >= 0 && slot < NK_CHART_MAX_SLOT);

    struct nk_panel* layout = win->layout;
    const struct nk_input* in = &ctx->input;
    float ratio = (value - g->slots[slot].min) / (g->slots[slot].max - g->slots[slot].min);

    /* first data point does not have a connection */
    if (g->slots[slot].index == 0)
    {
        g->slots[slot].last.x = g->x;
        g->slots[slot].last.y = (g->y + g->h) - ratio * (float)g->h;

        struct nk_rect bounds = {
            .x = g->slots[slot].last.x - 2,
            .y = g->slots[slot].last.y - 2,
            .w = 4,
            .h = 4
        };

        nk_color color = g->slots[slot].color;
        nk_flags ret = 0;
        if (!(layout->flags & NK_WINDOW_ROM) && nk_input_mouse_hover(in, bounds))
        {
            ret = NK_CHART_HOVERING;
            ret |= nk_input_mouse_released(in, NK_BUTTON_LEFT) ? NK_CHART_CLICKED: 0;
            color = g->slots[slot].highlight;
        }
        nk_fill_rect(&win->buffer, bounds, 0, color);
        g->slots[slot].index += 1;
        return ret;
    }

    /* draw a line between the last data point and the new one */
    nk_color color = g->slots[slot].color;
    float step = g->w / (float)g->slots[slot].count;
    struct nk_vec2 cur = {
        .x = g->x + (float)(step * (float)g->slots[slot].index),
        .y = (g->y + g->h) - (ratio * (float)g->h)
    };
    nk_stroke_line(&win->buffer, g->slots[slot].last.x, g->slots[slot].last.y, cur.x, cur.y, 1.0f, color);

    struct nk_rect bounds = {
        .x = cur.x - 2,
        .y = cur.y - 2,
        .w = 4,
        .h = 4
    };

    /* user selection of current data point */
    nk_flags ret = 0;
    if (!(layout->flags & NK_WINDOW_ROM) && nk_input_mouse_hover(in, bounds))
    {
        ret = NK_CHART_HOVERING;
        ret |= nk_input_mouse_released(in, NK_BUTTON_LEFT) ? NK_CHART_CLICKED : 0;
        color = g->slots[slot].highlight;
    }
    nk_fill_rect(&win->buffer, bounds, 0, color);

    /* save current data point position */
    g->slots[slot].last.x = cur.x;
    g->slots[slot].last.y = cur.y;
    g->slots[slot].index  += 1;
    return ret;
}

NK_INTERN nk_flags
nk_chart_push_column(const struct nk_context *ctx, struct nk_window *win, struct nk_chart *chart, float value, int slot)
{
    NK_ASSERT(slot >= 0 && slot < NK_CHART_MAX_SLOT);

    if (chart->slots[slot].index  >= chart->slots[slot].count)
        return nk_false;

    const struct nk_input* in = &ctx->input;

    struct nk_rect item = { 0 };
    if (chart->slots[slot].count)
    {
        float padding = (float)(chart->slots[slot].count-1);
        item.w = (chart->w - padding) / (float)(chart->slots[slot].count);
    }

    /* calculate bounds of current bar chart entry */
    float ratio;
    nk_color color = chart->slots[slot].color;;
    item.h = chart->h * NK_ABS((value/chart->slots[slot].range));
    if (value >= 0)
    {
        ratio = (value + NK_ABS(chart->slots[slot].min)) / NK_ABS(chart->slots[slot].range);
        item.y = (chart->y + chart->h) - chart->h * ratio;
    }
    else
    {
        ratio = (value - chart->slots[slot].max) / chart->slots[slot].range;
        item.y = chart->y + (chart->h * NK_ABS(ratio)) - item.h;
    }
    item.x = chart->x + ((float)chart->slots[slot].index * item.w);
    item.x = item.x + ((float)chart->slots[slot].index);

    /* user chart bar selection */
    nk_flags ret = 0;
    if (!(win->layout->flags & NK_WINDOW_ROM) && nk_input_mouse_hover(in, item))
    {
        ret = NK_CHART_HOVERING;
        ret |= nk_input_mouse_released(in, NK_BUTTON_LEFT) ? NK_CHART_CLICKED : 0;
        color = chart->slots[slot].highlight;
    }
    nk_fill_rect(&win->buffer, item, 0, color);
    chart->slots[slot].index += 1;
    return ret;
}

NK_API nk_flags nk_chart_push_slot(struct nk_context *ctx, float value, int slot)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(slot >= 0 && slot < NK_CHART_MAX_SLOT);
    NK_ASSERT(slot < ctx->current->layout->chart.slot);
    if (!ctx || !ctx->current || slot >= NK_CHART_MAX_SLOT) return nk_false;
    if (slot >= ctx->current->layout->chart.slot) return nk_false;

    struct nk_window* win = ctx->current;
    if (win->layout->chart.slot < slot) return nk_false;

    switch (win->layout->chart.slots[slot].type)
    {
    case NK_CHART_LINES:  return nk_chart_push_line(ctx, win, &win->layout->chart, value, slot);
    case NK_CHART_COLUMN: return nk_chart_push_column(ctx, win, &win->layout->chart, value, slot);
    }
    return 0;
}

NK_API nk_flags nk_chart_push(struct nk_context *ctx, float value)
{
    return nk_chart_push_slot(ctx, value, 0);
}

NK_API void
nk_plot(struct nk_context *ctx, enum nk_chart_type type, const float *values, int count, int offset)
{
    NK_ASSERT(ctx);
    NK_ASSERT(values);
    if (!ctx || !values || !count) return;

    float min_value = values[offset];
    float max_value = values[offset];
    for (int i = 0; i < count; ++i)
    {
        min_value = NK_MIN(values[i + offset], min_value);
        max_value = NK_MAX(values[i + offset], max_value);
    }

    if (!nk_chart_begin(ctx, type, count, min_value, max_value)) return;

    for (int i = 0; i < count; ++i)
        nk_chart_push(ctx, values[i + offset]);
    nk_chart_end(ctx);
}

NK_API void
nk_plot_function(struct nk_context *ctx, enum nk_chart_type type, void *userdata, float(*value_getter)(void* user, int index), int count, int offset)
{
    NK_ASSERT(ctx);
    NK_ASSERT(value_getter);
    if (!ctx || !value_getter || !count) return;

    float min_value = value_getter(userdata, offset);
    float max_value = min_value;
    for (int i = 0; i < count; ++i)
    {
        float value = value_getter(userdata, i + offset);
        min_value = NK_MIN(value, min_value);
        max_value = NK_MAX(value, max_value);
    }

    if (!nk_chart_begin(ctx, type, count, min_value, max_value)) return;

    for (int i = 0; i < count; ++i)
        nk_chart_push(ctx, value_getter(userdata, i + offset));
    nk_chart_end(ctx);
}

