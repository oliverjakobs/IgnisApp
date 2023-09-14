#ifndef CAMERA_H
#define CAMERA_H

#include "math/math.h"

typedef struct
{
    mat4 proj;
    mat4 view;
    mat4 view_proj;

    vec2 position;
    vec2 size;
} Camera;

void cameraCreate(Camera* camera, float x, float y, float w, float h);
void cameraCreateV(Camera* camera, vec2 pos, vec2 size);

/*
 * --------------------------------------------------------------
 *                          ortho
 * --------------------------------------------------------------
 */
void cameraCreateOrtho(Camera* camera, float x, float y, float w, float h);
void cameraCreateOrthoV(Camera* camera, vec2 pos, vec2 size);

/* Call after manually changing size or position */
void cameraUpdateViewOrtho(Camera* camera);

void cameraSetProjectionOrtho(Camera* camera, float w, float h);
void cameraSetProjectionOrthoV(Camera* camera, vec2 size);

void cameraSetPositionOrtho(Camera* camera, vec2 pos);
void cameraSetCenterOrtho(Camera* camera, vec2 center);

/*
 * --------------------------------------------------------------
 *                          get
 * --------------------------------------------------------------
 */
vec2 cameraGetCenter(const Camera* camera);
vec2 cameraGetMousePos(const Camera* camera, vec2 mouse);

const float* cameraGetProjectionPtr(const Camera* camera);
const float* cameraGetViewProjectionPtr(const Camera* camera);

#endif /* !CAMERA_H */