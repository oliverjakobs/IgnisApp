#include "iso.h"

void renderMap(const IsoMap* map, const IgnisTextureAtlas2D* texture_atlas)
{
    for (uint32_t i = 0; i < map->width * map->height; i++)
    {
        uint32_t col = i % map->width;
        uint32_t row = i / map->width;

        vec2 pos = worldToScreen(getTilePos(map, col, row));
        IgnisRect rect = {
            pos.x - map->tile_size,
            pos.y,
            map->tile_size * 2.0f,
            map->tile_size + map->tile_offset
        };
        uint32_t frame = map->grid[i];
        IgnisRect src = ignisGetTextureAtlas2DSrcRect(texture_atlas, frame);
        ignisBatch2DRenderTextureSrc((IgnisTexture2D*)texture_atlas, rect, src);
    }
}

// hightlight isometric version / screen
void highlightTile(const IsoMap* map, uint32_t index)
{
    uint32_t col = index % map->width;
    uint32_t row = index / map->width;

    if (col >= map->width || row >= map->height) return;

    const float vertices[] = {
         0.0f,           0.0f,
        -map->tile_size, map->tile_size * .5f,
         0.0f,           map->tile_size,
         map->tile_size, map->tile_size * .5f
    };

    vec2 pos = worldToScreen(getTilePos(map, col, row));
    ignisPrimitives2DRenderPoly(vertices, 8, pos.x, pos.y, IGNIS_WHITE);
}

// hightlight cartesian version / world
void highlightTileWorld(const IsoMap* map, uint32_t index)
{
    uint32_t col = index % map->width;
    uint32_t row = index / map->width;

    if (col >= map->width || row >= map->height) return;

    vec2 pos = getTilePos(map, col, row);
    ignisPrimitives2DRenderRect(pos.x, pos.y, map->tile_size, map->tile_size, IGNIS_WHITE);
}