#ifndef FONT_H
#define FONT_H

#include "ignis/ignis.h"
#include "nuklear/nuklear.h"

typedef uint32_t rune;

enum font_coord_type {
    IGNIS_COORD_UV, /* texture coordinates inside font glyphs are clamped between 0-1 */
    IGNIS_COORD_PIXEL /* texture coordinates inside font glyphs are in absolute pixel */
};

struct baked_font {
    float height;
    /* height of the font  */
    float ascent, descent;
    /* font glyphs ascent and descent  */
    rune glyph_offset;
    /* glyph array offset inside the font glyph baking output array  */
    rune glyph_count;
    /* number of glyphs of this font inside the glyph baking array output */
    const rune* ranges;
    /* font codepoint ranges as pairs of (from/to) and 0 as last element */
};

struct font_config {
    /* NOTE: only used internally */
    void* ttf_blob;
    /* pointer to loaded TTF file memory block.
     * NOTE: not needed for font_atlas_add_from_memory and font_atlas_add_from_file. */
    nk_size ttf_size;
    /* size of the loaded TTF file memory block
     * NOTE: not needed for font_atlas_add_from_memory and font_atlas_add_from_file. */

    unsigned char ttf_data_owned_by_atlas;
    /* used inside font atlas: default to: 0*/
    unsigned char pixel_snap;
    /* align every character to pixel boundary (if true set oversample (1,1)) */
    unsigned char oversample_v, oversample_h;
    /* rasterize at high quality for sub-pixel position */
    unsigned char padding[3];

    float size;
    /* baked pixel height of the font */
    enum font_coord_type coord_type;
    /* texture coordinate format with either pixel or UV coordinates */
    struct nk_vec2 spacing;
    /* extra pixel spacing between glyphs  */
    const rune* range;
    /* list of unicode ranges (2 values per range, zero terminated) */
    struct baked_font font;
    /* font to setup in the baking process: NOTE: not needed for font atlas */
    rune fallback_glyph;
    /* fallback glyph to use if a given rune is not found */
};

struct font_glyph {
    rune codepoint;
    float xadvance;
    float x0, y0, x1, y1, w, h;
    float u0, v0, u1, v1;
};

struct font {
    struct nk_user_font handle;

    float size;

    struct font_glyph* glyphs;
    const struct font_glyph* fallback; /* fallback glyph to use if a given rune is not found */

    const rune* range; /* list of unicode ranges (2 values per range, zero terminated) */
};

enum font_atlas_format {
    NK_FONT_ATLAS_ALPHA8,
    NK_FONT_ATLAS_RGBA32
};

struct font_atlas {
    void* pixel;
    int tex_width;
    int tex_height;

    struct nk_recti custom;

    struct font_glyph* glyphs;
    int glyph_count;

    struct font* fonts;
    struct font_config* configs;
    int font_count;
    int font_cap;
};




/* some language glyph codepoint ranges */
const rune* font_default_glyph_ranges(void);
const rune* font_chinese_glyph_ranges(void);
const rune* font_cyrillic_glyph_ranges(void);
const rune* font_korean_glyph_ranges(void);

void font_atlas_init(struct font_atlas*);
void font_atlas_begin(struct font_atlas*, int count);

struct font_config font_default_config(float pixel_height);

struct font* font_atlas_add(struct font_atlas*, const struct font_config*);
struct font* font_atlas_add_default(struct font_atlas*, float height, const struct font_config*);
struct font* font_atlas_add_from_memory(struct font_atlas* atlas, void* memory, nk_size size, float height, const struct font_config* config);
struct font* font_atlas_add_from_file(struct font_atlas* atlas, const char* file_path, float height, const struct font_config*);
struct font* font_atlas_add_compressed(struct font_atlas*, void* memory, nk_size size, float height, const struct font_config*);
struct font* font_atlas_add_compressed_base85(struct font_atlas*, const char* data, float height, const struct font_config* config);

const void* font_atlas_bake(struct font_atlas* atlas, int* width, int* height, enum font_atlas_format fmt);
void font_atlas_end(struct font_atlas*, nk_handle tex, struct nk_draw_null_texture*);
const struct font_glyph* font_find_glyph(struct font*, rune unicode);
void font_atlas_cleanup(struct font_atlas* atlas);
void font_atlas_clear(struct font_atlas*);



typedef struct
{
    rune codepoint;
    float xadvance;
    float x0, y0, x1, y1, w, h;
    float u0, v0, u1, v1;
} IgnisGlyph;

typedef struct
{
    float size;

    IgnisGlyph* glyphs;
    const IgnisGlyph* fallback; /* fallback glyph to use if a given rune is not found */

    const rune* range; /* list of unicode ranges (2 values per range, zero terminated) */
} IgnisFont;

typedef struct
{
    IgnisFont* fonts;
    int font_count;

    IgnisGlyph* glyphs;
    int glyph_count;

    IgnisTexture2D texture;
} IgnisFontAtlas;

#endif // !FONT_H
