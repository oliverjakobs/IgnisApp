#ifndef FONT_H
#define FONT_H

enum nk_font_coord_type {
    NK_COORD_UV, /* texture coordinates inside font glyphs are clamped between 0-1 */
    NK_COORD_PIXEL /* texture coordinates inside font glyphs are in absolute pixel */
};

struct nk_font;
struct nk_baked_font {
    float height;
    /* height of the font  */
    float ascent, descent;
    /* font glyphs ascent and descent  */
    nk_rune glyph_offset;
    /* glyph array offset inside the font glyph baking output array  */
    nk_rune glyph_count;
    /* number of glyphs of this font inside the glyph baking array output */
    const nk_rune* ranges;
    /* font codepoint ranges as pairs of (from/to) and 0 as last element */
};

struct nk_font_config {
    struct nk_font_config* next;
    /* NOTE: only used internally */
    void* ttf_blob;
    /* pointer to loaded TTF file memory block.
     * NOTE: not needed for nk_font_atlas_add_from_memory and nk_font_atlas_add_from_file. */
    nk_size ttf_size;
    /* size of the loaded TTF file memory block
     * NOTE: not needed for nk_font_atlas_add_from_memory and nk_font_atlas_add_from_file. */

    unsigned char ttf_data_owned_by_atlas;
    /* used inside font atlas: default to: 0*/
    unsigned char merge_mode;
    /* merges this font into the last font */
    unsigned char pixel_snap;
    /* align every character to pixel boundary (if true set oversample (1,1)) */
    unsigned char oversample_v, oversample_h;
    /* rasterize at high quality for sub-pixel position */
    unsigned char padding[3];

    float size;
    /* baked pixel height of the font */
    enum nk_font_coord_type coord_type;
    /* texture coordinate format with either pixel or UV coordinates */
    struct nk_vec2 spacing;
    /* extra pixel spacing between glyphs  */
    const nk_rune* range;
    /* list of unicode ranges (2 values per range, zero terminated) */
    struct nk_baked_font* font;
    /* font to setup in the baking process: NOTE: not needed for font atlas */
    nk_rune fallback_glyph;
    /* fallback glyph to use if a given rune is not found */
    struct nk_font_config* n;
    struct nk_font_config* p;
};

struct nk_font_glyph {
    nk_rune codepoint;
    float xadvance;
    float x0, y0, x1, y1, w, h;
    float u0, v0, u1, v1;
};

struct nk_font {
    struct nk_font* next;
    struct nk_user_font handle;
    struct nk_baked_font info;
    float scale;
    struct nk_font_glyph* glyphs;
    const struct nk_font_glyph* fallback;
    nk_rune fallback_codepoint;
    nk_handle texture;
    struct nk_font_config* config;
};

enum nk_font_atlas_format {
    NK_FONT_ATLAS_ALPHA8,
    NK_FONT_ATLAS_RGBA32
};

struct nk_font_atlas {
    void* pixel;
    int tex_width;
    int tex_height;

    struct nk_allocator permanent;
    struct nk_allocator temporary;

    struct nk_recti custom;
    struct nk_cursor cursors[NK_CURSOR_COUNT];

    int glyph_count;
    struct nk_font_glyph* glyphs;
    struct nk_font* default_font;
    struct nk_font* fonts;
    struct nk_font_config* config;
    int font_num;
};

/* some language glyph codepoint ranges */
const nk_rune* nk_font_default_glyph_ranges(void);
const nk_rune* nk_font_chinese_glyph_ranges(void);
const nk_rune* nk_font_cyrillic_glyph_ranges(void);
const nk_rune* nk_font_korean_glyph_ranges(void);

void nk_font_atlas_init_default(struct nk_font_atlas*);
void nk_font_atlas_init(struct nk_font_atlas*, struct nk_allocator*);
void nk_font_atlas_init_custom(struct nk_font_atlas*, struct nk_allocator* persistent, struct nk_allocator* transient);
void nk_font_atlas_begin(struct nk_font_atlas*);
struct nk_font_config nk_font_config(float pixel_height);
struct nk_font* nk_font_atlas_add(struct nk_font_atlas*, const struct nk_font_config*);
struct nk_font* nk_font_atlas_add_default(struct nk_font_atlas*, float height, const struct nk_font_config*);
struct nk_font* nk_font_atlas_add_from_memory(struct nk_font_atlas* atlas, void* memory, nk_size size, float height, const struct nk_font_config* config);
struct nk_font* nk_font_atlas_add_from_file(struct nk_font_atlas* atlas, const char* file_path, float height, const struct nk_font_config*);
struct nk_font* nk_font_atlas_add_compressed(struct nk_font_atlas*, void* memory, nk_size size, float height, const struct nk_font_config*);
struct nk_font* nk_font_atlas_add_compressed_base85(struct nk_font_atlas*, const char* data, float height, const struct nk_font_config* config);
const void* nk_font_atlas_bake(struct nk_font_atlas*, int* width, int* height, enum nk_font_atlas_format);
void nk_font_atlas_end(struct nk_font_atlas*, nk_handle tex, struct nk_draw_null_texture*);
const struct nk_font_glyph* nk_font_find_glyph(struct nk_font*, nk_rune unicode);
void nk_font_atlas_cleanup(struct nk_font_atlas* atlas);
void nk_font_atlas_clear(struct nk_font_atlas*);

#endif // !FONT_H
