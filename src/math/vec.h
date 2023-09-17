#ifndef VEC_H
#define VEC_H


#include <stdint.h>

typedef struct
{
    float x;
    float y;
} vec2;

vec2 vec2_mult(vec2 v, float f);
vec2 vec2_div(vec2 v, float f);
vec2 vec2_add(vec2 a, vec2 b);
vec2 vec2_sub(vec2 a, vec2 b);

vec2 vec2_normalize(vec2 v);

float vec2_dot(vec2 a, vec2 b);

typedef struct
{
    uint32_t x;
    uint32_t y;
} vec2i;

typedef struct
{
    float x;
    float y;
    float z;
} vec3;

vec3 vec3_mult(vec3 v, float f);
vec3 vec3_add(vec3 a, vec3 b);
vec3 vec3_sub(vec3 a, vec3 b);

vec3 vec3_normalize(vec3 v);
vec3 vec3_cross(vec3 left, vec3 right);

float vec3_dot(vec3 left, vec3 right);

vec3 vec3_negate(vec3 v);

vec3 vec3_lerp(vec3 v0, vec3 v1, float value);

#endif /* !VEC_H */
