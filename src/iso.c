#include "iso.h"

void isoMapInit(IsoMap* map, uint32_t* grid, uint32_t width, uint32_t height, float tile_size, float tile_offset)
{
    uint32_t size = width * height;
    map->grid = malloc(size * sizeof(uint32_t));
    if (!map->grid) return;

    memcpy(map->grid, grid, size * sizeof(uint32_t));

    map->width = width;
    map->height = height;
    map->tile_size = tile_size;
    map->tile_offset = tile_offset;
}

void isoMapDestroy(IsoMap* map)
{
    if (map->grid) free(map->grid);
}

vec2 isoMapGetCenter(const IsoMap* map)
{
    vec2 center = {
        map->width * .5f * map->tile_size,
        map->height * .5f * map->tile_size,
    };
    return center;
}

vec2 screenToWorld(vec2 s_pos)
{
    vec2 w_pos = {
        (2 * s_pos.y + s_pos.x) * .5f,
        (2 * s_pos.y - s_pos.x) * .5f
    };
    return w_pos;
}

vec2 worldToScreen(vec2 w_pos)
{
    vec2 s_pos = {
        w_pos.x - w_pos.y,
        (w_pos.x + w_pos.y) * .5f
    };
    return s_pos;
}

uint32_t tileClip(const IsoMap* map, float f) { return (uint32_t)floorf(f / map->tile_size); }

vec2 getTilePos(const IsoMap* map, uint32_t col, uint32_t row)
{
    return (vec2) { col* map->tile_size, row* map->tile_size };
}

uint32_t getTileIndexAt(const IsoMap* map, vec2 w_pos)
{
    uint32_t col = tileClip(map, w_pos.x);
    uint32_t row = tileClip(map, w_pos.y);

    if (col >= map->width || row >= map->height) return UINT32_MAX;

    return row * map->width + col;
}