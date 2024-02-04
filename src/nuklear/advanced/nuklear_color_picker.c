#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ==============================================================
 *
 *                          COLOR PICKER
 *
 * ===============================================================*/
NK_LIB nk_colorf
nk_color_picker_behavior(nk_flags *state,
    const struct nk_rect bounds, const struct nk_rect matrix,
    const struct nk_rect hue_bar, const struct nk_rect alpha_bar,
    nk_colorf color, const struct nk_input *in)
{
    NK_ASSERT(state);
    nk_widget_state_reset(state);

    nk_bool value_changed = 0;
    float hsva[4];
    nk_colorf_hsva_fv(hsva, color);

    /* color matrix */
    nk_bool changed = nk_false;
    if (nk_button_behavior(state, matrix, in, NK_BUTTON_REPEATER))
    {
        hsva[1] = NK_SATURATE((in->mouse_pos.x - matrix.x) / (matrix.w - 1));
        hsva[2] = 1.0f - NK_SATURATE((in->mouse_pos.y - matrix.y) / (matrix.h - 1));
        changed = nk_true;
    }

    /* hue bar */
    if (nk_button_behavior(state, hue_bar, in, NK_BUTTON_REPEATER))
    {
        hsva[0] = NK_SATURATE((in->mouse_pos.y - hue_bar.y) / (hue_bar.h - 1));
        changed = nk_true;
    }

    if (changed)
    {
        color = nk_hsva_colorfv(hsva);
        *state = NK_WIDGET_STATE_ACTIVE;
    }

    /* alpha bar */
    if (nk_button_behavior(state, alpha_bar, in, NK_BUTTON_REPEATER))
    {
        color.a = 1.0f - NK_SATURATE((in->mouse_pos.y - alpha_bar.y) / (alpha_bar.h - 1));
        *state = NK_WIDGET_STATE_ACTIVE;
    }

    /* set color picker widget state */
    if (nk_input_mouse_hover(in, bounds))
        *state = NK_WIDGET_STATE_HOVER;

    if (*state & NK_WIDGET_STATE_HOVER && !nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_ENTERED;
    else if (nk_input_mouse_prev_hover(in, bounds))
        *state |= NK_WIDGET_STATE_LEFT;

    return color;
}

NK_LIB void
nk_draw_color_picker(struct nk_command_buffer *o, const struct nk_rect *matrix,
    const struct nk_rect *hue_bar, const struct nk_rect *alpha_bar,
    nk_colorf col)
{
    NK_STORAGE const nk_color black = {0,0,0,255};
    NK_STORAGE const nk_color white = {255, 255, 255, 255};
    NK_STORAGE const nk_color black_trans = {0,0,0,0};

    NK_ASSERT(o);
    NK_ASSERT(matrix);
    NK_ASSERT(hue_bar);

    float hsva[4];
    nk_colorf_hsva_fv(hsva, col);

    /* draw hue bar */
    for (int i = 0; i < 6; ++i)
    {
        NK_GLOBAL const nk_color hue_colors[] = {
            {255,   0,   0, 255},
            {255, 255,   0, 255},
            {  0, 255,   0, 255},
            {  0, 255, 255, 255},
            {  0,   0, 255, 255},
            {255,   0, 255, 255},
            {255,   0,   0, 255}
        };
        struct nk_rect rect = {
            .x = hue_bar->x,
            .y = hue_bar->y + (float)i * (hue_bar->h / 6.0f) + 0.5f,
            .w = hue_bar->w,
            .h = (hue_bar->h / 6.0f) + 0.5f
        };
        nk_fill_rect_multi_color(o, rect, hue_colors[i], hue_colors[i], hue_colors[i+1], hue_colors[i+1]);
    }
    float line_y = (float)(int)(hue_bar->y + hsva[0] * matrix->h + 0.5f);
    nk_stroke_line(o, hue_bar->x-1, line_y, hue_bar->x + hue_bar->w + 2, line_y, 1, white);

    /* draw alpha bar */
    if (alpha_bar)
    {
        line_y = (float)(int)(alpha_bar->y +  (1.0f - NK_SATURATE(col.a)) * matrix->h + 0.5f);

        nk_fill_rect_multi_color(o, *alpha_bar, white, white, black, black);
        nk_stroke_line(o, alpha_bar->x-1, line_y, alpha_bar->x + alpha_bar->w + 2, line_y, 1, white);
    }

    /* draw color matrix */
    nk_color color = nk_hsv_f(hsva[0], 1.0f, 1.0f);
    nk_fill_rect_multi_color(o, *matrix, white, color, color, white);
    nk_fill_rect_multi_color(o, *matrix, black_trans, black_trans, black, black);

    /* draw cross-hair */
    const float crosshair_size = 7.0f;
    float S = hsva[1];
    float V = hsva[2];
    struct nk_vec2 p = {
        .x = (float)(int)(matrix->x + S * matrix->w),
        .y = (float)(int)(matrix->y + (1.0f - V) * matrix->h)
    };
    nk_stroke_line(o, p.x - crosshair_size, p.y, p.x-2, p.y, 1.0f, white);
    nk_stroke_line(o, p.x + crosshair_size + 1, p.y, p.x+3, p.y, 1.0f, white);
    nk_stroke_line(o, p.x, p.y + crosshair_size + 1, p.x, p.y+3, 1.0f, white);
    nk_stroke_line(o, p.x, p.y - crosshair_size, p.x, p.y-2, 1.0f, white);
}

NK_LIB nk_colorf
nk_do_color_picker(nk_flags *state,
    struct nk_command_buffer *out, nk_colorf color,
    enum nk_color_format fmt, struct nk_rect bounds,
    struct nk_vec2 padding, const struct nk_input *in,
    const struct nk_font *font)
{
    NK_ASSERT(out);
    NK_ASSERT(state);
    NK_ASSERT(font);
    if (!out || !state || !font) return color;

    bounds.x += padding.x;
    bounds.y += padding.x;
    bounds.w -= 2 * padding.x;
    bounds.h -= 2 * padding.y;

    struct nk_rect matrix = bounds;
    matrix.w -= (3 * padding.x + 2 * font->height);

    struct nk_rect hue_bar = {
        .x = matrix.x + matrix.w + padding.x,
        .y = bounds.y,
        .w = font->height,
        .h = matrix.h
    };

    struct nk_rect alpha_bar = {0};
    if (fmt == NK_RGBA)
    {
        alpha_bar.x = hue_bar.x + hue_bar.w + padding.x;
        alpha_bar.y = bounds.y;
        alpha_bar.w = font->height;
        alpha_bar.h = matrix.h;
    }

    color = nk_color_picker_behavior(state, bounds, matrix, hue_bar, alpha_bar, color, in);
    nk_draw_color_picker(out, &matrix, &hue_bar, (fmt == NK_RGBA) ? &alpha_bar : NULL, color);
    return color;
}

NK_API nk_colorf
nk_color_picker(struct nk_context *ctx, nk_colorf color, enum nk_color_format fmt)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return color;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state layout_state;
    const struct nk_input* in = nk_widget_input(&bounds, &layout_state, ctx);
    if (!layout_state) return color;

    nk_flags state = 0;
    return nk_do_color_picker(&state, &win->buffer, color, fmt, bounds, nk_vec2(0, 0), in, style->font);
}

