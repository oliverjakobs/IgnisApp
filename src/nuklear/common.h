#ifndef NUKLEAR_COMMON_H
#define NUKLEAR_COMMON_H


/*
 * ==============================================================
 *
 *                          CONSTANTS
 *
 * ===============================================================
 */
#define NK_UNDEFINED (-1.0f)
#define NK_UTF_INVALID 0xFFFD /* internal invalid utf8 rune */
#define NK_UTF_SIZE 4 /* describes the number of bytes a glyph consists of*/
#ifndef NK_INPUT_MAX
#define NK_INPUT_MAX 16
#endif
#ifndef NK_MAX_NUMBER_BUFFER
#define NK_MAX_NUMBER_BUFFER 64
#endif
#ifndef NK_SCROLLBAR_HIDING_TIMEOUT
#define NK_SCROLLBAR_HIDING_TIMEOUT 4.0f
#endif
 /*
  * ==============================================================
  *
  *                          HELPER
  *
  * ===============================================================
  */
#ifndef NK_API
#ifdef NK_PRIVATE
#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199409L))
#define NK_API static inline
#elif defined(__cplusplus)
#define NK_API static inline
#else
#define NK_API static
#endif
#else
#define NK_API extern
#endif
#endif
#ifndef NK_LIB
#ifdef NK_SINGLE_FILE
#define NK_LIB static
#else
#define NK_LIB extern
#endif
#endif

#define NK_INTERN static
#define NK_STORAGE static
#define NK_GLOBAL static

#define NK_FLAG(x) (1 << (x))
#define NK_STRINGIFY(x) #x
#define NK_MACRO_STRINGIFY(x) NK_STRINGIFY(x)
#define NK_STRING_JOIN_IMMEDIATE(arg1, arg2) arg1 ## arg2
#define NK_STRING_JOIN_DELAY(arg1, arg2) NK_STRING_JOIN_IMMEDIATE(arg1, arg2)
#define NK_STRING_JOIN(arg1, arg2) NK_STRING_JOIN_DELAY(arg1, arg2)

#ifdef _MSC_VER
#define NK_UNIQUE_NAME(name) NK_STRING_JOIN(name,__COUNTER__)
#else
#define NK_UNIQUE_NAME(name) NK_STRING_JOIN(name,__LINE__)
#endif

#ifndef NK_STATIC_ASSERT
#define NK_STATIC_ASSERT(exp) typedef char NK_UNIQUE_NAME(_dummy_array)[(exp)?1:-1]
#endif

#ifndef NK_FILE_LINE
#ifdef _MSC_VER
#define NK_FILE_LINE __FILE__ ":" NK_MACRO_STRINGIFY(__COUNTER__)
#else
#define NK_FILE_LINE __FILE__ ":" NK_MACRO_STRINGIFY(__LINE__)
#endif
#endif

#define NK_MIN(a,b) ((a) < (b) ? (a) : (b))
#define NK_MAX(a,b) ((a) < (b) ? (b) : (a))
#define NK_CLAMP(i,v,x) (NK_MAX(NK_MIN(v,x), i))

#include <stdarg.h>
#if defined(_MSC_VER) && (_MSC_VER >= 1600) /* VS 2010 and above */
#include <sal.h>
#define NK_PRINTF_FORMAT_STRING _Printf_format_string_
#else
#define NK_PRINTF_FORMAT_STRING
#endif
#if defined(__GNUC__)
#define NK_PRINTF_VARARG_FUNC(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, fmtargnumber+1)))
#define NK_PRINTF_VALIST_FUNC(fmtargnumber) __attribute__((format(__printf__, fmtargnumber, 0)))
#else
#define NK_PRINTF_VARARG_FUNC(fmtargnumber)
#define NK_PRINTF_VALIST_FUNC(fmtargnumber)
#endif

  /*
   * ===============================================================
   *
   *                          BASIC
   *
   * ===============================================================
   */
#include <stdint.h>
#define NK_INT8 int8_t
#define NK_UINT8 uint8_t
#define NK_INT16 int16_t
#define NK_UINT16 uint16_t
#define NK_INT32 int32_t
#define NK_UINT32 uint32_t
#define NK_SIZE_TYPE uintptr_t
#define NK_POINTER_TYPE uintptr_t

#ifndef NK_BOOL
#ifdef NK_INCLUDE_STANDARD_BOOL
#include <stdbool.h>
#define NK_BOOL bool
#else
#define NK_BOOL unsigned int /* could be char, use int for drop-in replacement backwards compatibility */
#endif
#endif

typedef NK_INT8 nk_char;
typedef NK_UINT8 nk_uchar;
typedef NK_UINT8 nk_byte;
typedef NK_INT16 nk_short;
typedef NK_UINT16 nk_ushort;
typedef NK_INT32 nk_int;
typedef NK_UINT32 nk_uint;
typedef NK_SIZE_TYPE nk_size;
typedef NK_POINTER_TYPE nk_ptr;
typedef NK_BOOL nk_bool;

typedef nk_uint nk_hash;
typedef nk_uint nk_flags;
typedef nk_uint nk_rune;

/* Make sure correct type size:
 * This will fire with a negative subscript error if the type sizes
 * are set incorrectly by the compiler, and compile out if not */
NK_STATIC_ASSERT(sizeof(nk_short) == 2);
NK_STATIC_ASSERT(sizeof(nk_ushort) == 2);
NK_STATIC_ASSERT(sizeof(nk_uint) == 4);
NK_STATIC_ASSERT(sizeof(nk_int) == 4);
NK_STATIC_ASSERT(sizeof(nk_byte) == 1);
NK_STATIC_ASSERT(sizeof(nk_flags) >= 4);
NK_STATIC_ASSERT(sizeof(nk_rune) >= 4);
NK_STATIC_ASSERT(sizeof(nk_size) >= sizeof(void*));
NK_STATIC_ASSERT(sizeof(nk_ptr) >= sizeof(void*));
#ifdef NK_INCLUDE_STANDARD_BOOL
NK_STATIC_ASSERT(sizeof(nk_bool) == sizeof(bool));
#else
NK_STATIC_ASSERT(sizeof(nk_bool) >= 2);
#endif

/* ============================================================================
 *
 *                                  API
 *
 * =========================================================================== */
struct nk_buffer;
struct nk_allocator;
typedef struct nk_command_buffer nk_command_buffer;
struct nk_draw_command;
struct nk_convert_config;
struct nk_text_edit;
struct nk_draw_list;
typedef struct nk_font nk_font;
struct nk_panel;
struct nk_context;
struct nk_draw_vertex_layout_element;

enum { nk_false, nk_true };

typedef struct
{ 
    nk_byte r, g, b, a;
} nk_color;

typedef struct 
{
    float r, g, b, a;
} nk_colorf;

struct nk_vec2 { float x, y; };
struct nk_vec2i { short x, y; };
struct nk_rect { float x, y, w, h; };
struct nk_recti { short x, y, w, h; };

typedef char nk_glyph[NK_UTF_SIZE];
typedef union { void* ptr; int id; } nk_handle;

typedef struct
{
    nk_handle handle;
    nk_ushort w, h;
} nk_image;

struct nk_cursor { nk_image img; struct nk_vec2 size, offset; };
struct nk_scroll { nk_uint x, y; };

enum nk_heading { NK_UP, NK_RIGHT, NK_DOWN, NK_LEFT };
enum nk_orientation { NK_VERTICAL, NK_HORIZONTAL };
enum nk_collapse_states { NK_MINIMIZED = nk_false, NK_MAXIMIZED = nk_true };
enum nk_show_states { NK_HIDDEN = nk_false, NK_SHOWN = nk_true };

enum nk_chart_type { NK_CHART_LINES, NK_CHART_COLUMN, NK_CHART_MAX };
enum nk_chart_event { NK_CHART_HOVERING = 0x01, NK_CHART_CLICKED = 0x02 };

enum nk_color_format { NK_RGB, NK_RGBA };
enum nk_popup_type { NK_POPUP_STATIC, NK_POPUP_DYNAMIC };
enum nk_layout_format { NK_DYNAMIC, NK_STATIC };
enum nk_tree_type { NK_TREE_NODE, NK_TREE_TAB };

typedef void* (*nk_plugin_alloc)(nk_handle, void* old, nk_size);
typedef void (*nk_plugin_free)(nk_handle, void* old);
typedef nk_bool(*nk_plugin_filter)(const struct nk_text_edit*, nk_rune unicode);
typedef void(*nk_plugin_paste)(nk_handle, struct nk_text_edit*);
typedef void(*nk_plugin_copy)(nk_handle, const char*, int len);

struct nk_allocator {
    nk_handle userdata;
    nk_plugin_alloc alloc;
    nk_plugin_free free;
};

typedef enum
{
    NK_SYMBOL_NONE,
    NK_SYMBOL_CIRCLE_SOLID,
    NK_SYMBOL_CIRCLE_OUTLINE,
    NK_SYMBOL_RECT_SOLID,
    NK_SYMBOL_RECT_OUTLINE,
    NK_SYMBOL_TRIANGLE_UP,
    NK_SYMBOL_TRIANGLE_DOWN,
    NK_SYMBOL_TRIANGLE_LEFT,
    NK_SYMBOL_TRIANGLE_RIGHT,
    NK_SYMBOL_CHECK,
    NK_SYMBOL_MAX
} nk_symbol;

#endif // !NUKLEAR_COMMON_H
