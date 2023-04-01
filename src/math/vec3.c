#include "vec3.h"

#include <math.h>

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
