#ifndef FONT_H
#define FONT_H

#include "ignis/ignis.h"
#include "nuklear/nuklear.h"

typedef uint32_t rune;

enum font_coord_type {
    IGNIS_COORD_UV, /* texture coordinates inside font glyphs are clamped between 0-1 */
    IGNIS_COORD_PIXEL /* texture coordinates inside font glyphs are in absolute pixel */
};

struct font_config {
    void* ttf_blob;     /* pointer to loaded TTF file memory block. */
    nk_size ttf_size;   /* size of the loaded TTF file memory block */

    unsigned char pixel_snap; /* align every character to pixel boundary (if true set oversample (1,1)) */
    unsigned char oversample_v, oversample_h; /* rasterize at high quality for sub-pixel position */

    enum font_coord_type coord_type; /* texture coordinate format with either pixel or UV coordinates */

    float size; /* pixel height of the font */

    uint32_t glyph_offset;  /* glyph array offset inside the font glyph baking output array  */
    const rune* range;      /* list of unicode ranges (2 values per range, zero terminated) */
    rune fallback_glyph;    /* fallback glyph to use if a given rune is not found */
};

typedef struct
{
    float size; /* pixel height of the font */

    unsigned char pixel_snap; /* align every character to pixel boundary (if true set oversample (1,1)) */
    unsigned char oversample_v, oversample_h; /* rasterize at high quality for sub-pixel position */

    enum font_coord_type coord_type; /* texture coordinate format with either pixel or UV coordinates */

    const rune* range;   /* list of unicode ranges (2 values per range, zero terminated) */
    rune fallback_glyph; /* fallback glyph to use if a given rune is not found */
} IgnisFontConfig;

struct font_glyph {
    rune codepoint;
    float xadvance;
    float x0, y0, x1, y1, w, h;
    float u0, v0, u1, v1;
};

struct font {    
    const IgnisTexture2D* texture;
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
    struct font_glyph* glyphs;
    int glyph_count;

    struct font* fonts;
    int font_count;

    IgnisTexture2D texture;
};

void font_query_font_glyph(nk_handle handle, float height, struct nk_user_font_glyph* glyph, rune codepoint, rune next);
float font_text_width(nk_handle handle, float height, const char* text, int len);

/* some language glyph codepoint ranges */
const rune* font_default_glyph_ranges(void);
const rune* font_chinese_glyph_ranges(void);
const rune* font_cyrillic_glyph_ranges(void);
const rune* font_korean_glyph_ranges(void);

struct font_config font_default_config(float pixel_height);

uint8_t font_atlas_add_default(struct font_config* config, float height);
uint8_t font_atlas_add_compressed(struct font_config* config, void* memory, nk_size size, float height);
uint8_t font_atlas_add_compressed_base85(struct font_config* config, const char* data, float height);

const void* font_atlas_bake(struct font_atlas* atlas, const struct font_config* configs, int font_count, enum font_atlas_format fmt);
const struct font_glyph* font_find_glyph(struct font*, rune unicode);
void font_atlas_cleanup(struct font_atlas* atlas);
void font_atlas_clear(struct font_atlas*);



uint8_t ignisFontAtlasLoadFromFile(struct font_config* config, const char* path, float height);





typedef struct
{
    rune codepoint;
    float xadvance;
    float x0, y0, x1, y1, w, h;
    float u0, v0, u1, v1;
} IgnisGlyph;

typedef struct
{
    const IgnisTexture2D* texture;
    float size;

    IgnisGlyph* glyphs;
    const IgnisGlyph* fallback; /* fallback glyph to use if a given rune is not found */

    const rune* range; /* list of unicode ranges (2 values per range, zero terminated) */
} IgnisFont;

const IgnisGlyph ignisFontFindGlyph(const IgnisFont* font, rune unicode);

typedef struct
{
    IgnisFont* fonts;
    int font_count;

    IgnisGlyph* glyphs;
    int glyph_count;

    IgnisTexture2D texture;
} IgnisFontAtlas;

#endif // !FONT_H
