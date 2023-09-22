#include "resources.h"

#include "toolbox/tb_str.h"
#include "toolbox/tb_mem.h"

static int ResourcesAllocEntry(void* allocator, tb_hashmap_entry* entry, const void* key, void* value)
{
    entry->key = tb_mem_dup(allocator, key, strlen(key) + 1);
    entry->val = tb_mem_dup(allocator, value, sizeof(IgnisTexture2D));

    return entry->key && entry->val;
}

static void ResourcesFreeEntry(void* allocator, tb_hashmap_entry* entry)
{
    ignisDeleteTexture2D(entry->val);
    tb_mem_constfree(allocator, entry->key);
    tb_mem_free(allocator, entry->val);
}

int ResourcesInit(Resources* res)
{
    res->textures.allocator = NULL;
    res->textures.alloc = NULL;
    res->textures.free = NULL;
    res->textures.entry_alloc = ResourcesAllocEntry;
    res->textures.entry_free = ResourcesFreeEntry;

    return tb_hashmap_init(&res->textures, tb_hash_string, strcmp, 0) == TB_HASHMAP_OK;
}

void ResourcesDestroy(Resources* res) { tb_hashmap_destroy(&res->textures); }
void ResourcesClear(Resources* res) { tb_hashmap_clear(&res->textures); }

const IgnisTexture2D* ResourcesLoadTexture2D(Resources* res, const char* path)
{
    IgnisTexture2D* entry = tb_hashmap_find(&res->textures, path);

    IgnisTexture2D new_entry = {0};
    if (!entry && ignisLoadTexture2D(&new_entry, path, NULL))
        entry = tb_hashmap_insert(&res->textures, path, &new_entry);

    return entry;
}

const IgnisTexture2D* ResourcesReloadTexture2D(Resources* res, const IgnisTexture2D* texture, const char* path)
{
    IgnisTexture2D* entry = tb_hashmap_find(&res->textures, path);
    if (!entry || entry != texture) return texture;

    ignisLoadTexture2D(entry, path, NULL);

    return entry;
}

const char* ResourcesGetTexture2DPath(const Resources* res, const IgnisTexture2D* texture)
{
    for (tb_hashmap_iter* iter = tb_hashmap_iterator(&res->textures); iter; iter = tb_hashmap_iter_next(&res->textures, iter))
    {
        if (texture == tb_hashmap_iter_get_val(iter))
            return tb_hashmap_iter_get_key(iter);
    }

    return NULL;
}