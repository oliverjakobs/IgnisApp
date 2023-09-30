#include "font.h"
#include "nuklear/nuklear_internal.h"


/* -------------------------------------------------------------
 *
 *                          RECT PACK
 *
 * --------------------------------------------------------------*/

#define STB_RECT_PACK_IMPLEMENTATION
#include "nuklear/stb_rect_pack.h"


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "nuklear/stb_image_write.h"

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
 *                          Ranges
 *
 * --------------------------------------------------------------*/

NK_INTERN int nk_range_count(const rune* range)
{
    const rune* iter = range;
    NK_ASSERT(range);
    if (!range) return 0;
    while (*(iter++) != 0);
    return (iter == range) ? 0 : (int)((iter - range) / 2);
}

NK_INTERN int nk_range_glyph_count(const rune* range, int count)
{
    int total_glyphs = 0;
    for (int i = 0; i < count; ++i)
    {
        rune f = range[(i * 2) + 0];
        rune t = range[(i * 2) + 1];
        NK_ASSERT(t >= f);
        total_glyphs += (int)((t - f) + 1);
    }
    return total_glyphs;
}

NK_API const rune* font_default_glyph_ranges(void)
{
    NK_STORAGE const rune ranges[] = { 0x0020, 0x00FF, 0 };
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
    int *width, int *height,
    const struct font_config *config_list, int count)
{
    NK_STORAGE const nk_size max_height = 1024 * 32;

    NK_ASSERT(width);
    NK_ASSERT(height);
    NK_ASSERT(config_list);
    NK_ASSERT(count);

    if (!width || !height || !config_list || !count) return nk_false;

    int total_glyph_count = 0;
    int total_range_count = 0;
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
        struct font_config* cfg = &config_list[i];
        struct stbtt_fontinfo* font_info = &baker->build[i].info;

        if (!stbtt_InitFont(font_info, cfg->ttf_blob, stbtt_GetFontOffsetForIndex(cfg->ttf_blob, 0)))
            return nk_false;
    }

    *height = 0;
    *width = (total_glyph_count > 1000) ? 1024 : 512;
    stbtt_PackBegin(&baker->spc, 0, (int)*width, (int)max_height, 0, 1, NULL);
    {
        int range_n = 0;
        int rect_n = 0;
        int char_n = 0;

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
    return nk_true;
}

static void* font_bake(struct font_baker *baker, int width, int height, struct font_glyph *glyphs, int glyphs_count, const struct font_config *config_list, int font_count)
{
    NK_ASSERT(width);
    NK_ASSERT(height);
    NK_ASSERT(config_list);
    NK_ASSERT(baker);
    NK_ASSERT(font_count);
    NK_ASSERT(glyphs_count);
    if (!width || !height || !config_list || !font_count || !glyphs || !glyphs_count)
        return NULL;

    void* pixels = malloc((size_t)width * (size_t)height);
    NK_ASSERT(pixels);
    if (!pixels) return NULL;

    nk_zero(pixels, (nk_size)((nk_size)width * (nk_size)height));

    /* second font pass: render glyphs */
    baker->spc.pixels = (unsigned char*)pixels;
    baker->spc.height = (int)height;

    for (int i = 0; i < font_count; ++i)
    {
        struct font_config* iter = &config_list[i];
        struct font_bake_data* tmp = &baker->build[i];
        stbtt_PackSetOversampling(&baker->spc, iter->oversample_h, iter->oversample_v);
        stbtt_PackFontRangesRenderIntoRects(&baker->spc, &tmp->info, tmp->ranges, tmp->range_count, tmp->rects);
    }
    stbtt_PackEnd(&baker->spc);

    /* third pass: setup font and glyphs */
    rune glyph_offset = 0;
    for (int i = 0; i < font_count; ++i)
    {
        struct font_config* config = &config_list[i];
        config->glyph_offset = glyph_offset;

        struct font_bake_data* tmp = &baker->build[i];
        float font_scale = stbtt_ScaleForPixelHeight(&tmp->info, config->size);

        int unscaled_ascent, unscaled_descent, unscaled_line_gap;
        stbtt_GetFontVMetrics(&tmp->info, &unscaled_ascent, &unscaled_descent, &unscaled_line_gap);

        float ascent = ((float)unscaled_ascent * font_scale);

        /* fill own baked font glyph array */
        rune glyph_count = 0;
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
                stbtt_GetPackedQuad(range->chardata_for_range, width, height, char_idx, &dummy_x, &dummy_y, &q, 0);

                /* fill own glyph type with data */
                glyph = &glyphs[glyph_offset + glyph_count];
                glyph->codepoint = codepoint;
                glyph->x0 = q.x0;
                glyph->y0 = q.y0 + (ascent + 0.5f);
                glyph->x1 = q.x1;
                glyph->y1 = q.y1 + (ascent + 0.5f);
                glyph->w = glyph->x1 - glyph->x0 + 0.5f;
                glyph->h = glyph->y1 - glyph->y0;

                if (config->coord_type == IGNIS_COORD_PIXEL) {
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
                glyph->xadvance = pc->xadvance;
                if (config->pixel_snap)
                    glyph->xadvance = (float)(int)(glyph->xadvance + 0.5f);
                glyph_count++;
            }
        }
        glyph_offset += glyph_count;
    }

    return pixels;
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
float font_text_width(nk_handle handle, float height, const char *text, int len)
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

void
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
    return font->fallback;
}

/* -------------------------------------------------------------
 *
 *                          FONT ATLAS
 *
 * --------------------------------------------------------------*/
struct font_config font_default_config(float pixel_height)
{
    struct font_config cfg;
    nk_zero_struct(cfg);
    cfg.ttf_blob = NULL;
    cfg.ttf_size = 0;
    cfg.size = pixel_height;
    cfg.oversample_h = 3;
    cfg.oversample_v = 1;
    cfg.pixel_snap = 0;
    cfg.coord_type = IGNIS_COORD_UV;
    cfg.range = font_default_glyph_ranges();
    cfg.fallback_glyph = '?';
    return cfg;
}

uint8_t ignisFontAtlasLoadFromFile(struct font_config* config, const char* path, float height)
{
    nk_size size;
    char* memory = ignisReadFile(path, &size);
    if (!memory) return IGNIS_FAILURE;

    struct font_config cfg = font_default_config(height);
    cfg.ttf_blob = memory;
    cfg.ttf_size = size;

    nk_memcopy(config, &cfg, sizeof(struct font_config));
    return IGNIS_SUCCESS;
}


NK_API const void*
font_atlas_bake(struct font_atlas *atlas, const struct font_config *configs, int font_count, enum font_atlas_format fmt)
{
    void *tmp = 0;
    nk_size tmp_size;
    struct font_baker *baker;

    NK_ASSERT(atlas);
    NK_ASSERT(configs);
    NK_ASSERT(font_count);
    if (!atlas || !configs || !font_count) return 0;

    /* allocate temporary baker memory required for the baking process */
    font_baker_memory(&tmp_size, &atlas->glyph_count, configs, font_count);
    tmp = malloc(tmp_size);
    NK_ASSERT(tmp);
    if (!tmp) goto failed;
    nk_memset(tmp,0,tmp_size);

    /* allocate glyph memory for all fonts */
    baker = font_baker(tmp, atlas->glyph_count, font_count);
    atlas->glyphs = malloc(sizeof(struct font_glyph)*(nk_size)atlas->glyph_count);
    NK_ASSERT(atlas->glyphs);
    if (!atlas->glyphs)
        goto failed;

    /* pack all glyphs into a tight fit space */
    int width, height;
    if (!font_bake_pack(baker, &width, &height, configs, font_count))
        goto failed;

    /* bake glyphs */
    void* pixels = font_bake(baker, width, height, atlas->glyphs, atlas->glyph_count, configs, font_count);

    NK_ASSERT(pixels);
    if (!pixels)
        goto failed;

    if (fmt == NK_FONT_ATLAS_RGBA32) {
        /* convert alpha8 image into rgba32 image */
        void *img_rgba = malloc((size_t)width * (size_t)height * 4);
        NK_ASSERT(img_rgba);
        if (!img_rgba) goto failed;
        font_bake_convert(img_rgba, width, height, pixels);
        free(pixels);
        pixels = img_rgba;
    }

    // init atlas

    /* create texture */
    ignisGenerateTexture2D(&atlas->texture, width, height, pixels, NULL);

    /* initialize each font */
    atlas->fonts = malloc(sizeof(struct font) * font_count);
    atlas->font_count = font_count;
    for (int i = 0; i < font_count; ++i)
    {
        struct font* font = &atlas->fonts[i];
        struct font_config* config = &configs[i];

        font->texture = &atlas->texture;
        font->size = config->size;
        font->range = config->range;
        font->glyphs = &atlas->glyphs[config->glyph_offset];
        font->fallback = font_find_glyph(font, config->fallback_glyph);
    }

    // stbi_write_png("font.png", width, height, 4, pixel, 4 * width);


    /* free temporary memory */
    free(tmp);
    free(pixels);
    return NULL;

failed:
    /* error so cleanup all memory */
    if (tmp) free(tmp);
    if (atlas->glyphs) {
        free(atlas->glyphs);
        atlas->glyphs = 0;
    }
    if (pixels) {
        free(pixels);
        pixels = 0;
    }
    return 0;
}

NK_API void
font_atlas_cleanup(struct font_atlas *atlas)
{
    NK_ASSERT(atlas);
    if (!atlas) return;
}
NK_API void
font_atlas_clear(struct font_atlas *atlas)
{
    NK_ASSERT(atlas);
    if (!atlas) return;

    if (atlas->fonts) {
        free(atlas->fonts);
    }
    if (atlas->glyphs)
        free(atlas->glyphs);
    nk_zero_struct(*atlas);
}


