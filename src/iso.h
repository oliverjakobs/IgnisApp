#ifndef ISO_H
#define ISO_H

#include <Ignis/Ignis.h>

#include "math/math.h"

typedef struct
{
    uint32_t* grid;
    uint32_t width;
    uint32_t height;

    float tile_size;
    float tile_offset;
} IsoMap;

void isoMapInit(IsoMap* map, uint32_t* grid, uint32_t width, uint32_t height, float tile_size, float tile_offset);
void isoMapDestroy(IsoMap* map);

vec2 isoMapGetCenter(const IsoMap* map);

vec2 screenToWorld(vec2 s_pos); // iso to cartesian
vec2 worldToScreen(vec2 w_pos); // cartesian to iso

uint32_t tileClip(const IsoMap* map, float f);
vec2 getTilePos(const IsoMap* map, uint32_t col, uint32_t row);

uint32_t getTileIndexAt(const IsoMap* map, vec2 w_pos);

// renderer
void renderMap(const IsoMap* map, const IgnisTextureAtlas2D* texture_atlas);

void highlightTile(const IsoMap* map, uint32_t index);
void highlightTileWorld(const IsoMap* map, uint32_t index);

#endif // !ISO_H
