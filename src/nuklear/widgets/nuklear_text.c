#include "../nuklear.h"
#include "../nuklear_internal.h"

/* ===============================================================
 *
 *                              TEXT
 *
 * ===============================================================*/
NK_LIB void
nk_widget_text(nk_command_buffer *out, struct nk_rect bounds, const char *string, int len,
    const nk_style_text *text, const struct nk_font *font)
{
    NK_ASSERT(out);
    NK_ASSERT(text);
    if (!out || !text) return;

    bounds.w = NK_MAX(bounds.w, 2 * text->padding.x);
    bounds.h = NK_MAX(bounds.h, 2 * text->padding.y);

    float text_width = font->width(font->userdata, font->height, string, len);

    struct nk_rect label;
    label.x = bounds.x + text->padding.x;
    label.y = bounds.y + text->padding.y;
    label.w = NK_MIN(text_width, bounds.w - 2.0f * text->padding.x);
    label.h = NK_MIN(font->height, bounds.h - 2.0f * text->padding.y);

    /* align in x-axis */
    if (text->alignment & NK_TEXT_ALIGN_CENTER)
        label.x += (bounds.w - label.w) * 0.5f;
    else if (text->alignment & NK_TEXT_ALIGN_RIGHT)
        label.x += bounds.w - label.w;

    /* align in y-axis */
    if (text->alignment & NK_TEXT_ALIGN_MIDDLE)
    {
        label.y = bounds.y + bounds.h/2.0f - font->height/2.0f;
        label.h = NK_MAX(bounds.h/2.0f, bounds.h - (bounds.h/2.0f + font->height/2.0f));
    }
    else if (text->alignment & NK_TEXT_ALIGN_BOTTOM)
    {
        label.y = bounds.y + bounds.h - font->height;
        label.h = font->height;
    }
    nk_draw_text(out, label, string, len, font, text->color);
}

NK_LIB void
nk_widget_text_wrap(nk_command_buffer *o, struct nk_rect b, const char *string, int len,
    const nk_style_text*t, const struct nk_font *f)
{
    NK_ASSERT(o);
    NK_ASSERT(t);
    if (!o || !t) return;

    NK_STORAGE const nk_rune sep[] = { ' ' };
    NK_STORAGE const int sep_len = NK_LEN(sep);

    b.w = NK_MAX(b.w, 2 * t->padding.x);
    b.h = NK_MAX(b.h, 2 * t->padding.y);
    b.h = b.h - 2 * t->padding.y;

    struct nk_rect line;
    line.x = b.x + t->padding.x;
    line.y = b.y + t->padding.y;
    line.w = b.w - 2 * t->padding.x;
    line.h = 2 * t->padding.y + f->height;

    float width;
    int glyphs = 0;
    int done = 0;
    int fitting = nk_text_clamp(f, string, len, line.w, &glyphs, &width, sep, sep_len);
    while (done < len)
    {
        if (!fitting || line.y + line.h >= (b.y + b.h)) break;
        nk_widget_text(o, line, &string[done], fitting, t, f);
        done += fitting;
        line.y += f->height + 2 * t->padding.y;
        fitting = nk_text_clamp(f, &string[done], len - done, line.w, &glyphs, &width, sep, sep_len);
    }
}

NK_API void nk_text(struct nk_context* ctx, const char* str, int len, nk_flags align)
{
    NK_ASSERT(ctx);
    NK_ASSERT(ctx->current);
    NK_ASSERT(ctx->current->layout);
    if (!ctx || !ctx->current || !ctx->current->layout) return;

    struct nk_window* win = ctx->current;
    const struct nk_style* style = &ctx->style;

    struct nk_rect bounds;
    nk_widget_layout_state state = nk_widget(&bounds, ctx);
    if (state == NK_WIDGET_INVALID) return;

    nk_style_text text = style->text;
    text.alignment = align;

    if (align & NK_TEXT_ALIGN_WRAP)
        nk_widget_text_wrap(&win->buffer, bounds, str, len, &text, style->font);
    else
        nk_widget_text(&win->buffer, bounds, str, len, &text, style->font);
}

NK_API void nk_label(struct nk_context* ctx, const char* str, nk_flags align)
{
    nk_text(ctx, str, nk_strlen(str), align);
}

NK_API void nk_labelfv(struct nk_context* ctx, nk_flags align, const char* fmt, va_list args)
{
    char buf[256];
    int len = nk_strfmt(buf, NK_LEN(buf), fmt, args);
    nk_text(ctx, buf, len, align);
}

NK_API void nk_labelf(struct nk_context* ctx, nk_flags align, const char* fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    nk_labelfv(ctx, align, fmt, args);
    va_end(args);
}

NK_API void nk_value_bool(struct nk_context *ctx, const char *prefix, int value)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %s", prefix, ((value) ? "true": "false"));
}

NK_API void nk_value_int(struct nk_context *ctx, const char *prefix, int value)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %d", prefix, value);
}

NK_API void nk_value_uint(struct nk_context *ctx, const char *prefix, unsigned int value)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %u", prefix, value);
}

NK_API void nk_value_float(struct nk_context *ctx, const char *prefix, float value)
{
    double double_value = (double)value;
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %.3f", prefix, double_value);
}

NK_API void nk_value_color_byte(struct nk_context *ctx, const char *p, nk_color c)
{
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: (%d, %d, %d, %d)", p, c.r, c.g, c.b, c.a);
}

NK_API void nk_value_color_float(struct nk_context *ctx, const char *p, nk_color color)
{
    double c[4]; nk_color_dv(c, color);
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: (%.2f, %.2f, %.2f, %.2f)", p, c[0], c[1], c[2], c[3]);
}

NK_API void nk_value_color_hex(struct nk_context *ctx, const char *prefix, nk_color color)
{
    char hex[16];
    nk_color_hex_rgba(hex, color);
    nk_labelf(ctx, NK_TEXT_LEFT, "%s: %s", prefix, hex);
}

