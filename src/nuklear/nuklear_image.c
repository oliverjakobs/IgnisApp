#include "nuklear.h"
#include "nuklear_internal.h"

/* ===============================================================
 *
 *                          IMAGE
 *
 * ===============================================================*/
NK_API nk_handle nk_handle_ptr(void *ptr)
{
    nk_handle handle = {0};
    handle.ptr = ptr;
    return handle;
}

NK_API nk_handle nk_handle_id(int id)
{
    nk_handle handle = {0};
    handle.id = id;
    return handle;
}

NK_API struct nk_image nk_image_handle(nk_handle handle)
{
    struct nk_image s = {
        .handle = handle,
        .w = 0,
        .h = 0
    };
    return s;
}

NK_API struct nk_image nk_image_ptr(void *ptr)
{
    NK_ASSERT(ptr);
    struct nk_image s = {
        .handle.ptr = ptr,
        .w = 0,
        .h = 0
    };
    return s;
}

NK_API struct nk_image nk_image_id(int id)
{
    struct nk_image s = {
        .handle.id = id,
        .w = 0,
        .h = 0
    };
    return s;
}

