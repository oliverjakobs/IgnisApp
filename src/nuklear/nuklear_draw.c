#include "nuklear.h"
#include "nuklear_internal.h"

/* ==============================================================
 *
 *                          DRAW
 *
 * ===============================================================*/
NK_LIB void
nk_command_buffer_init(struct nk_command_buffer *cb,
    struct nk_buffer *b, enum nk_command_clipping clip)
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
nk_command_buffer_reset(struct nk_command_buffer *b)
{
    NK_ASSERT(b);
    if (!b) return;
    b->begin = 0;
    b->end = 0;
    b->last = 0;
    b->clip = nk_null_rect;
}
NK_LIB void*
nk_command_buffer_push(struct nk_command_buffer* b,
    enum nk_command_type t, nk_size size)
{
    NK_STORAGE const nk_size align = NK_ALIGNOF(struct nk_command);
    struct nk_command *cmd;
    nk_size alignment;
    void *unaligned;
    void *memory;

    NK_ASSERT(b);
    NK_ASSERT(b->base);
    if (!b) return 0;
    cmd = (struct nk_command*)nk_buffer_alloc(b->base,NK_BUFFER_FRONT,size,align);
    if (!cmd) return 0;

    /* make sure the offset to the next command is aligned */
    b->last = (nk_size)((nk_byte*)cmd - (nk_byte*)b->base->memory.ptr);
    unaligned = (nk_byte*)cmd + size;
    memory = NK_ALIGN_PTR(unaligned, align);
    alignment = (nk_size)((nk_byte*)memory - (nk_byte*)unaligned);
#ifdef NK_ZERO_COMMAND_MEMORY
    nk_memset(cmd, 0, size + alignment);
#endif

    cmd->type = t;
    cmd->next = b->base->allocated + alignment;
    b->end = cmd->next;
    return cmd;
}

NK_API void nk_push_scissor(struct nk_command_buffer *b, struct nk_rect r)
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
nk_stroke_line(struct nk_command_buffer *b, float x0, float y0, float x1, float y1, float line_thickness, struct nk_color c)
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
nk_stroke_curve(struct nk_command_buffer *b, float ax, float ay,
    float ctrl0x, float ctrl0y, float ctrl1x, float ctrl1y,
    float bx, float by, float line_thickness, struct nk_color col)
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
nk_stroke_rect(struct nk_command_buffer *b, struct nk_rect rect, float rounding, float line_thickness, struct nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || rect.w == 0 || rect.h == 0 || line_thickness <= 0) return;

    if (b->use_clipping)
    {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INTERSECT(rect.x, rect.y, rect.w, rect.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    struct nk_command_rect* cmd = nk_command_buffer_push(b, NK_COMMAND_RECT, sizeof(*cmd));
    if (!cmd) return;
    cmd->rounding = (unsigned short)rounding;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)NK_MAX(0, rect.w);
    cmd->h = (unsigned short)NK_MAX(0, rect.h);
    cmd->color = c;
}

NK_API void
nk_fill_rect(struct nk_command_buffer *b, struct nk_rect rect, float rounding, struct nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || rect.w == 0 || rect.h == 0) return;

    if (b->use_clipping)
    {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INTERSECT(rect.x, rect.y, rect.w, rect.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    struct nk_command_rect* cmd = nk_command_buffer_push(b, NK_COMMAND_RECT_FILLED, sizeof(*cmd));
    if (!cmd) return;
    cmd->rounding = (unsigned short)rounding;
    cmd->line_thickness = 0.0f;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)NK_MAX(0, rect.w);
    cmd->h = (unsigned short)NK_MAX(0, rect.h);
    cmd->color = c;
}

NK_API void
nk_fill_rect_multi_color(struct nk_command_buffer *b, struct nk_rect rect,
    struct nk_color left, struct nk_color top, struct nk_color right,
    struct nk_color bottom)
{
    NK_ASSERT(b);
    if (!b || rect.w == 0 || rect.h == 0) return;
    if (b->use_clipping)
    {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INTERSECT(rect.x, rect.y, rect.w, rect.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    struct nk_command_rect_multi_color* cmd = nk_command_buffer_push(b, NK_COMMAND_RECT_MULTI_COLOR, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)rect.x;
    cmd->y = (short)rect.y;
    cmd->w = (unsigned short)NK_MAX(0, rect.w);
    cmd->h = (unsigned short)NK_MAX(0, rect.h);
    cmd->left = left;
    cmd->top = top;
    cmd->right = right;
    cmd->bottom = bottom;
}

NK_API void
nk_stroke_circle(struct nk_command_buffer *b, struct nk_rect r, float line_thickness, struct nk_color c)
{
    NK_ASSERT(b);
    if (!b || r.w == 0 || r.h == 0 || line_thickness <= 0) return;

    if (b->use_clipping)
    {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INTERSECT(r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    struct nk_command_circle* cmd = nk_command_buffer_push(b, NK_COMMAND_CIRCLE, sizeof(*cmd));
    if (!cmd) return;

    float radius = r.w / 2;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->x = (short)(r.x + radius);
    cmd->y = (short)(r.y + radius);
    cmd->r = (unsigned short)radius;
    cmd->color = c;
}

NK_API void
nk_fill_circle(struct nk_command_buffer *b, struct nk_rect r, struct nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0 || r.w == 0 || r.h == 0) return;

    if (b->use_clipping)
    {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INTERSECT(r.x, r.y, r.w, r.h, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    struct nk_command_circle* cmd = nk_command_buffer_push(b, NK_COMMAND_CIRCLE_FILLED, sizeof(*cmd));
    if (!cmd) return;

    float radius = r.w / 2;
    cmd->line_thickness = 0;
    cmd->x = (short)(r.x + radius);
    cmd->y = (short)(r.y + radius);
    cmd->r = (unsigned short)radius;
    cmd->color = c;
}
NK_API void
nk_stroke_triangle(struct nk_command_buffer *b, float x0, float y0, float x1,
    float y1, float x2, float y2, float line_thickness, struct nk_color c)
{
    struct nk_command_triangle *cmd;
    NK_ASSERT(b);
    if (!b || c.a == 0 || line_thickness <= 0) return;
    if (b->use_clipping) {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INBOX(x0, y0, clip->x, clip->y, clip->w, clip->h) &&
            !NK_INBOX(x1, y1, clip->x, clip->y, clip->w, clip->h) &&
            !NK_INBOX(x2, y2, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    cmd = (struct nk_command_triangle*) nk_command_buffer_push(b, NK_COMMAND_TRIANGLE, sizeof(*cmd));
    if (!cmd) return;
    cmd->line_thickness = (unsigned short)line_thickness;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
}

NK_API void
nk_fill_triangle(struct nk_command_buffer *b, float x0, float y0, float x1,
    float y1, float x2, float y2, struct nk_color c)
{
    NK_ASSERT(b);
    if (!b || c.a == 0) return;
    if (!b) return;
    if (b->use_clipping) {
        const struct nk_rect *clip = &b->clip;
        if (!NK_INBOX(x0, y0, clip->x, clip->y, clip->w, clip->h) &&
            !NK_INBOX(x1, y1, clip->x, clip->y, clip->w, clip->h) &&
            !NK_INBOX(x2, y2, clip->x, clip->y, clip->w, clip->h))
            return;
    }

    struct nk_command_triangle* cmd = nk_command_buffer_push(b, NK_COMMAND_TRIANGLE_FILLED, sizeof(*cmd));
    if (!cmd) return;
    cmd->a.x = (short)x0;
    cmd->a.y = (short)y0;
    cmd->b.x = (short)x1;
    cmd->b.y = (short)y1;
    cmd->c.x = (short)x2;
    cmd->c.y = (short)y2;
    cmd->color = c;
}

NK_API void
nk_draw_image(struct nk_command_buffer *b, struct nk_rect r,
    const struct nk_image *img, struct nk_color col)
{
    struct nk_command_image *cmd;
    NK_ASSERT(b);
    if (!b) return;
    if (b->use_clipping) {
        const struct nk_rect *c = &b->clip;
        if (c->w == 0 || c->h == 0 || !NK_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    cmd = (struct nk_command_image*)
        nk_command_buffer_push(b, NK_COMMAND_IMAGE, sizeof(*cmd));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)NK_MAX(0, r.w);
    cmd->h = (unsigned short)NK_MAX(0, r.h);
    cmd->img = *img;
    cmd->col = col;
}
NK_API void
nk_draw_nine_slice(struct nk_command_buffer *b, struct nk_rect r,
    const struct nk_nine_slice *slc, struct nk_color col)
{
    struct nk_image img;
    const struct nk_image *slcimg = (const struct nk_image*)slc;
    nk_ushort rgnX, rgnY, rgnW, rgnH;
    rgnX = slcimg->region[0];
    rgnY = slcimg->region[1];
    rgnW = slcimg->region[2];
    rgnH = slcimg->region[3];

    /* top-left */
    img.handle = slcimg->handle;
    img.w = slcimg->w;
    img.h = slcimg->h;
    img.region[0] = rgnX;
    img.region[1] = rgnY;
    img.region[2] = slc->l;
    img.region[3] = slc->t;

    nk_draw_image(b,
        nk_rect(r.x, r.y, (float)slc->l, (float)slc->t),
        &img, col);

#define IMG_RGN(x, y, w, h) img.region[0] = (nk_ushort)(x); img.region[1] = (nk_ushort)(y); img.region[2] = (nk_ushort)(w); img.region[3] = (nk_ushort)(h);

    /* top-center */
    IMG_RGN(rgnX + slc->l, rgnY, rgnW - slc->l - slc->r, slc->t);
    nk_draw_image(b,
        nk_rect(r.x + (float)slc->l, r.y, (float)(r.w - slc->l - slc->r), (float)slc->t),
        &img, col);

    /* top-right */
    IMG_RGN(rgnX + rgnW - slc->r, rgnY, slc->r, slc->t);
    nk_draw_image(b,
        nk_rect(r.x + r.w - (float)slc->r, r.y, (float)slc->r, (float)slc->t),
        &img, col);

    /* center-left */
    IMG_RGN(rgnX, rgnY + slc->t, slc->l, rgnH - slc->t - slc->b);
    nk_draw_image(b,
        nk_rect(r.x, r.y + (float)slc->t, (float)slc->l, (float)(r.h - slc->t - slc->b)),
        &img, col);

    /* center */
    IMG_RGN(rgnX + slc->l, rgnY + slc->t, rgnW - slc->l - slc->r, rgnH - slc->t - slc->b);
    nk_draw_image(b,
        nk_rect(r.x + (float)slc->l, r.y + (float)slc->t, (float)(r.w - slc->l - slc->r), (float)(r.h - slc->t - slc->b)),
        &img, col);

    /* center-right */
    IMG_RGN(rgnX + rgnW - slc->r, rgnY + slc->t, slc->r, rgnH - slc->t - slc->b);
    nk_draw_image(b,
        nk_rect(r.x + r.w - (float)slc->r, r.y + (float)slc->t, (float)slc->r, (float)(r.h - slc->t - slc->b)),
        &img, col);

    /* bottom-left */
    IMG_RGN(rgnX, rgnY + rgnH - slc->b, slc->l, slc->b);
    nk_draw_image(b,
        nk_rect(r.x, r.y + r.h - (float)slc->b, (float)slc->l, (float)slc->b),
        &img, col);

    /* bottom-center */
    IMG_RGN(rgnX + slc->l, rgnY + rgnH - slc->b, rgnW - slc->l - slc->r, slc->b);
    nk_draw_image(b,
        nk_rect(r.x + (float)slc->l, r.y + r.h - (float)slc->b, (float)(r.w - slc->l - slc->r), (float)slc->b),
        &img, col);

    /* bottom-right */
    IMG_RGN(rgnX + rgnW - slc->r, rgnY + rgnH - slc->b, slc->r, slc->b);
    nk_draw_image(b,
        nk_rect(r.x + r.w - (float)slc->r, r.y + r.h - (float)slc->b, (float)slc->r, (float)slc->b),
        &img, col);

#undef IMG_RGN
}

NK_API void
nk_draw_text(struct nk_command_buffer *b, struct nk_rect r,
    const char *string, int length, const struct nk_user_font *font,
    struct nk_color bg, struct nk_color fg)
{
    float text_width = 0;
    struct nk_command_text *cmd;

    NK_ASSERT(b);
    NK_ASSERT(font);
    if (!b || !string || !length || (bg.a == 0 && fg.a == 0)) return;
    if (b->use_clipping) {
        const struct nk_rect *c = &b->clip;
        if (c->w == 0 || c->h == 0 || !NK_INTERSECT(r.x, r.y, r.w, r.h, c->x, c->y, c->w, c->h))
            return;
    }

    /* make sure text fits inside bounds */
    text_width = font->width(font->userdata, font->height, string, length);
    if (text_width > r.w){
        int glyphs = 0;
        float txt_width = (float)text_width;
        length = nk_text_clamp(font, string, length, r.w, &glyphs, &txt_width, 0,0);
    }

    if (!length) return;
    cmd = (struct nk_command_text*)
        nk_command_buffer_push(b, NK_COMMAND_TEXT, sizeof(*cmd) + (nk_size)(length + 1));
    if (!cmd) return;
    cmd->x = (short)r.x;
    cmd->y = (short)r.y;
    cmd->w = (unsigned short)r.w;
    cmd->h = (unsigned short)r.h;
    cmd->background = bg;
    cmd->foreground = fg;
    cmd->font = font;
    cmd->length = length;
    cmd->height = font->height;
    nk_memcopy(cmd->string, string, (nk_size)length);
    cmd->string[length] = '\0';
}

