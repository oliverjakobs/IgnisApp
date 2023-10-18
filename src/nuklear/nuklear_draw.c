#include "nuklear.h"
#include "nuklear_internal.h"

/* ==============================================================
 *
 *                          DRAW
 *
 * ===============================================================*/
static nk_bool nk_clip_rect(struct nk_rect rect, struct nk_rect clip)
{
    return NK_INTERSECT(rect.x, rect.y, rect.w, rect.h, clip.x, clip.y, clip.w, clip.h);
}

static nk_bool nk_clip_triangle(struct nk_vec2 a, struct nk_vec2 b, struct nk_vec2 c, struct nk_rect clip)
{
    return NK_INBOX(a.x, a.y, clip.x, clip.y, clip.w, clip.h)
        || NK_INBOX(b.x, b.y, clip.x, clip.y, clip.w, clip.h)
        || NK_INBOX(c.x, c.y, clip.x, clip.y, clip.w, clip.h);
}

NK_LIB void
nk_command_buffer_init(nk_command_buffer *cb, struct nk_buffer *b, enum nk_command_clipping clip)
{
    NK_ASSERT(cb);
    NK_ASSERT(b);
    if (!cb || !b) return;
    cb->base = b;
    cb->use_clipping = (int)clip;
    cb->begin = b->allocated;
    cb->end = b->allocated;
    cb->last = b->allocated;
}

NK_LIB void
nk_command_buffer_reset(nk_command_buffer *b)
{
    NK_ASSERT(b);
    if (!b) return;
    b->begin = 0;
    b->end = 0;
    b->last = 0;
    b->clip = nk_null_rect;
}

NK_LIB void* nk_command_buffer_push(nk_command_buffer* b, enum nk_command_type t, nk_size size)
{
    NK_STORAGE const nk_size align = NK_ALIGNOF(struct nk_command);

    NK_ASSERT(b);
    NK_ASSERT(b->base);
    if (!b) return 0;

    struct nk_command* cmd = nk_buffer_alloc(b->base, NK_BUFFER_FRONT, size, align);
    if (!cmd) return 0;

    /* make sure the offset to the next command is aligned */
    b->last = (nk_size)((nk_byte*)cmd - (nk_byte*)b->base->memory.ptr);

    void* unaligned = (nk_byte*)cmd + size;
    void* memory = NK_ALIGN_PTR(unaligned, align);
    nk_size alignment = (nk_size)((nk_byte*)memory - (nk_byte*)unaligned);

    cmd->type = t;
    cmd->next = b->base->allocated + alignment;
    b->end = cmd->next;
    return cmd;
}

static void nk_command_push_rect(nk_command_buffer* b, enum nk_command_type t, struct nk_rect rect, float rounding, float line_thickness, nk_color c)
{
    struct nk_command_rect* cmd = nk_command_buffer_push(b, t, sizeof(*cmd));
    if (!cmd) return;

    cmd->rounding = (unsigned short)rounding;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)NK_MAX(0, rect.w);
    cmd->h = (unsigned short)NK_MAX(0, rect.h);
    cmd->color = c;
}

static void nk_command_push_circle(struct nk_command_buffer* b, enum nk_command_type t, struct nk_vec2 center, float radius, float line_thickness, nk_color c)
{
    struct nk_command_circle* cmd = nk_command_buffer_push(b, t, sizeof(*cmd));
    if (!cmd) return;

    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)center.x;
    cmd->y = (short)center.y;
    cmd->r = (unsigned short)radius;
    cmd->color = c;
}

static void nk_command_push_triangle(nk_command_buffer* buf, enum nk_command_type t, struct nk_vec2 a, struct nk_vec2 b, struct nk_vec2 c, float line_thickness, nk_color col)
{
    struct nk_command_triangle* cmd = nk_command_buffer_push(buf, t, sizeof(*cmd));
    if (!cmd) return;

    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->a.x = (short)a.x;
    cmd->a.y = (short)a.y;
    cmd->b.x = (short)b.x;
    cmd->b.y = (short)b.y;
    cmd->c.x = (short)c.x;
    cmd->c.y = (short)c.y;
    cmd->color = col;
}

NK_API void nk_push_scissor(nk_command_buffer *b, struct nk_rect r)
{
    NK_ASSERT(b);
    if (!b) return;

    b->clip.x = r.x;
    b->clip.y = r.y;
    b->clip.w = r.w;
    b->clip.h = r.h;
    struct nk_command_scissor* cmd = nk_command_buffer_push(b, NK_COMMAND_SCISSOR, sizeof(*cmd));

    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)NK_MAX(0, r.w);
    cmd->h = (unsigned short)NK_MAX(0, r.h);
}

NK_API void
nk_stroke_line(nk_command_buffer *b, float x0, float y0, float x1, float y1, float line_thickness, nk_color c)
{
    NK_ASSERT(b);
    if (!b || line_thickness <= 0) return;

    struct nk_command_line* cmd = nk_command_buffer_push(b, NK_COMMAND_LINE, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->begin.x = (short)x0;
    cmd->begin.y = (short)y0;
    cmd->end.x = (short)x1;
    cmd->end.y = (short)y1;
    cmd->color = c;
}

NK_API void
nk_stroke_curve(nk_command_buffer *b, float ax, float ay,
    float ctrl0x, float ctrl0y, float ctrl1x, float ctrl1y,
    float bx, float by, float line_thickness, nk_color col)
{
    NK_ASSERT(b);
    if (!b || col.a == 0 || line_thickness <= 0) return;

    struct nk_command_curve* cmd = nk_command_buffer_push(b, NK_COMMAND_CURVE, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->begin.x = (short)ax;
    cmd->begin.y = (short)ay;
    cmd->ctrl[0].x = (short)ctrl0x;
    cmd->ctrl[0].y = (short)ctrl0y;
    cmd->ctrl[1].x = (short)ctrl1x;
    cmd->ctrl[1].y = (short)ctrl1y;
    cmd->end.x = (short)bx;
    cmd->end.y = (short)by;
    cmd->color = col;
}

NK_API void
nk_stroke_rect(nk_command_buffer *b, struct nk_rect rect, float rounding, float line_thickness, nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || rect.w == 0 || rect.h == 0 || line_thickness <= 0) return;
    if (b->use_clipping && !nk_clip_rect(rect, b->clip)) return;

    nk_command_push_rect(b, NK_COMMAND_RECT, rect, rounding, line_thickness, c);
}

NK_API void
nk_fill_rect(nk_command_buffer *b, struct nk_rect rect, float rounding, nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || rect.w == 0 || rect.h == 0) return;
    if (b->use_clipping && !nk_clip_rect(rect, b->clip)) return;

    nk_command_push_rect(b, NK_COMMAND_RECT_FILLED, rect, rounding, 0.0f, c);
}

NK_API void 
nk_fill_rect_border(nk_command_buffer *b, struct nk_rect rect, float rounding, nk_color color, float border, nk_color border_color)
{
    NK_ASSERT(b);
    if (!b || color.a == 0 || rect.w == 0 || rect.h == 0) return;
    if (b->use_clipping && !nk_clip_rect(rect, b->clip)) return;

    nk_command_push_rect(b, NK_COMMAND_RECT_FILLED, rect, rounding, 0.0f, color);
    nk_command_push_rect(b, NK_COMMAND_RECT, rect, rounding, border, border_color);
}

NK_API void
nk_fill_rect_multi_color(nk_command_buffer *b, struct nk_rect rect,
    nk_color left, nk_color top, nk_color right, nk_color bottom)
{
    NK_ASSERT(b);
    if (!b || rect.w == 0 || rect.h == 0) return;
    if (b->use_clipping && !nk_clip_rect(rect, b->clip)) return;

    struct nk_command_rect_multi_color* cmd = nk_command_buffer_push(b, NK_COMMAND_RECT_MULTI_COLOR, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)rect.w;
    cmd->h = (unsigned short)rect.h;
    cmd->left = left;
    cmd->top = top;
    cmd->right = right;
    cmd->bottom = bottom;
}

NK_API void
nk_stroke_circle(nk_command_buffer *b, struct nk_rect r, float line_thickness, nk_color c)
{
    NK_ASSERT(b);
    if (!b || r.w == 0 || r.h == 0 || line_thickness <= 0) return;
    if (b->use_clipping && !nk_clip_rect(r, b->clip)) return;

    float radius = r.w / 2;
    nk_command_push_circle(b, NK_COMMAND_CIRCLE, nk_vec2(r.x + radius, r.y + radius), radius, line_thickness, c);
}

NK_API void
nk_fill_circle(nk_command_buffer *b, struct nk_rect r, nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || r.w == 0 || r.h == 0) return;
    if (b->use_clipping && !nk_clip_rect(r, b->clip)) return;

    float radius = r.w / 2;
    nk_command_push_circle(b, NK_COMMAND_CIRCLE_FILLED, nk_vec2(r.x + radius, r.y + radius), radius, 0, c);
}

NK_API void
nk_fill_circle_border(nk_command_buffer* b, struct nk_rect r, nk_color c, float border, nk_color border_color)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || r.w == 0 || r.h == 0) return;
    if (b->use_clipping && !nk_clip_rect(r, b->clip)) return;

    float radius = r.w / 2;
    struct nk_vec2 center = nk_vec2(r.x + radius, r.y + radius);
    nk_command_push_circle(b, NK_COMMAND_CIRCLE_FILLED, center, radius, 0, c);
    nk_command_push_circle(b, NK_COMMAND_CIRCLE, center, radius, border, border_color);
}

NK_API void
nk_stroke_triangle(nk_command_buffer *buf, struct nk_vec2 a, struct nk_vec2 b, struct nk_vec2 c, float line_thickness, nk_color col)
{
    NK_ASSERT(buf);
    if (!buf || col.a == 0 || line_thickness <= 0) return;
    if (buf->use_clipping && !nk_clip_triangle(a, b, c, buf->clip)) return;

    nk_command_push_triangle(buf, NK_COMMAND_TRIANGLE, a, b, c, line_thickness, col);
}

NK_API void
nk_fill_triangle(nk_command_buffer *buf, struct nk_vec2 a, struct nk_vec2 b, struct nk_vec2 c, nk_color col)
{
    NK_ASSERT(buf);
    if (!buf || col.a == 0) return;
    if (buf->use_clipping && !nk_clip_triangle(a, b, c, buf->clip)) return;

    nk_command_push_triangle(buf, NK_COMMAND_TRIANGLE_FILLED, a, b, c, 0, col);
}

NK_API void
nk_draw_image(nk_command_buffer *b, struct nk_rect r, const nk_image *img, nk_color col)
{
    NK_ASSERT(b);
    if (!b) return;
    if (b->use_clipping && !nk_clip_rect(r, b->clip)) return;

    struct nk_command_image* cmd = nk_command_buffer_push(b, NK_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)r.w;
    cmd->h = (unsigned short)r.h;
    cmd->img = *img;
    cmd->col = col;
}

NK_API void
nk_draw_text(nk_command_buffer *b, struct nk_rect r,
    const char *string, int length, const struct nk_font *font,
    nk_color color)
{
    NK_ASSERT(b);
    NK_ASSERT(font);
    if (!b || !string || !length || color.a == 0) return;
    if (b->use_clipping && !nk_clip_rect(r, b->clip)) return;

    /* make sure text fits inside bounds */
    float text_width = font->width(font->userdata, font->height, string, length);
    if (text_width > r.w)
    {
        int glyphs = 0;
        length = nk_text_clamp(font, string, length, r.w, &glyphs, &text_width, 0,0);
    }

    if (!length) return;
    struct nk_command_text* cmd = nk_command_buffer_push(b, NK_COMMAND_TEXT, sizeof(*cmd) + (nk_size)length + 1);
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)r.w;
    cmd->h = (unsigned short)r.h;
    cmd->color = color;
    cmd->font = font;
    cmd->length = length;
    cmd->height = font->height;
    nk_memcopy(cmd->string, string, (nk_size)length);
    cmd->string[length] = '\0';
}

