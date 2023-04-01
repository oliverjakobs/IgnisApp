#ifndef MAT4_H
#define MAT4_H

#include "vec3.h"

typedef struct
{
    float v[4][4];
} mat4;

mat4 mat4_identity();

mat4 mat4_perspective(float fov_y, float aspect, float near, float far);
mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far);

mat4 mat4_rotation(vec3 axis, float angle); /* angle in radians */
mat4 mat4_translation(vec3 v);


#endif /* !MAT4_H */
