#ifndef POPUP_H
#define POPUP_H

#include "../common.h"

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
NK_API nk_bool nk_combo_begin_color(struct nk_context*, nk_color color, struct nk_vec2 size);
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

#endif