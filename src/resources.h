#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H

#include <ignis/ignis.h>

#include "toolbox/tb_hashmap.h"

typedef struct
{
    tb_hashmap textures;    /* <str,IgnisTexture> */
} Resources;

int ResourcesInit(Resources* res);
void ResourcesDestroy(Resources* res);

void ResourcesClear(Resources* res);

const IgnisTexture2D* ResourcesLoadTexture2D(Resources* res, const char* path);
const IgnisTexture2D* ResourcesReloadTexture2D(Resources* res, const IgnisTexture2D* texture, const char* path);


const char* ResourcesGetTexture2DPath(const Resources* res, const IgnisTexture2D* texture);

#endif // !RESOURCE_MANAGER_H
