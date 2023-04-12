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

mat4 mat4_look_at(vec3 eye, vec3 look_at, vec3 up);

mat4 mat4_rotation(vec3 axis, float angle); /* angle in radians */
mat4 mat4_translation(vec3 v);

mat4 mat4_rotate_x(mat4 mat, float f);
mat4 mat4_rotate_y(mat4 mat, float f);
mat4 mat4_rotate_z(mat4 mat, float f);

mat4 mat4_multiply(mat4 l, mat4 r);

#endif /* !MAT4_H */
