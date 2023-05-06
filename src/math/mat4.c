#include "mat4.h"

#include <math.h>

mat4 mat4_identity()
{
    mat4 result = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    return result;
}

mat4 mat4_cast(quat q)
{
    float a = q.w;
    float b = q.x;
    float c = q.y;
    float d = q.z;
    float a2 = a * a;
    float b2 = b * b;
    float c2 = c * c;
    float d2 = d * d;

    mat4 result = { 0.0f };
    result.v[0][0] = a2 + b2 - c2 - d2;
    result.v[0][1] = 2.f * (b * c + a * d);
    result.v[0][2] = 2.f * (b * d - a * c);

    result.v[1][0] = 2 * (b * c - a * d);
    result.v[1][1] = a2 - b2 + c2 - d2;
    result.v[1][2] = 2.f * (c * d + a * b);

    result.v[2][0] = 2.f * (b * d + a * c);
    result.v[2][1] = 2.f * (c * d - a * b);
    result.v[2][2] = a2 - b2 - c2 + d2;

    result.v[3][3] = 1.f;

    return result;
}

mat4 mat4_perspective(float fov_y, float aspect, float near, float far)
{
    float tan_half_fov_y = 1.0f / tanf(fov_y * 0.5f);

    mat4 result = { 0.0f };
    result.v[0][0] = tan_half_fov_y / aspect;
    result.v[1][1] = tan_half_fov_y;
    result.v[2][2] = -((far + near) / (far - near));
    result.v[2][3] = -1.f;
    result.v[3][2] = -((2.f * far * near) / (far - near));

    return result;
}

mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far)
{
    float rl = 1.0f / (right - left);
    float tb = 1.0f / (top - bottom);
    float fn = -1.0f / (far - near);

    mat4 result = { 0.0f };
    result.v[0][0] = 2.0f * rl;
    result.v[1][1] = 2.0f * tb;
    result.v[2][2] = 2.0f * fn;
    result.v[3][0] = -(left + right) * rl;
    result.v[3][1] = -(top + bottom) * tb;
    result.v[3][2] = (far + near) * fn;
    result.v[3][3] = 1.f;

    return result;
}


mat4 mat4_look_at(vec3 eye, vec3 look_at, vec3 up)
{
    vec3 forward = vec3_normalize(vec3_sub(look_at, eye));
    vec3 side = vec3_normalize(vec3_cross(forward, up));
    vec3 upward = vec3_cross(side, forward);

    mat4 result;
    result.v[0][0] = side.x;
    result.v[0][1] = upward.x;
    result.v[0][2] = -forward.x;
    result.v[0][3] = 0.0f;
    result.v[1][0] = side.y;
    result.v[1][1] = upward.y;
    result.v[1][2] = -forward.y;
    result.v[1][3] = 0.0f;
    result.v[2][0] = side.z;
    result.v[2][1] = upward.z;
    result.v[2][2] = -forward.z;
    result.v[2][3] = 0.0f;
    result.v[3][0] = -vec3_dot(side, eye);
    result.v[3][1] = -vec3_dot(upward, eye);
    result.v[3][2] =  vec3_dot(forward, eye);
    result.v[3][3] = 1.0f;

    return result;
}

mat4 mat4_rotation(vec3 axis, float angle)
{
    float c = cosf(angle);
    float s = sinf(angle);
    float one_c = 1.0f - c;
    float x = axis.x;
    float y = axis.y;
    float z = axis.z;
    float xx = x * x;
    float xy = x * y;
    float xz = x * z;
    float yy = y * y;
    float yz = y * z;
    float zz = z * z;
    float l = xx + yy + zz;
    float sqrt_l = sqrtf(l);

    mat4 result = { 0.0f };
    result.v[0][0] = (xx + (yy + zz) * c) / l;
    result.v[0][1] = (xy * one_c + z * sqrt_l * s) / l;
    result.v[0][2] = (xz * one_c - y * sqrt_l * s) / l;
    result.v[1][0] = (xy * one_c - z * sqrt_l * s) / l;
    result.v[1][1] = (yy + (xx + zz) * c) / l;
    result.v[1][2] = (yz * one_c + x * sqrt_l * s) / l;
    result.v[2][0] = (xz * one_c + y * sqrt_l * s) / l;
    result.v[2][1] = (yz * one_c - x * sqrt_l * s) / l;
    result.v[2][2] = (zz + (xx + yy) * c) / l;
    result.v[3][3] = 1.0f;
    return result;
}

mat4 mat4_translation(vec3 v)
{
    mat4 result = mat4_identity();
    result.v[3][0] = v.x;
    result.v[3][1] = v.y;
    result.v[3][2] = v.z;
    return result;
}

mat4 mat4_scale(vec3 v)
{
    mat4 result = mat4_identity();
    result.v[0][0] = v.x;
    result.v[1][1] = v.y;
    result.v[2][2] = v.z;
    return result;
}

mat4 mat4_rotate_x(mat4 mat, float f)
{
    float c = cosf(f);
    float s = sinf(f);

    mat4 result = mat4_identity();
    result.v[1][1] = c;
    result.v[1][2] = s;
    result.v[2][1] = -s;
    result.v[2][2] = c;
    return mat4_multiply(mat, result);
}

mat4 mat4_rotate_y(mat4 mat, float f)
{
    float c = cosf(f);
    float s = sinf(f);

    mat4 result = mat4_identity();
    result.v[0][0] = c;
    result.v[0][2] = -s;
    result.v[2][0] = s;
    result.v[2][2] = c;
    return mat4_multiply(mat, result);
}

mat4 mat4_rotate_z(mat4 mat, float f)
{
    float c = cosf(f);
    float s = sinf(f);

    mat4 result = mat4_identity();
    result.v[0][0] = c;
    result.v[0][1] = s;
    result.v[1][0] = -s;
    result.v[1][1] = c;
    return mat4_multiply(mat, result);
}

mat4 mat4_multiply(mat4 l, mat4 r)
{
    mat4 result;
    result.v[0][0] = l.v[0][0] * r.v[0][0] + l.v[1][0] * r.v[0][1] + l.v[2][0] * r.v[0][2] + l.v[3][0] * r.v[0][3];
    result.v[0][1] = l.v[0][1] * r.v[0][0] + l.v[1][1] * r.v[0][1] + l.v[2][1] * r.v[0][2] + l.v[3][1] * r.v[0][3];
    result.v[0][2] = l.v[0][2] * r.v[0][0] + l.v[1][2] * r.v[0][1] + l.v[2][2] * r.v[0][2] + l.v[3][2] * r.v[0][3];
    result.v[0][3] = l.v[0][3] * r.v[0][0] + l.v[1][3] * r.v[0][1] + l.v[2][3] * r.v[0][2] + l.v[3][3] * r.v[0][3];
    result.v[1][0] = l.v[0][0] * r.v[1][0] + l.v[1][0] * r.v[1][1] + l.v[2][0] * r.v[1][2] + l.v[3][0] * r.v[1][3];
    result.v[1][1] = l.v[0][1] * r.v[1][0] + l.v[1][1] * r.v[1][1] + l.v[2][1] * r.v[1][2] + l.v[3][1] * r.v[1][3];
    result.v[1][2] = l.v[0][2] * r.v[1][0] + l.v[1][2] * r.v[1][1] + l.v[2][2] * r.v[1][2] + l.v[3][2] * r.v[1][3];
    result.v[1][3] = l.v[0][3] * r.v[1][0] + l.v[1][3] * r.v[1][1] + l.v[2][3] * r.v[1][2] + l.v[3][3] * r.v[1][3];
    result.v[2][0] = l.v[0][0] * r.v[2][0] + l.v[1][0] * r.v[2][1] + l.v[2][0] * r.v[2][2] + l.v[3][0] * r.v[2][3];
    result.v[2][1] = l.v[0][1] * r.v[2][0] + l.v[1][1] * r.v[2][1] + l.v[2][1] * r.v[2][2] + l.v[3][1] * r.v[2][3];
    result.v[2][2] = l.v[0][2] * r.v[2][0] + l.v[1][2] * r.v[2][1] + l.v[2][2] * r.v[2][2] + l.v[3][2] * r.v[2][3];
    result.v[2][3] = l.v[0][3] * r.v[2][0] + l.v[1][3] * r.v[2][1] + l.v[2][3] * r.v[2][2] + l.v[3][3] * r.v[2][3];
    result.v[3][0] = l.v[0][0] * r.v[3][0] + l.v[1][0] * r.v[3][1] + l.v[2][0] * r.v[3][2] + l.v[3][0] * r.v[3][3];
    result.v[3][1] = l.v[0][1] * r.v[3][0] + l.v[1][1] * r.v[3][1] + l.v[2][1] * r.v[3][2] + l.v[3][1] * r.v[3][3];
    result.v[3][2] = l.v[0][2] * r.v[3][0] + l.v[1][2] * r.v[3][1] + l.v[2][2] * r.v[3][2] + l.v[3][2] * r.v[3][3];
    result.v[3][3] = l.v[0][3] * r.v[3][0] + l.v[1][3] * r.v[3][1] + l.v[2][3] * r.v[3][2] + l.v[3][3] * r.v[3][3];
    return result;
}

mat4 mat4_invert(mat4 m)
{
    float s[6];
    float c[6];
    s[0] = m.v[0][0] * m.v[1][1] - m.v[1][0] * m.v[0][1];
    s[1] = m.v[0][0] * m.v[1][2] - m.v[1][0] * m.v[0][2];
    s[2] = m.v[0][0] * m.v[1][3] - m.v[1][0] * m.v[0][3];
    s[3] = m.v[0][1] * m.v[1][2] - m.v[1][1] * m.v[0][2];
    s[4] = m.v[0][1] * m.v[1][3] - m.v[1][1] * m.v[0][3];
    s[5] = m.v[0][2] * m.v[1][3] - m.v[1][2] * m.v[0][3];

    c[0] = m.v[2][0] * m.v[3][1] - m.v[3][0] * m.v[2][1];
    c[1] = m.v[2][0] * m.v[3][2] - m.v[3][0] * m.v[2][2];
    c[2] = m.v[2][0] * m.v[3][3] - m.v[3][0] * m.v[2][3];
    c[3] = m.v[2][1] * m.v[3][2] - m.v[3][1] * m.v[2][2];
    c[4] = m.v[2][1] * m.v[3][3] - m.v[3][1] * m.v[2][3];
    c[5] = m.v[2][2] * m.v[3][3] - m.v[3][2] * m.v[2][3];

    /* Assumes it is invertible */
    float idet = 1.0f / (s[0] * c[5] - s[1] * c[4] + s[2] * c[3] + s[3] * c[2] - s[4] * c[1] + s[5] * c[0]);

    mat4 result = { 0 };
    result.v[0][0] = (m.v[1][1] * c[5] - m.v[1][2] * c[4] + m.v[1][3] * c[3]) * idet;
    result.v[0][1] = (-m.v[0][1] * c[5] + m.v[0][2] * c[4] - m.v[0][3] * c[3]) * idet;
    result.v[0][2] = (m.v[3][1] * s[5] - m.v[3][2] * s[4] + m.v[3][3] * s[3]) * idet;
    result.v[0][3] = (-m.v[2][1] * s[5] + m.v[2][2] * s[4] - m.v[2][3] * s[3]) * idet;

    result.v[1][0] = (-m.v[1][0] * c[5] + m.v[1][2] * c[2] - m.v[1][3] * c[1]) * idet;
    result.v[1][1] = (m.v[0][0] * c[5] - m.v[0][2] * c[2] + m.v[0][3] * c[1]) * idet;
    result.v[1][2] = (-m.v[3][0] * s[5] + m.v[3][2] * s[2] - m.v[3][3] * s[1]) * idet;
    result.v[1][3] = (m.v[2][0] * s[5] - m.v[2][2] * s[2] + m.v[2][3] * s[1]) * idet;

    result.v[2][0] = (m.v[1][0] * c[4] - m.v[1][1] * c[2] + m.v[1][3] * c[0]) * idet;
    result.v[2][1] = (-m.v[0][0] * c[4] + m.v[0][1] * c[2] - m.v[0][3] * c[0]) * idet;
    result.v[2][2] = (m.v[3][0] * s[4] - m.v[3][1] * s[2] + m.v[3][3] * s[0]) * idet;
    result.v[2][3] = (-m.v[2][0] * s[4] + m.v[2][1] * s[2] - m.v[2][3] * s[0]) * idet;

    result.v[3][0] = (-m.v[1][0] * c[3] + m.v[1][1] * c[1] - m.v[1][2] * c[0]) * idet;
    result.v[3][1] = (m.v[0][0] * c[3] - m.v[0][1] * c[1] + m.v[0][2] * c[0]) * idet;
    result.v[3][2] = (-m.v[3][0] * s[3] + m.v[3][1] * s[1] - m.v[3][2] * s[0]) * idet;
    result.v[3][3] = (m.v[2][0] * s[3] - m.v[2][1] * s[1] + m.v[2][2] * s[0]) * idet;
    return result;
}

mat4 mat4_interpolate(mat4 mat0, mat4 mat1, float time)
{
    quat rot0 = quat_cast(mat0);
    quat rot1 = quat_cast(mat1);

    quat final_rot = quat_slerp(rot0, rot1, time);

    mat4 final_mat = mat4_cast(final_rot);

    final_mat.v[3][0] = mat0.v[3][0] * (1 - time) + mat1.v[3][0] * time;
    final_mat.v[3][1] = mat0.v[3][1] * (1 - time) + mat1.v[3][1] * time;
    final_mat.v[3][2] = mat0.v[3][2] * (1 - time) + mat1.v[3][2] * time;

    return final_mat;
}

quat quat_identity()
{
    return (quat) { 0.0f, 0.0f, 0.0f, 1.0f };
}

// Calculate slerp-optimized interpolation between two quaternions
quat quat_nlerp(quat q0, quat q1, float value)
{
    quat result = { 0 };

    // QuaternionLerp(q1, q2, amount)
    result.x = q0.x + value * (q1.x - q0.x);
    result.y = q0.y + value * (q1.y - q0.y);
    result.z = q0.z + value * (q1.z - q0.z);
    result.w = q0.w + value * (q1.w - q0.w);

    // QuaternionNormalize(q);
    quat q = result;
    float length = sqrtf(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    if (length == 0.0f) length = 1.0f;
    float ilength = 1.0f / length;

    result.x = q.x * ilength;
    result.y = q.y * ilength;
    result.z = q.z * ilength;
    result.w = q.w * ilength;

    return result;
}

quat quat_slerp(quat q0, quat q1, float value)
{
    quat result = { 0 };

    float cosHalfTheta = q0.x * q1.x + q0.y * q1.y + q0.z * q1.z + q0.w * q1.w;

    if (cosHalfTheta < 0)
    {
        q1.x = -q1.x; q1.y = -q1.y; q1.z = -q1.z; q1.w = -q1.w;
        cosHalfTheta = -cosHalfTheta;
    }

    if (fabsf(cosHalfTheta) >= 1.0f) return q0;

    if (cosHalfTheta > 0.95f) return quat_nlerp(q0, q1, value);


    float halfTheta = acosf(cosHalfTheta);
    float sinHalfTheta = sqrtf(1.0f - cosHalfTheta * cosHalfTheta);

    if (fabsf(sinHalfTheta) < 0.001f)
    {
        result.x = (q0.x * 0.5f + q1.x * 0.5f);
        result.y = (q0.y * 0.5f + q1.y * 0.5f);
        result.z = (q0.z * 0.5f + q1.z * 0.5f);
        result.w = (q0.w * 0.5f + q1.w * 0.5f);
    }
    else
    {
        float ratioA = sinf((1 - value) * halfTheta) / sinHalfTheta;
        float ratioB = sinf(value * halfTheta) / sinHalfTheta;

        result.x = (q0.x * ratioA + q1.x * ratioB);
        result.y = (q0.y * ratioA + q1.y * ratioB);
        result.z = (q0.z * ratioA + q1.z * ratioB);
        result.w = (q0.w * ratioA + q1.w * ratioB);
    }

    return result;
}

quat quat_cast(mat4 mat)
{
    float r = 0.f;
    int i;

    int perm[] = { 0, 1, 2, 0, 1 };
    int* p = perm;

    for (i = 0; i < 3; i++)
    {
        float m = mat.v[i][i];
        if (m < r) continue;
        m = r;
        p = &perm[i];
    }

    r = sqrtf(1.f + mat.v[p[0]][p[0]] - mat.v[p[1]][p[1]] - mat.v[p[2]][p[2]]);

    if (r < 1e-6) return (quat) { 1.0f, 0.0f, 0.0f, 0.0f };

    quat q = {
        r / 2.f,
        (mat.v[p[0]][p[1]] - mat.v[p[1]][p[0]]) / (2.f * r),
        (mat.v[p[2]][p[0]] - mat.v[p[0]][p[2]]) / (2.f * r),
        (mat.v[p[2]][p[1]] - mat.v[p[1]][p[2]]) / (2.f * r)
    };
    return q;
}