#include "font.h"
#include "nuklear/nuklear_internal.h"


/* -------------------------------------------------------------
 *
 *                          RECT PACK
 *
 * --------------------------------------------------------------*/

#define STB_RECT_PACK_IMPLEMENTATION
#include "nuklear/stb_rect_pack.h"

/*
 * ==============================================================
 *
 *                          TRUETYPE
 *
 * ===============================================================
 */
#define STBTT_MAX_OVERSAMPLE   8
#define STB_TRUETYPE_IMPLEMENTATION
#include "nuklear/stb_truetype.h"

/* -------------------------------------------------------------
 *
 *                          FONT BAKING
 *
 * --------------------------------------------------------------*/
struct font_bake_data {
    struct stbtt_fontinfo info;
    struct stbrp_rect *rects;
    stbtt_pack_range *ranges;
    rune range_count;
};

struct font_baker {
    struct stbtt_pack_context spc;
    struct font_bake_data *build;
    stbtt_packedchar *packed;
    struct stbrp_rect *rects;
    stbtt_pack_range *ranges;
};

NK_GLOBAL const nk_size nk_rect_align = NK_ALIGNOF(struct stbrp_rect);
NK_GLOBAL const nk_size nk_range_align = NK_ALIGNOF(stbtt_pack_range);
NK_GLOBAL const nk_size nk_char_align = NK_ALIGNOF(stbtt_packedchar);
NK_GLOBAL const nk_size nk_build_align = NK_ALIGNOF(struct font_bake_data);
NK_GLOBAL const nk_size nk_baker_align = NK_ALIGNOF(struct font_baker);

NK_INTERN int nk_range_count(const rune *range)
{
    const rune *iter = range;
    NK_ASSERT(range);
    if (!range) return 0;
    while (*(iter++) != 0);
    return (iter == range) ? 0 : (int)((iter - range)/2);
}

NK_INTERN int nk_range_glyph_count(const rune *range, int count)
{
    int total_glyphs = 0;
    for (int i = 0; i < count; ++i)
    {
        rune f = range[(i*2)+0];
        rune t = range[(i*2)+1];
        NK_ASSERT(t >= f);
        total_glyphs += (int)((t - f) + 1);
    }
    return total_glyphs;
}

NK_API const rune* font_default_glyph_ranges(void)
{
    NK_STORAGE const rune ranges[] = {0x0020, 0x00FF, 0};
    return ranges;
}

NK_API const rune* font_chinese_glyph_ranges(void)
{
    NK_STORAGE const rune ranges[] = {
        0x0020, 0x00FF,
        0x3000, 0x30FF,
        0x31F0, 0x31FF,
        0xFF00, 0xFFEF,
        0x4E00, 0x9FAF,
        0
    };
    return ranges;
}

NK_API const rune* font_cyrillic_glyph_ranges(void)
{
    NK_STORAGE const rune ranges[] = {
        0x0020, 0x00FF,
        0x0400, 0x052F,
        0x2DE0, 0x2DFF,
        0xA640, 0xA69F,
        0
    };
    return ranges;
}

NK_API const rune* font_korean_glyph_ranges(void)
{
    NK_STORAGE const rune ranges[] = {
        0x0020, 0x00FF,
        0x3131, 0x3163,
        0xAC00, 0xD79D,
        0
    };
    return ranges;
}

NK_INTERN void font_baker_memory(nk_size *temp, int *glyph_count, struct font_config *config_list, int count)
{
    int range_count = 0;
    int total_range_count = 0;

    NK_ASSERT(config_list);
    NK_ASSERT(glyph_count);
    if (!config_list)
    {
        *temp = 0;
        *glyph_count = 0;
        return;
    }

    *glyph_count = 0;
    for (int i = 0; i < count; ++i)
    {
        struct font_config* iter = &config_list[i];
        if (!iter->range) iter->range = font_default_glyph_ranges();
        range_count = nk_range_count(iter->range);
        total_range_count += range_count;
        *glyph_count += nk_range_glyph_count(iter->range, range_count);
    }

    *temp = (nk_size)*glyph_count * sizeof(struct stbrp_rect);
    *temp += (nk_size)total_range_count * sizeof(stbtt_pack_range);
    *temp += (nk_size)*glyph_count * sizeof(stbtt_packedchar);
    *temp += (nk_size)count * sizeof(struct font_bake_data);
    *temp += sizeof(struct font_baker);
    *temp += nk_rect_align + nk_range_align + nk_char_align;
    *temp += nk_build_align + nk_baker_align;
}

NK_INTERN struct font_baker* font_baker(void *memory, int glyph_count, int count)
{
    if (!memory) return 0;
    /* setup baker inside a memory block  */
    struct font_baker* baker = (struct font_baker*)NK_ALIGN_PTR(memory, nk_baker_align);
    baker->build  = (struct font_bake_data*)NK_ALIGN_PTR((baker + 1), nk_build_align);
    baker->packed = (stbtt_packedchar*)NK_ALIGN_PTR((baker->build + count), nk_char_align);
    baker->rects  = (struct stbrp_rect*)NK_ALIGN_PTR((baker->packed + glyph_count), nk_rect_align);
    baker->ranges = (stbtt_pack_range*)NK_ALIGN_PTR((baker->rects + glyph_count), nk_range_align);
    return baker;
}

NK_INTERN int font_bake_pack(struct font_baker *baker, 
    nk_size *image_memory, int *width, int *height, struct nk_recti *custom,
    const struct font_config *config_list, int count)
{
    NK_STORAGE const nk_size max_height = 1024 * 32;
    int total_glyph_count = 0;
    int total_range_count = 0;

    NK_ASSERT(image_memory);
    NK_ASSERT(width);
    NK_ASSERT(height);
    NK_ASSERT(config_list);
    NK_ASSERT(count);

    if (!image_memory || !width || !height || !config_list || !count) return nk_false;

    for (int i = 0; i < count; ++i)
    {
        struct font_config* iter = &config_list[i];
        int range_count = nk_range_count(iter->range);
        total_range_count += range_count;
        total_glyph_count += nk_range_glyph_count(iter->range, range_count);
    }

    /* setup font baker from temporary memory */
    for (int i = 0; i < count; ++i)
    {
        struct font_config* iter = &config_list[i];
        struct stbtt_fontinfo* font_info = &baker->build[i].info;

        if (!stbtt_InitFont(font_info, (const unsigned char*)iter->ttf_blob, stbtt_GetFontOffsetForIndex((const unsigned char*)iter->ttf_blob, 0)))
            return nk_false;
    }

    *height = 0;
    *width = (total_glyph_count > 1000) ? 1024 : 512;
    stbtt_PackBegin(&baker->spc, 0, (int)*width, (int)max_height, 0, 1, NULL);
    {
        int input_i = 0;
        int range_n = 0;
        int rect_n = 0;
        int char_n = 0;

        if (custom) {
            /* pack custom user data first so it will be in the upper left corner*/
            struct stbrp_rect custom_space;
            nk_zero(&custom_space, sizeof(custom_space));
            custom_space.w = (stbrp_coord)(custom->w);
            custom_space.h = (stbrp_coord)(custom->h);

            stbtt_PackSetOversampling(&baker->spc, 1, 1);
            stbrp_pack_rects((struct stbrp_context*)baker->spc.pack_info, &custom_space, 1);
            *height = NK_MAX(*height, (int)(custom_space.y + custom_space.h));

            custom->x = (short)custom_space.x;
            custom->y = (short)custom_space.y;
            custom->w = (short)custom_space.w;
            custom->h = (short)custom_space.h;
        }

        /* first font pass: pack all glyphs */
        for (int i = 0; i < count; ++i)
        {
            struct font_config* iter = &config_list[i];
            struct font_bake_data* tmp = &baker->build[i];

            /* count glyphs + ranges in current font */
            int glyph_count = 0; 
            int range_count = 0;
            for (const rune* in_range = iter->range; in_range[0] && in_range[1]; in_range += 2) {
                glyph_count += (int)(in_range[1] - in_range[0]) + 1;
                range_count++;
            }

            /* setup ranges  */
            tmp->ranges = baker->ranges + range_n;
            tmp->range_count = (rune)range_count;
            range_n += range_count;
            for (int r = 0; r < range_count; ++r) {
                const rune* in_range = &iter->range[r * 2];
                tmp->ranges[r].font_size = iter->size;
                tmp->ranges[r].first_unicode_codepoint_in_range = (int)in_range[0];
                tmp->ranges[r].num_chars = (int)(in_range[1] - in_range[0]) + 1;
                tmp->ranges[r].chardata_for_range = baker->packed + char_n;
                char_n += tmp->ranges[r].num_chars;
            }

            /* pack */
            tmp->rects = baker->rects + rect_n;
            rect_n += glyph_count;
            stbtt_PackSetOversampling(&baker->spc, iter->oversample_h, iter->oversample_v);
            int n = stbtt_PackFontRangesGatherRects(&baker->spc, &tmp->info, tmp->ranges, (int)tmp->range_count, tmp->rects);
            stbrp_pack_rects((struct stbrp_context*)baker->spc.pack_info, tmp->rects, (int)n);

            /* texture height */
            for (int t = 0; t < n; ++t) {
                if (tmp->rects[t].was_packed)
                    *height = NK_MAX(*height, tmp->rects[t].y + tmp->rects[t].h);
            }
        }
        NK_ASSERT(rect_n == total_glyph_count);
        NK_ASSERT(char_n == total_glyph_count);
        NK_ASSERT(range_n == total_range_count);
    }
    *height = (int)nk_round_up_pow2((nk_uint)*height);
    *image_memory = (nk_size)(*width) * (nk_size)(*height);
    return nk_true;
}

NK_INTERN void
font_bake(struct font_baker *baker, void *image_memory, int width, int height,
    struct font_glyph *glyphs, int glyphs_count,
    const struct font_config *config_list, int font_count)
{
    rune glyph_n = 0;

    NK_ASSERT(image_memory);
    NK_ASSERT(width);
    NK_ASSERT(height);
    NK_ASSERT(config_list);
    NK_ASSERT(baker);
    NK_ASSERT(font_count);
    NK_ASSERT(glyphs_count);
    if (!image_memory || !width || !height || !config_list || !font_count || !glyphs || !glyphs_count)
        return;

    /* second font pass: render glyphs */
    nk_zero(image_memory, (nk_size)((nk_size)width * (nk_size)height));
    baker->spc.pixels = (unsigned char*)image_memory;
    baker->spc.height = (int)height;

    for (int i = 0; i < font_count; ++i)
    {
        struct font_config* iter = &config_list[i];
        struct font_bake_data* tmp = &baker->build[i];
        stbtt_PackSetOversampling(&baker->spc, iter->oversample_h, iter->oversample_v);
        stbtt_PackFontRangesRenderIntoRects(&baker->spc, &tmp->info, tmp->ranges, (int)tmp->range_count, tmp->rects);
    }
    stbtt_PackEnd(&baker->spc);

    /* third pass: setup font and glyphs */
    for (int i = 0; i < font_count; ++i)
    {
        struct font_config* iter = &config_list[i];
        struct font_bake_data* tmp = &baker->build[i];

        float font_scale = stbtt_ScaleForPixelHeight(&tmp->info, iter->size);
        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&tmp->info, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        /* fill baked font */
        struct baked_font* dst_font = &iter->font;
        dst_font->ranges = iter->range;
        dst_font->height = iter->size;
        dst_font->ascent = ((float)unscaled_ascent * font_scale);
        dst_font->descent = ((float)unscaled_descent * font_scale);
        dst_font->glyph_offset = glyph_n;
        /*
            Need to zero this, or it will carry over from a previous
            bake, and cause a segfault when accessing glyphs[].
        */
        dst_font->glyph_count = 0;
        rune glyph_count = 0;

        /* fill own baked font glyph array */
        for (int r = 0; r < tmp->range_count; ++r)
        {
            stbtt_pack_range* range = &tmp->ranges[r];
            for (int char_idx = 0; char_idx < range->num_chars; char_idx++)
            {
                rune codepoint = 0;
                float dummy_x = 0, dummy_y = 0;
                stbtt_aligned_quad q;
                struct font_glyph* glyph;

                /* query glyph bounds from stb_truetype */
                const stbtt_packedchar* pc = &range->chardata_for_range[char_idx];
                codepoint = (rune)(range->first_unicode_codepoint_in_range + char_idx);
                stbtt_GetPackedQuad(range->chardata_for_range, (int)width, (int)height, char_idx, &dummy_x, &dummy_y, &q, 0);

                /* fill own glyph type with data */
                glyph = &glyphs[dst_font->glyph_offset + dst_font->glyph_count + (unsigned int)glyph_count];
                glyph->codepoint = codepoint;
                glyph->x0 = q.x0; glyph->y0 = q.y0;
                glyph->x1 = q.x1; glyph->y1 = q.y1;
                glyph->y0 += (dst_font->ascent + 0.5f);
                glyph->y1 += (dst_font->ascent + 0.5f);
                glyph->w = glyph->x1 - glyph->x0 + 0.5f;
                glyph->h = glyph->y1 - glyph->y0;

                if (iter->coord_type == IGNIS_COORD_PIXEL) {
                    glyph->u0 = q.s0 * (float)width;
                    glyph->v0 = q.t0 * (float)height;
                    glyph->u1 = q.s1 * (float)width;
                    glyph->v1 = q.t1 * (float)height;
                }
                else {
                    glyph->u0 = q.s0;
                    glyph->v0 = q.t0;
                    glyph->u1 = q.s1;
                    glyph->v1 = q.t1;
                }
                glyph->xadvance = (pc->xadvance + iter->spacing.x);
                if (iter->pixel_snap)
                    glyph->xadvance = (float)(int)(glyph->xadvance + 0.5f);
                glyph_count++;
            }
        }
        dst_font->glyph_count += glyph_count;
        glyph_n += glyph_count;
    }
}

NK_INTERN void 
font_bake_custom_data(void *img_memory, int img_width, int img_height,
    struct nk_recti img_dst, const char *texture_data_mask, int tex_width,
    int tex_height, char white, char black)
{
    nk_byte *pixels;
    int y = 0;
    int x = 0;
    int n = 0;

    NK_ASSERT(img_memory);
    NK_ASSERT(img_width);
    NK_ASSERT(img_height);
    NK_ASSERT(texture_data_mask);
    NK_UNUSED(tex_height);
    if (!img_memory || !img_width || !img_height || !texture_data_mask)
        return;

    pixels = (nk_byte*)img_memory;
    for (y = 0, n = 0; y < tex_height; ++y) {
        for (x = 0; x < tex_width; ++x, ++n) {
            const int off0 = ((img_dst.x + x) + (img_dst.y + y) * img_width);
            const int off1 = off0 + 1 + tex_width;
            pixels[off0] = (texture_data_mask[n] == white) ? 0xFF : 0x00;
            pixels[off1] = (texture_data_mask[n] == black) ? 0xFF : 0x00;
        }
    }
}

NK_INTERN void font_bake_convert(void *out_memory, int img_width, int img_height, const void *in_memory)
{
    int n = 0;
    rune *dst;
    const nk_byte *src;

    NK_ASSERT(out_memory);
    NK_ASSERT(in_memory);
    NK_ASSERT(img_width);
    NK_ASSERT(img_height);
    if (!out_memory || !in_memory || !img_height || !img_width) return;

    dst = (rune*)out_memory;
    src = (const nk_byte*)in_memory;
    for (n = (int)(img_width * img_height); n > 0; n--)
        *dst++ = ((rune)(*src++) << 24) | 0x00FFFFFF;
}

/* -------------------------------------------------------------
 *
 *                          FONT
 *
 * --------------------------------------------------------------*/
NK_INTERN float font_text_width(nk_handle handle, float height, const char *text, int len)
{
    struct font *font = (struct font*)handle.ptr;

    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    if (!font || !text || !len)
        return 0;

    float scale = height / font->size;

    rune unicode;
    int glyph_len = nk_utf_decode(text, &unicode, (int)len);
    if (!glyph_len) return 0;

    float text_width = 0;
    int text_len = glyph_len;
    while (text_len <= len && glyph_len)
    {
        if (unicode == NK_UTF_INVALID) break;

        /* query currently drawn glyph information */
        const struct font_glyph* g = font_find_glyph(font, unicode);
        text_width += g->xadvance * scale;

        /* offset next glyph */
        glyph_len = nk_utf_decode(text + text_len, &unicode, (int)len - text_len);
        text_len += glyph_len;
    }
    return text_width;
}

NK_INTERN void
font_query_font_glyph(nk_handle handle, float height, struct nk_user_font_glyph *glyph, rune codepoint, rune next)
{
    NK_ASSERT(glyph);
    NK_UNUSED(next);

    struct font* font = (struct font*)handle.ptr;
    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    if (!font || !glyph)
        return;

    float scale = height/font->size;
    const struct font_glyph* g = font_find_glyph(font, codepoint);
    glyph->width = (g->x1 - g->x0) * scale;
    glyph->height = (g->y1 - g->y0) * scale;
    glyph->offset = nk_vec2(g->x0 * scale, g->y0 * scale);
    glyph->xadvance = (g->xadvance * scale);
    glyph->uv[0] = nk_vec2(g->u0, g->v0);
    glyph->uv[1] = nk_vec2(g->u1, g->v1);
}

NK_API const struct font_glyph* font_find_glyph(struct font *font, rune unicode)
{
    NK_ASSERT(font);
    NK_ASSERT(font->glyphs);
    if (!font || !font->glyphs) return 0;

    const struct font_glyph* glyph = font->fallback;

    int total_glyphs = 0;
    int count = nk_range_count(font->range);
    for (int i = 0; i < count; ++i)
    {
        rune f = font->range[(i * 2) + 0];
        rune t = font->range[(i * 2) + 1];
        int diff = (int)((t - f) + 1);
        if (unicode >= f && unicode <= t)
            return &font->glyphs[((rune)total_glyphs + (unicode - f))];
        total_glyphs += diff;
    }
    return glyph;
}

NK_INTERN void font_init(struct font *font, struct font_config* config, struct font_glyph *glyphs)
{
    NK_ASSERT(font);
    NK_ASSERT(glyphs);
    if (!font || !glyphs)
        return;

    font->size = config->font.height;
    font->range = config->range;
    font->glyphs = &glyphs[config->font.glyph_offset];
    font->fallback = font_find_glyph(font, config->fallback_glyph);

    font->handle.userdata.ptr = font;
    font->handle.height = config->size;
    font->handle.width = font_text_width;
    font->handle.query = font_query_font_glyph;
    font->handle.texture = nk_handle_ptr(0);
}

#define NK_CURSOR_DATA_W 90
#define NK_CURSOR_DATA_H 27
NK_GLOBAL const char nk_custom_cursor_data[NK_CURSOR_DATA_W * NK_CURSOR_DATA_H + 1] =
{
    "..-         -XXXXXXX-    X    -           X           -XXXXXXX          -          XXXXXXX"
    "..-         -X.....X-   X.X   -          X.X          -X.....X          -          X.....X"
    "---         -XXX.XXX-  X...X  -         X...X         -X....X           -           X....X"
    "X           -  X.X  - X.....X -        X.....X        -X...X            -            X...X"
    "XX          -  X.X  -X.......X-       X.......X       -X..X.X           -           X.X..X"
    "X.X         -  X.X  -XXXX.XXXX-       XXXX.XXXX       -X.X X.X          -          X.X X.X"
    "X..X        -  X.X  -   X.X   -          X.X          -XX   X.X         -         X.X   XX"
    "X...X       -  X.X  -   X.X   -    XX    X.X    XX    -      X.X        -        X.X      "
    "X....X      -  X.X  -   X.X   -   X.X    X.X    X.X   -       X.X       -       X.X       "
    "X.....X     -  X.X  -   X.X   -  X..X    X.X    X..X  -        X.X      -      X.X        "
    "X......X    -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -         X.X   XX-XX   X.X         "
    "X.......X   -  X.X  -   X.X   -X.....................X-          X.X X.X-X.X X.X          "
    "X........X  -  X.X  -   X.X   - X...XXXXXX.XXXXXX...X -           X.X..X-X..X.X           "
    "X.........X -XXX.XXX-   X.X   -  X..X    X.X    X..X  -            X...X-X...X            "
    "X..........X-X.....X-   X.X   -   X.X    X.X    X.X   -           X....X-X....X           "
    "X......XXXXX-XXXXXXX-   X.X   -    XX    X.X    XX    -          X.....X-X.....X          "
    "X...X..X    ---------   X.X   -          X.X          -          XXXXXXX-XXXXXXX          "
    "X..X X..X   -       -XXXX.XXXX-       XXXX.XXXX       ------------------------------------"
    "X.X  X..X   -       -X.......X-       X.......X       -    XX           XX    -           "
    "XX    X..X  -       - X.....X -        X.....X        -   X.X           X.X   -           "
    "      X..X          -  X...X  -         X...X         -  X..X           X..X  -           "
    "       XX           -   X.X   -          X.X          - X...XXXXXXXXXXXXX...X -           "
    "------------        -    X    -           X           -X.....................X-           "
    "                    ----------------------------------- X...XXXXXXXXXXXXX...X -           "
    "                                                      -  X..X           X..X  -           "
    "                                                      -   X.X           X.X   -           "
    "                                                      -    XX           XX    -           "
};

/* -------------------------------------------------------------
 *
 *                          FONT ATLAS
 *
 * --------------------------------------------------------------*/
struct font_config font_default_config(float pixel_height)
{
    struct font_config cfg;
    nk_zero_struct(cfg);
    cfg.ttf_blob = 0;
    cfg.ttf_size = 0;
    cfg.ttf_data_owned_by_atlas = 0;
    cfg.size = pixel_height;
    cfg.oversample_h = 3;
    cfg.oversample_v = 1;
    cfg.pixel_snap = 0;
    cfg.coord_type = IGNIS_COORD_UV;
    cfg.spacing = nk_vec2(0,0);
    cfg.range = font_default_glyph_ranges();
    cfg.fallback_glyph = '?';
    return cfg;
}

void font_atlas_init(struct font_atlas *atlas)
{
    NK_ASSERT(atlas);
    if (!atlas) return;
    nk_zero_struct(*atlas);
}

NK_API void
font_atlas_begin(struct font_atlas *atlas, int count)
{
    NK_ASSERT(atlas);
    if (atlas->glyphs)
    {
        free(atlas->glyphs);
        atlas->glyphs = 0;
    }

    if (atlas->pixel)
    {
        free(atlas->pixel);
        atlas->pixel = 0;
    }

    atlas->fonts = malloc(sizeof(struct font) * count);
    atlas->configs = malloc(sizeof(struct font_config) * count);
    atlas->font_cap = count;
}
NK_API struct font*
font_atlas_add(struct font_atlas *atlas, const struct font_config *config)
{
    NK_ASSERT(atlas);

    NK_ASSERT(config);
    NK_ASSERT(config->ttf_blob);
    NK_ASSERT(config->ttf_size);
    NK_ASSERT(config->size > 0.0f);

    if (!atlas || !config || !config->ttf_blob || !config->ttf_size || config->size <= 0.0f)
        return 0;

    NK_ASSERT(atlas->font_count < atlas->font_cap);

    /* add font config  */
    struct font_config* cfg = &atlas->configs[atlas->font_count];
    nk_memcopy(cfg, config, sizeof(*config));

    /* add new font */
    struct font* font = &atlas->fonts[atlas->font_count];
    nk_zero(font, sizeof(*font));

    /* create own copy of .TTF font blob */
    if (!config->ttf_data_owned_by_atlas) {
        cfg->ttf_blob = malloc(cfg->ttf_size);
        NK_ASSERT(cfg->ttf_blob);
        if (!cfg->ttf_blob) {
            atlas->font_count++;
            return 0;
        }
        nk_memcopy(cfg->ttf_blob, config->ttf_blob, cfg->ttf_size);
        cfg->ttf_data_owned_by_atlas = 1;
    }
    atlas->font_count++;
    return font;
}
NK_API struct font*
font_atlas_add_from_memory(struct font_atlas *atlas, void *memory,
    nk_size size, float height, const struct font_config *config)
{
    struct font_config cfg;
    NK_ASSERT(memory);
    NK_ASSERT(size);

    NK_ASSERT(atlas);
    if (!atlas || !memory || !size)
        return 0;

    cfg = (config) ? *config : font_default_config(height);
    cfg.ttf_blob = memory;
    cfg.ttf_size = size;
    cfg.size = height;
    cfg.ttf_data_owned_by_atlas = 0;
    return font_atlas_add(atlas, &cfg);
}
NK_API struct font*
font_atlas_add_from_file(struct font_atlas *atlas, const char *file_path,
    float height, const struct font_config *config)
{
    nk_size size;
    char *memory;
    struct font_config cfg;

    NK_ASSERT(atlas);

    if (!atlas || !file_path) return 0;
    memory = ignisReadFile(file_path, &size);
    if (!memory) return 0;

    cfg = (config) ? *config : font_default_config(height);
    cfg.ttf_blob = memory;
    cfg.ttf_size = size;
    cfg.size = height;
    cfg.ttf_data_owned_by_atlas = 1;
    return font_atlas_add(atlas, &cfg);
}

NK_API const void*
font_atlas_bake(struct font_atlas *atlas, int *width, int *height, enum font_atlas_format fmt)
{
    void *tmp = 0;
    nk_size tmp_size, img_size;
    struct font_baker *baker;

    NK_ASSERT(atlas);

    NK_ASSERT(width);
    NK_ASSERT(height);
    if (!atlas || !width || !height)
        return 0;

    NK_ASSERT(atlas->font_count);
    if (!atlas->font_count) return 0;

    /* allocate temporary baker memory required for the baking process */
    font_baker_memory(&tmp_size, &atlas->glyph_count, atlas->configs, atlas->font_count);
    tmp = malloc(tmp_size);
    NK_ASSERT(tmp);
    if (!tmp) goto failed;
    nk_memset(tmp,0,tmp_size);

    /* allocate glyph memory for all fonts */
    baker = font_baker(tmp, atlas->glyph_count, atlas->font_count);
    atlas->glyphs = malloc(sizeof(struct font_glyph)*(nk_size)atlas->glyph_count);
    NK_ASSERT(atlas->glyphs);
    if (!atlas->glyphs)
        goto failed;

    /* pack all glyphs into a tight fit space */
    atlas->custom.w = (NK_CURSOR_DATA_W*2)+1;
    atlas->custom.h = NK_CURSOR_DATA_H + 1;
    if (!font_bake_pack(baker, &img_size, width, height, &atlas->custom, atlas->configs, atlas->font_count))
        goto failed;

    /* allocate memory for the baked image font atlas */
    atlas->pixel = malloc(img_size);
    NK_ASSERT(atlas->pixel);
    if (!atlas->pixel)
        goto failed;

    /* bake glyphs and custom white pixel into image */
    font_bake(baker, atlas->pixel, *width, *height,
        atlas->glyphs, atlas->glyph_count, atlas->configs, atlas->font_count);
    font_bake_custom_data(atlas->pixel, *width, *height, atlas->custom,
            nk_custom_cursor_data, NK_CURSOR_DATA_W, NK_CURSOR_DATA_H, '.', 'X');

    if (fmt == NK_FONT_ATLAS_RGBA32) {
        /* convert alpha8 image into rgba32 image */
        void *img_rgba = malloc((nk_size)(*width * *height * 4));
        NK_ASSERT(img_rgba);
        if (!img_rgba) goto failed;
        font_bake_convert(img_rgba, *width, *height, atlas->pixel);
        free(atlas->pixel);
        atlas->pixel = img_rgba;
    }
    atlas->tex_width = *width;
    atlas->tex_height = *height;

    /* initialize each font */
    for (int i = 0; i < atlas->font_count; ++i)
    {
        struct font* font = &atlas->fonts[i];
        struct font_config* config = &atlas->configs[i];
        font_init(font, &atlas->configs[i], atlas->glyphs);
    }

    /* free temporary memory */
    free(tmp);
    return atlas->pixel;

failed:
    /* error so cleanup all memory */
    if (tmp) free(tmp);
    if (atlas->glyphs) {
        free(atlas->glyphs);
        atlas->glyphs = 0;
    }
    if (atlas->pixel) {
        free(atlas->pixel);
        atlas->pixel = 0;
    }
    return 0;
}
NK_API void
font_atlas_end(struct font_atlas *atlas, nk_handle texture,
    struct nk_draw_null_texture *tex_null)
{
    NK_ASSERT(atlas);
    if (!atlas) {
        if (!tex_null) return;
        tex_null->texture = texture;
        tex_null->uv = nk_vec2(0.5f,0.5f);
    }
    if (tex_null) {
        tex_null->texture = texture;
        tex_null->uv.x = (atlas->custom.x + 0.5f)/(float)atlas->tex_width;
        tex_null->uv.y = (atlas->custom.y + 0.5f)/(float)atlas->tex_height;
    }

    for (int i = 0; i < atlas->font_count; ++i)
    {
        atlas->fonts[i].handle.texture = texture;
    }

    free(atlas->pixel);
    atlas->pixel = 0;
    atlas->tex_width = 0;
    atlas->tex_height = 0;
    atlas->custom.x = 0;
    atlas->custom.y = 0;
    atlas->custom.w = 0;
    atlas->custom.h = 0;
}
NK_API void
font_atlas_cleanup(struct font_atlas *atlas)
{
    NK_ASSERT(atlas);
    if (!atlas) return;
    if (atlas->configs) {
        for (int i = 0; i < atlas->font_count; ++i)
        {
            free(atlas->configs[i].ttf_blob);
            atlas->configs[i].ttf_blob = 0;
        }
    }
}
NK_API void
font_atlas_clear(struct font_atlas *atlas)
{
    NK_ASSERT(atlas);
    if (!atlas) return;

    if (atlas->configs) {

        for (int i = 0; i < atlas->font_count; ++i)
        {
            free(atlas->configs[i].ttf_blob);
            atlas->configs[i].ttf_blob = 0;
        }
        free(atlas->configs);
    }
    if (atlas->fonts) {
        free(atlas->fonts);
    }
    if (atlas->glyphs)
        free(atlas->glyphs);
    nk_zero_struct(*atlas);
}


