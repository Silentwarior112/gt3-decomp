/* vecmath.c - unit u114, 0x0024B5B0..0x0024B888 (G001, no FDE: C).
 *
 * Small vector / matrix helpers of the graphics library on top of its
 * matrix stack (pgl_mult_matrix multiplies the current matrix,
 * pgl_translate translates, pgl_get_matrix / pgl_load_matrix store / load
 * it).  Matrices are 16 floats, OpenGL column order (m[12..14] =
 * translation).
 */
#include "engine/pgl/G001_flare.h"
#include "engine/pgl/pgl_font_internal.h"

// FUNC 0x0024B5B0 vec3_normalize
// out = (x, y, z) / |(x, y, z)|   (rsqrt.s)
void vec3_normalize(f32 *out, f32 x, f32 y, f32 z)
{
    f32 invLen = u114_rsqrt(1.0f, x * x + y * y + z * z);

    out[0] = x * invLen;
    out[1] = y * invLen;
    out[2] = z * invLen;
}

// FUNC 0x0024B5F0 vec3_cross
// out = a x b
void vec3_cross(f32 *out, const f32 *a, const f32 *b)
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// FUNC 0x0024B658 glu_look_at
// gluLookAt: f = normalize(centre - eye), up = normalize(up), s = f x up,
// u = s x f; multiplies the current matrix by the rotation rows (s, u, -f)
// and translates by -eye.  Callers: gt2k_menu::CarGetMenuCamera::applyView
// (T106), MenuCarCamera::F_0020F9D8 (T411).
void glu_look_at(f32 ex, f32 ey, f32 ez, f32 cx, f32 cy, f32 cz,
                 f32 ux, f32 uy, f32 uz)
{
    f32 m[16] __attribute__((aligned(16)));        /* sp+0x00 */
    f32 forward[4] __attribute__((aligned(16)));   /* sp+0x40 */
    f32 up[4] __attribute__((aligned(16)));        /* sp+0x50 */
    f32 upOrtho[4] __attribute__((aligned(16)));   /* sp+0x60 */
    f32 side[4] __attribute__((aligned(16)));      /* sp+0x70 */

    vec3_normalize(forward, cx - ex, cy - ey, cz - ez);
    vec3_normalize(up, ux, uy, uz);
    vec3_cross(side, forward, up);
    vec3_cross(upOrtho, side, forward);
    m[0] = side[0];
    m[4] = side[1];
    m[8] = side[2];
    m[1] = upOrtho[0];
    m[5] = upOrtho[1];
    m[9] = upOrtho[2];
    m[2] = -forward[0];
    m[6] = -forward[1];
    m[10] = -forward[2];
    m[15] = 1.0f;
    m[12] = 0;
    m[13] = 0;
    m[14] = 0;
    m[3] = 0;
    m[7] = 0;
    m[11] = 0;
    pgl_mult_matrix(m);
    pgl_translate(-ex, -ey, -ez);
}

// FUNC 0x0024B7B0 mat_reflect
// Multiplies the current matrix by the reflection through the plane with
// the (unit) normal n through the point p:  I - 2 n n^T, translation
// 2 (p . n) n.  Caller: RaceCarModel::setReflectionPlane (T180).
void mat_reflect(f32 px, f32 py, f32 pz, f32 nx, f32 ny, f32 nz)
{
    f32 m[16] __attribute__((aligned(16)));
    f32 dist2 = px * nx + py * ny + pz * nz;
    f32 ax = nx * -2.0f;
    f32 ay = ny * -2.0f;
    f32 az = nz * -2.0f;

    dist2 = dist2 + dist2;
    m[0] = 1.0f - (nx + nx) * nx;
    m[1] = ax * ny;
    m[2] = ax * nz;
    m[3] = 0;
    m[4] = ay * nx;
    m[5] = 1.0f - (ny + ny) * ny;
    m[6] = ay * nz;
    m[7] = 0;
    m[8] = az * nx;
    m[9] = az * ny;
    m[10] = 1.0f - (nz + nz) * nz;
    m[11] = 0;
    m[12] = dist2 * nx;
    m[13] = dist2 * ny;
    m[14] = dist2 * nz;
    m[15] = 1.0f;
    pgl_mult_matrix(m);
}

// FUNC 0x0024B888 proj_shift_depth
// Recovers near / far from the current (perspective) matrix,
// n = m14 / (m10 + 1), f = m14 / (m10 - 1), moves both by d and rebuilds
// m10 = -(n + f) / (n - f), m14 = -2 n f / (n - f).  Callers:
// gt2k_menu::CarsItem::drawFloor (T107), F_001A1DD0,
// CarModel::beginGroundPass (T446).
void proj_shift_depth(f32 d)
{
    f32 m[16] __attribute__((aligned(16)));
    f32 zNear, zFar;

    pgl_get_matrix(m);
    zFar = m[14] / (m[10] - 1.0f) + d;
    zNear = m[14] / (m[10] + 1.0f) + d;
    m[10] = -(zNear + zFar) / (zNear - zFar);
    m[14] = (zNear * -2.0f * zFar) / (zNear - zFar);
    pgl_load_matrix(m);
}
