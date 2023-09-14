#include "Camera.h"

void cameraCreate(Camera* camera, float x, float y, float w, float h)
{
    camera->proj = mat4_identity();
    camera->view = mat4_identity();
    camera->view_proj = mat4_identity();

    camera->position.x = x;
    camera->position.y = y;
    camera->size.x = w;
    camera->size.y = h;
}

void cameraCreateV(Camera* camera, vec2 pos, vec2 size) { cameraCreate(camera, pos.x, pos.y, size.x, size.y); }

void cameraCreateOrtho(Camera* camera, float x, float y, float w, float h)
{
    cameraCreate(camera, x, y, w, h);
    cameraSetProjectionOrtho(camera, w, h);
}

void cameraCreateOrthoV(Camera* camera, vec2 pos, vec2 size)
{
    cameraCreateOrtho(camera, pos.x, pos.y, size.x, size.y);
}

void cameraUpdateViewOrtho(Camera* camera)
{
    vec3 position = { -camera->position.x, -camera->position.y, 0.0f };
    camera->view = mat4_translate(mat4_identity(), position);
    camera->view_proj = mat4_multiply(camera->proj, camera->view);
}

void cameraSetProjectionOrtho(Camera* camera, float w, float h)
{
    camera->proj = mat4_ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);

    camera->size.x = w;
    camera->size.y = h;

    cameraUpdateViewOrtho(camera);
}

void cameraSetProjectionOrthoV(Camera* camera, vec2 size)
{
    cameraSetProjectionOrtho(camera, size.x, size.y);
}

void cameraSetPositionOrtho(Camera* camera, vec2 position)
{
    camera->position = position;
    cameraUpdateViewOrtho(camera);
}

void cameraSetCenterOrtho(Camera* camera, vec2 center)
{
    camera->position = vec2_sub(center, vec2_mult(camera->size, 0.5f));
    cameraUpdateViewOrtho(camera);
}

vec2 cameraGetCenter(const Camera* camera)
{
    return vec2_add(camera->position, vec2_mult(camera->size, 0.5f));
}

vec2 cameraGetMousePos(const Camera* camera, vec2 mouse)
{
    return vec2_add(mouse, camera->position);
}

const float* cameraGetProjectionPtr(const Camera* camera)     { return camera->proj.v; }
const float* cameraGetViewProjectionPtr(const Camera* camera) { return camera->view_proj.v; }
