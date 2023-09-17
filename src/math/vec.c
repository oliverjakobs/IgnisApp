#include "vec.h"

#include <math.h>

vec2 vec2_mult(vec2 v, float f)
{
    vec2 result = { v.x * f, v.y * f };
    return result;
}

vec2 vec2_div(vec2 v, float f)
{
    vec2 result = { v.x / f, v.y / f };
    return result;
}

vec2 vec2_add(vec2 a, vec2 b)
{
    vec2 result = { a.x + b.x, a.y + b.y };
    return result;
}

vec2 vec2_sub(vec2 a, vec2 b)
{
    vec2 result = { a.x - b.x, a.y - b.y };
    return result;
}

vec2 vec2_normalize(vec2 v)
{
    float l = 1.0f / sqrtf(v.x * v.x + v.y * v.y);
    vec2 result = { v.x * l, v.y * l };
    return result;
}

float vec2_dot(vec2 a, vec2 b)
{
    return a.x * b.x + a.y * b.y;
}

vec3 vec3_mult(vec3 vec, float f)
{
    vec3 result = { vec.x * f, vec.y * f, vec.z * f };
    return result;
}

vec3 vec3_add(vec3 a, vec3 b)
{
    vec3 result = { a.x + b.x, a.y + b.y, a.z + b.z };
    return result;
}

vec3 vec3_sub(vec3 a, vec3 b)
{
    vec3 result = { a.x - b.x, a.y - b.y, a.z - b.z };
    return result;
}

vec3 vec3_normalize(vec3 v)
{
    float l = 1.0f / sqrtf(v.x * v.x + v.y * v.y + v.z * v.z);

    vec3 result = { v.x * l,  v.y * l, v.z * l };
    return result;
}

vec3 vec3_cross(vec3 left, vec3 right)
{
    vec3 result = {
        .x = left.y * right.z - left.z * right.y,
        .y = left.z * right.x - left.x * right.z,
        .z = left.x * right.y - left.y * right.x
    };
    return result;
}

float vec3_dot(vec3 left, vec3 right)
{
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

vec3 vec3_negate(vec3 v)
{
    return (vec3) { -v.x, -v.y, -v.z };
}

vec3 vec3_lerp(vec3 v0, vec3 v1, float value)
{
    vec3 result = { 0 };

    result.x = v0.x + value * (v1.x - v0.x);
    result.y = v0.y + value * (v1.y - v0.y);
    result.z = v0.z + value * (v1.z - v0.z);

    return result;
}