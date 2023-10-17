#ifndef WIDGETS_H
#define WIDGETS_H

#include "../common.h"

/* =============================================================================
 *
 *                                  WIDGET
 *
 * ============================================================================= */
typedef enum
{
    NK_WIDGET_INVALID,  /* The widget cannot be seen and is completely out of view */
    NK_WIDGET_VALID,    /* The widget is completely inside the window and can be updated and drawn */
    NK_WIDGET_ROM       /* The widget is partially visible and cannot be updated */
} nk_widget_layout_state;

typedef enum
{
    NK_WIDGET_STATE_MODIFIED    = NK_FLAG(1),
    NK_WIDGET_STATE_INACTIVE    = NK_FLAG(2), /* widget is neither active nor hovered */
    NK_WIDGET_STATE_ENTERED     = NK_FLAG(3), /* widget has been hovered on the current frame */
    NK_WIDGET_STATE_HOVER       = NK_FLAG(4), /* widget is being hovered */
    NK_WIDGET_STATE_ACTIVED     = NK_FLAG(5),/* widget is currently activated */
    NK_WIDGET_STATE_LEFT        = NK_FLAG(6), /* widget is from this frame on not hovered anymore */
    NK_WIDGET_STATE_HOVERED     = NK_WIDGET_STATE_HOVER|NK_WIDGET_STATE_MODIFIED, /* widget is being hovered */
    NK_WIDGET_STATE_ACTIVE      = NK_WIDGET_STATE_ACTIVED|NK_WIDGET_STATE_MODIFIED /* widget is currently activated */
} nk_widget_state;

NK_API nk_widget_layout_state nk_widget(struct nk_rect*, const struct nk_context*);
NK_API struct nk_input* nk_widget_input(struct nk_rect*, nk_widget_layout_state*, struct nk_context*);

NK_API struct nk_rect nk_widget_bounds(struct nk_context*);
NK_API float nk_widget_width(struct nk_context*);
NK_API float nk_widget_height(struct nk_context*);

NK_API void nk_spacing(struct nk_context*, int cols);

typedef enum
{
    NK_STYLE_ITEM_COLOR,
    NK_STYLE_ITEM_IMAGE
} nk_style_item_type;

typedef struct
{
    nk_style_item_type type;
    union
    {
        struct nk_color color;
        struct nk_image image;
    };
} nk_style_item;

NK_API void nk_label_image(struct nk_context*, struct nk_image, struct nk_color);
NK_API void nk_label_color(struct nk_context*, struct nk_color);

/* =============================================================================
 *
 *                                  TEXT
 *
 * ============================================================================= */
typedef enum
{
    NK_TEXT_ALIGN_LEFT      = 0x01,
    NK_TEXT_ALIGN_CENTER    = 0x02,
    NK_TEXT_ALIGN_RIGHT     = 0x04,
    NK_TEXT_ALIGN_TOP       = 0x08,
    NK_TEXT_ALIGN_MIDDLE    = 0x10,
    NK_TEXT_ALIGN_BOTTOM    = 0x20,
    NK_TEXT_ALIGN_WRAP      = 0x40
} nk_text_align;

typedef struct
{
    struct nk_color color;
    struct nk_vec2 padding;
    nk_flags alignment;
} nk_style_text;

#define NK_TEXT_LEFT      NK_TEXT_ALIGN_MIDDLE | NK_TEXT_ALIGN_LEFT
#define NK_TEXT_CENTERED  NK_TEXT_ALIGN_MIDDLE | NK_TEXT_ALIGN_CENTER
#define NK_TEXT_RIGHT     NK_TEXT_ALIGN_MIDDLE | NK_TEXT_ALIGN_RIGHT

NK_LIB void nk_widget_text(nk_command_buffer* out, struct nk_rect b, const char* string, int len, const nk_style_text* style, const struct nk_font* f);
NK_LIB void nk_widget_text_wrap(nk_command_buffer* out, struct nk_rect b, const char* string, int len, const nk_style_text* style, const struct nk_font* f);

NK_API void nk_text(struct nk_context* ctx, const char* text, int len, nk_flags align);
NK_API void nk_label(struct nk_context* ctx, const char* text, nk_flags align);

NK_API void nk_labelfv(struct nk_context* ctx, nk_flags align, const char* fmt, va_list);
NK_API void nk_labelf(struct nk_context* ctx, nk_flags align, const char* fmt, ...);

NK_API void nk_value_bool(struct nk_context*, const char* prefix, int);
NK_API void nk_value_int(struct nk_context*, const char* prefix, int);
NK_API void nk_value_uint(struct nk_context*, const char* prefix, unsigned int);
NK_API void nk_value_float(struct nk_context*, const char* prefix, float);
NK_API void nk_value_color_byte(struct nk_context*, const char* prefix, struct nk_color);
NK_API void nk_value_color_float(struct nk_context*, const char* prefix, struct nk_color);
NK_API void nk_value_color_hex(struct nk_context*, const char* prefix, struct nk_color);

/* =============================================================================
 *
 *                                  BUTTON
 *
 * ============================================================================= */

struct nk_style_button {
    /* background */
    nk_style_item normal;
    nk_style_item hover;
    nk_style_item active;

    /* text */
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_active;
    nk_flags text_alignment;

    /* properties */
    float rounding;
    float border;
    struct nk_color border_color;
    struct nk_vec2 padding;
};

typedef enum 
{
    NK_BUTTON_DEFAULT,
    NK_BUTTON_REPEATER
} nk_button_type;

NK_LIB nk_bool nk_button_behavior(nk_flags* state, struct nk_rect r, const struct nk_input* i, nk_button_type behavior);

NK_LIB void nk_draw_button_text(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const struct nk_style_button* style, const char* str, int len, nk_flags align, const struct nk_font* font);
NK_LIB void nk_draw_button_symbol(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const struct nk_style_button* style, nk_symbol symbol);
NK_LIB void nk_draw_button_text_symbol(nk_command_buffer* out, struct nk_rect bounds, nk_flags state, const struct nk_style_button* style, const char* str, int len, nk_symbol symbol, nk_flags align, const struct nk_font* font);

NK_LIB nk_bool nk_do_button_text(nk_flags* state, nk_command_buffer* out, struct nk_rect bounds, const char* string, int len, nk_flags align, nk_button_type behavior, const struct nk_style_button* style, const struct nk_input* in, const struct nk_font* font);
NK_LIB nk_bool nk_do_button_symbol(nk_flags* state, nk_command_buffer* out, struct nk_rect bounds, nk_symbol symbol, nk_button_type behavior, const struct nk_style_button* style, const struct nk_input* in);
NK_LIB nk_bool nk_do_button_text_symbol(nk_flags* state, nk_command_buffer* out, struct nk_rect bounds, nk_symbol symbol, const char* str, int len, nk_flags align, nk_button_type behavior, const struct nk_style_button* style, const struct nk_font* font, const struct nk_input* in);

NK_API nk_bool nk_button_text(struct nk_context *ctx, const char* title, int len);
NK_API nk_bool nk_button_label(struct nk_context *ctx, const char* title);
NK_API nk_bool nk_button_symbol(struct nk_context *ctx, nk_symbol sym);
NK_API nk_bool nk_button_symbol_text(struct nk_context* ctx, nk_symbol sym, const char*, int, nk_flags align);
NK_API nk_bool nk_button_symbol_label(struct nk_context *ctx, nk_symbol sym, const char*, nk_flags align);

NK_API nk_bool nk_button_push_behavior(struct nk_context*, nk_button_type);
NK_API nk_bool nk_button_pop_behavior(struct nk_context*);

/* =============================================================================
 *
 *                                  TOGGLE
 *
 * ============================================================================= */
struct nk_style_toggle {
    /* background */
    nk_style_item normal;
    nk_style_item hover;
    nk_style_item active;
    struct nk_color border_color;

    /* cursor */
    nk_style_item cursor_normal;
    nk_style_item cursor_hover;

    /* text */
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_active;
    nk_flags text_alignment;

    /* properties */
    struct nk_vec2 padding;
    float spacing;
    float border;
};

enum nk_toggle_type {
    NK_TOGGLE_CHECK,
    NK_TOGGLE_OPTION
};
NK_LIB nk_bool nk_toggle_behavior(const struct nk_input* in, struct nk_rect select, nk_flags* state, nk_bool active);
NK_LIB void nk_draw_toggle(nk_command_buffer* out, nk_flags state, const struct nk_style_toggle* style, nk_bool active, const struct nk_rect* label, const struct nk_rect* selector, const struct nk_rect* cursors, const char* string, int len, enum nk_toggle_type type, const struct nk_font* font);
NK_LIB nk_bool nk_do_toggle(nk_flags* state, nk_command_buffer* out, struct nk_rect r, nk_bool active, const char* str, int len, enum nk_toggle_type type, const struct nk_style_toggle* style, const struct nk_input* in, const struct nk_font* font);


NK_API nk_bool nk_checkbox_text(struct nk_context*, const char*, int, nk_bool active);
NK_API nk_bool nk_checkbox_label(struct nk_context*, const char*, nk_bool active);
NK_API nk_flags nk_checkbox_flags_text(struct nk_context*, const char*, int, nk_flags flags, nk_flags value);
NK_API nk_flags nk_checkbox_flags_label(struct nk_context*, const char*, nk_flags flags, nk_flags value);

NK_API int nk_radio_text(struct nk_context*, const char*, int, int option, int value);
NK_API int nk_radio_label(struct nk_context*, const char*, int option, int value);

/* =============================================================================
 *
 *                                  SELECTABLE
 *
 * ============================================================================= */
struct nk_style_selectable {
    /* background (inactive) */
    nk_style_item normal;
    nk_style_item hover;
    nk_style_item pressed;

    /* background (active) */
    nk_style_item normal_active;
    nk_style_item hover_active;
    nk_style_item pressed_active;

    /* text color (inactive) */
    struct nk_color text_normal;
    struct nk_color text_hover;
    struct nk_color text_pressed;

    /* text color (active) */
    struct nk_color text_normal_active;
    struct nk_color text_hover_active;
    struct nk_color text_pressed_active;
    nk_flags text_alignment;

    /* properties */
    float rounding;
    struct nk_vec2 padding;
    struct nk_vec2 touch_padding;
    struct nk_vec2 image_padding;
};

NK_LIB void nk_draw_selectable(nk_command_buffer* out, nk_flags state, const struct nk_style_selectable* style, nk_bool selected, const struct nk_rect bounds, const char* string, int len, nk_flags align, const struct nk_font* font);
NK_LIB nk_bool nk_do_selectable(nk_flags* state, nk_command_buffer* out, struct nk_rect bounds, const char* str, int len, nk_flags align, nk_bool selected, const struct nk_style_selectable* style, const struct nk_input* in, const struct nk_font* font);

NK_API nk_bool nk_selectable_text(struct nk_context*, const char*, int, nk_flags align, nk_bool selected);
NK_API nk_bool nk_selectable_label(struct nk_context*, const char*, nk_flags align, nk_bool selected);

/* =============================================================================
 *
 *                                  SLIDER
 *
 * ============================================================================= */

struct nk_style_slider {
    /* background */
    nk_style_item normal;
    nk_style_item hover;
    nk_style_item active;
    struct nk_color border_color;

    /* background bar */
    struct nk_color bar_normal;
    struct nk_color bar_hover;
    struct nk_color bar_active;
    struct nk_color bar_filled;

    /* cursor */
    nk_style_item cursor_normal;
    nk_style_item cursor_hover;
    nk_style_item cursor_active;

    /* properties */
    float border;
    float rounding;
    float bar_height;
    struct nk_vec2 padding;
    struct nk_vec2 spacing;
    struct nk_vec2 cursor_size;

    /* optional buttons */
    int show_buttons;
    struct nk_style_button inc_button;
    struct nk_style_button dec_button;
    nk_symbol inc_symbol;
    nk_symbol dec_symbol;
};

struct nk_style_progress {
    /* background */
    nk_style_item normal;
    nk_style_item hover;
    nk_style_item active;
    struct nk_color border_color;

    /* cursor */
    nk_style_item cursor_normal;
    nk_style_item cursor_hover;
    nk_style_item cursor_active;
    struct nk_color cursor_border_color;

    /* properties */
    float rounding;
    float border;
    float cursor_border;
    float cursor_rounding;
    struct nk_vec2 padding;
};


NK_LIB float nk_slider_behavior(nk_flags* state, struct nk_rect* visual_cursor, struct nk_input* in, struct nk_rect bounds, float min_value, float value, float max_value, float step);
NK_LIB void nk_draw_slider(struct nk_command_buffer* out, nk_flags state, const struct nk_style_slider* style, const struct nk_rect* bounds, const struct nk_rect* cursor);
NK_LIB float nk_do_slider(nk_flags* state, struct nk_command_buffer* out, struct nk_rect bounds, float min, float val, float max, float step, const struct nk_style_slider* style, struct nk_input* in, const struct nk_font* font);

NK_LIB nk_size nk_bar_slider_behavior(nk_flags* state, struct nk_input* in, struct nk_rect r, struct nk_rect cursor, nk_size max, nk_size value);
NK_LIB void nk_draw_bar_slider(struct nk_command_buffer* out, nk_flags state, const struct nk_style_progress* style, const struct nk_rect* bounds, const struct nk_rect* scursor);
NK_LIB nk_size nk_do_bar_slider(nk_flags* state, struct nk_command_buffer* out, struct nk_rect bounds, nk_size value, nk_size max, const struct nk_style_progress* style, struct nk_input* in);


NK_API float nk_slider(struct nk_context*, float min, float value, float max, float step);
NK_API int nk_slider_int(struct nk_context*, int min, int value, int max, int step);

NK_API nk_size nk_bar_slider(struct nk_context*, nk_size value, nk_size max);

NK_API nk_size nk_progress(struct nk_context*, nk_size value, nk_size max);

/* =============================================================================
 *
 *                                  POPUP
 *
 * ============================================================================= */
NK_API nk_bool nk_popup_begin(struct nk_context*, enum nk_popup_type, const char*, nk_flags, struct nk_rect bounds);
NK_API void nk_popup_close(struct nk_context*);
NK_API void nk_popup_end(struct nk_context*);
NK_API void nk_popup_get_scroll(struct nk_context*, nk_uint* offset_x, nk_uint* offset_y);
NK_API void nk_popup_set_scroll(struct nk_context*, nk_uint offset_x, nk_uint offset_y);
/* =============================================================================
 *
 *                                  CONTEXTUAL
 *
 * ============================================================================= */
NK_API nk_bool nk_contextual_begin(struct nk_context*, nk_flags, struct nk_vec2, struct nk_rect trigger_bounds);
NK_API void nk_contextual_close(struct nk_context*);
NK_API void nk_contextual_end(struct nk_context*);

NK_API nk_bool nk_contextual_item_text(struct nk_context*, const char*, int, nk_flags align);
NK_API nk_bool nk_contextual_item_label(struct nk_context*, const char*, nk_flags align);
NK_API nk_bool nk_contextual_item_symbol_text(struct nk_context*, nk_symbol, const char*, int, nk_flags align);
NK_API nk_bool nk_contextual_item_symbol_label(struct nk_context*, nk_symbol, const char*, nk_flags align);

/* =============================================================================
 *
 *                                  COMBOBOX
 *
 * ============================================================================= */
NK_API int nk_combo(struct nk_context*, const char** items, int count, int selected, int item_height, struct nk_vec2 size);
NK_API int nk_combo_separator(struct nk_context*, const char* items_separated_by_separator, int separator, int selected, int count, int item_height, struct nk_vec2 size);

NK_API nk_bool nk_combo_begin_text(struct nk_context*, const char* selected, int, struct nk_vec2 size);
NK_API nk_bool nk_combo_begin_label(struct nk_context*, const char* selected, struct nk_vec2 size);
NK_API nk_bool nk_combo_begin_color(struct nk_context*, struct nk_color color, struct nk_vec2 size);
NK_API void nk_combo_close(struct nk_context*);
NK_API void nk_combo_end(struct nk_context*);
/* =============================================================================
 *
 *                                  TOOLTIP
 *
 * ============================================================================= */
NK_API nk_bool nk_tooltip_begin(struct nk_context*, float width);
NK_API void nk_tooltip_end(struct nk_context*);

NK_API void nk_tooltip(struct nk_context*, const char*);
NK_API void nk_tooltipf(struct nk_context*, const char*, ...);
NK_API void nk_tooltipfv(struct nk_context*, const char*, va_list);

#endif // !WIDGETS_H
