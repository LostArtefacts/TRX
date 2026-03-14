#include <trx/game/fx/explosion_ring.h>

#include <trx/core/math/func.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/interpolation.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>

static FX_EXPLOSION_RING m_ExplosionRings[6] = {};
static FX_EXPLOSION_RING m_SummonRings[6] = {};
static FX_EXPLOSION_RING m_KnockBackRings[3] = {};
static bool m_ExplosionActive = false;
static bool m_SummonActive = false;
static bool m_KnockBackActive = false;

static void M_RotateZX(
    XYZ_32 *const out, const XYZ_32 in, const int32_t rot_z,
    const int32_t rot_x)
{
    const int32_t sz = Math_Sin(rot_z);
    const int32_t cz = Math_Cos(rot_z);
    const int32_t sx = Math_Sin(rot_x);
    const int32_t cx = Math_Cos(rot_x);

    const int32_t xz = (in.x * cz - in.y * sz) >> W2V_SHIFT;
    const int32_t yz = (in.x * sz + in.y * cz) >> W2V_SHIFT;
    const int32_t zz = in.z;

    out->x = xz;
    out->y = (yz * cx - zz * sx) >> W2V_SHIFT;
    out->z = (yz * sx + zz * cx) >> W2V_SHIFT;
}

static void M_RememberRing(FX_EXPLOSION_RING *const ring)
{
    ring->prev_radius = ring->radius;
    ring->prev_rot = ring->rot;
    ring->prev_pos = ring->pos;
}

static void M_InterpolateRing(
    const FX_EXPLOSION_RING *const ring, FX_EXPLOSION_RING *const out)
{
    *out = *ring;
    const double ratio = Interpolation_GetWorldRate();
    out->radius = (int16_t)LERP(ring->prev_radius, ring->radius, ratio);
    out->rot.x = Math_AngleMean(ring->prev_rot.x, ring->rot.x, ratio);
    out->rot.z = Math_AngleMean(ring->prev_rot.z, ring->rot.z, ratio);
    out->pos.x = (int32_t)LERP(ring->prev_pos.x, ring->pos.x, ratio);
    out->pos.y = (int32_t)LERP(ring->prev_pos.y, ring->pos.y, ratio);
    out->pos.z = (int32_t)LERP(ring->prev_pos.z, ring->pos.z, ratio);
}

static void M_BuildRingCircle(
    FX_EXPLOSION_RING *const ring, const int32_t radius, const int32_t band,
    const bool clear_inner, const int32_t angle_base)
{
    int32_t angle = angle_base;
    for (int32_t i = 0; i < 8; i++) {
        FX_EXPLOSION_VERT *const vtx = &ring->verts[band * 8 + i];
        vtx->pos.x = (radius * Math_Sin(angle << 4)) >> W2V_SHIFT;
        vtx->pos.z = (radius * Math_Cos(angle << 4)) >> W2V_SHIFT;
        if (clear_inner && band != 0) {
            vtx->color = COLOR_RGB_888_BLACK;
        }
        angle = (angle + 512) & 0xFFF;
    }
}

static void M_DrawTexturedRing(const FX_EXPLOSION_RING *const ring)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    const int32_t sprite_base = Object_Get(O_EXPLOSION_1)->mesh_idx;
    const int32_t sprite_idx = sprite_base + 4 + ((time4 >> 4) & 3);
    const int32_t rot_z = ring->rot.z << 4;
    const int32_t rot_x = ring->rot.x << 4;

    for (int32_t j = 0; j < 8; j++) {
        const int32_t j2 = (j == 7) ? 0 : (j + 1);
        const FX_EXPLOSION_VERT *const o0 = &ring->verts[j];
        const FX_EXPLOSION_VERT *const o1 = &ring->verts[j2];
        const FX_EXPLOSION_VERT *const i0 = &ring->verts[8 + j];
        const FX_EXPLOSION_VERT *const i1 = &ring->verts[8 + j2];

        if ((o0->color.r | o0->color.g | o0->color.b | o1->color.r | o1->color.g
             | o1->color.b | i0->color.r | i0->color.g | i0->color.b
             | i1->color.r | i1->color.g | i1->color.b)
            == 0U) {
            continue;
        }

        XYZ_32 p_local[4] = {
            { o0->pos.x, 0, o0->pos.z },
            { o1->pos.x, 0, o1->pos.z },
            { i1->pos.x, 0, i1->pos.z },
            { i0->pos.x, 0, i0->pos.z },
        };
        XYZ_32 p_rot[4] = {};
        XYZ_32 p_world[4] = {};
        for (int32_t c = 0; c < 4; c++) {
            M_RotateZX(&p_rot[c], p_local[c], rot_z, rot_x);
            p_world[c].x = ring->pos.x + p_rot[c].x;
            p_world[c].y = ring->pos.y + p_rot[c].y;
            p_world[c].z = ring->pos.z + p_rot[c].z;
        }

        const RGBA_8888 color[4] = {
            { o0->color.r, o0->color.g, o0->color.b, 255 },
            { o1->color.r, o1->color.g, o1->color.b, 255 },
            { i1->color.r, i1->color.g, i1->color.b, 255 },
            { i0->color.r, i0->color.g, i0->color.b, 255 },
        };

        OutputSource_PolyFX_StageSpriteQuadWorld(
            sprite_idx, p_world, color, DRAW_BLEND_ADD);
    }
}

static void M_DrawFlatRing(const FX_EXPLOSION_RING *const ring)
{
    const int32_t rot_z = ring->rot.z << 4;
    const int32_t rot_x = ring->rot.x << 4;

    for (int32_t j = 0; j < 8; j++) {
        const int32_t j2 = (j == 7) ? 0 : (j + 1);
        const FX_EXPLOSION_VERT *const o0 = &ring->verts[j];
        const FX_EXPLOSION_VERT *const o1 = &ring->verts[j2];
        const FX_EXPLOSION_VERT *const i0 = &ring->verts[8 + j];
        const FX_EXPLOSION_VERT *const i1 = &ring->verts[8 + j2];

        if ((o0->color.r | o0->color.g | o0->color.b | o1->color.r | o1->color.g
             | o1->color.b | i0->color.r | i0->color.g | i0->color.b
             | i1->color.r | i1->color.g | i1->color.b)
            == 0U) {
            continue;
        }

        XYZ_32 p_local[4] = {
            { o0->pos.x, 0, o0->pos.z },
            { o1->pos.x, 0, o1->pos.z },
            { i1->pos.x, 0, i1->pos.z },
            { i0->pos.x, 0, i0->pos.z },
        };
        XYZ_32 p_rot[4] = {};
        XYZ_32 p_world[4] = {};
        for (int32_t c = 0; c < 4; c++) {
            M_RotateZX(&p_rot[c], p_local[c], rot_z, rot_x);
            p_world[c].x = ring->pos.x + p_rot[c].x;
            p_world[c].y = ring->pos.y + p_rot[c].y;
            p_world[c].z = ring->pos.z + p_rot[c].z;
        }

        const XYZ_32 world_pos[4] = {
            p_world[0],
            p_world[1],
            p_world[2],
            p_world[3],
        };
        const RGBA_8888 color[4] = {
            { o0->color.r, o0->color.g, o0->color.b, 0xC0 },
            { o1->color.r, o1->color.g, o1->color.b, 0xC0 },
            { i1->color.r, i1->color.g, i1->color.b, 0xC0 },
            { i0->color.r, i0->color.g, i0->color.b, 0xC0 },
        };
        OutputSource_PolyFX_StageQuadExt(
            -1, world_pos, nullptr, color,
            VERT_FLAT_SHADED | VERT_NO_LIGHTING | VERT_NO_WIBBLE,
            DRAW_BLEND_ADD);
    }
}

static void M_ControlExplosionRings(void)
{
    bool any_active = false;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_ExplosionRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_ExplosionRings[i];
        if (ring->on == 0) {
            continue;
        }

        M_RememberRing(ring);
        ring->life--;
        if (ring->life == 0) {
            ring->on = 0;
            continue;
        }

        ring->radius += ring->speed;
        any_active = true;
    }
    if (!any_active) {
        m_ExplosionActive = false;
    }
}

static void M_ControlSummonRings(void)
{
    bool any_active = false;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_SummonRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_SummonRings[i];
        if (ring->on == 0) {
            continue;
        }

        M_RememberRing(ring);
        ring->life--;
        ring->radius -= ring->speed;
        if (ring->life == 0 || ring->radius <= 0) {
            ring->on = 0;
            continue;
        }

        ring->speed += 2;
        any_active = true;
    }
    if (!any_active) {
        m_SummonActive = false;
    }
}

static void M_ControlKnockBackRings(void)
{
    bool any_active = false;
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_KnockBackRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_KnockBackRings[i];
        if (ring->on == 0) {
            continue;
        }

        M_RememberRing(ring);
        ring->life--;
        if (ring->life == 0) {
            ring->on = 0;
            continue;
        }

        ring->radius += ring->speed;
        if (ring->speed < 0) {
            ring->speed--;
        } else {
            ring->speed += i == 1 ? 3 : 2;
        }
        any_active = true;
    }
    if (!any_active) {
        m_KnockBackActive = false;
    }
}

static void M_DrawExplosionRings(const int32_t angle_base)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_ExplosionRings); i++) {
        FX_EXPLOSION_RING draw_ring = {};
        FX_EXPLOSION_RING *const ring = &m_ExplosionRings[i];
        if (ring->on == 0) {
            continue;
        }

        M_InterpolateRing(ring, &draw_ring);

        int32_t rad = draw_ring.radius;
        for (int32_t band = 0; band < 2; band++) {
            M_BuildRingCircle(&draw_ring, rad, band, false, angle_base);
            for (int32_t k = 0; k < 8; k++) {
                FX_EXPLOSION_VERT *const vtx = &draw_ring.verts[band * 8 + k];

                int32_t r = 0;
                int32_t g = 0;
                int32_t b = 0;
                if (draw_ring.on == 2) {
                    // Tony
                    r = (Random_GetDraw() & 0x1F) + 224;
                    g = (r >> 2) + (Random_GetDraw() & 0x3F);
                    b = Random_GetDraw() & 0x3F;
                } else if (draw_ring.on == 3) {
                    // Sophia
                    r = Random_GetDraw() & 0x3F;
                    g = (Random_GetDraw() & 0x1F) + 224;
                    b = (g >> 2) + (Random_GetDraw() & 0x3F);
                } else if (draw_ring.on == 4) {
                    // Puna
                    r = Random_GetDraw() & 0x1F;
                    b = (Random_GetDraw() & 0x3F) + 224;
                    g = (b >> 2) + (Random_GetDraw() & 0x3F);
                } else {
                    // Willard
                    r = Random_GetDraw() & 0x3F;
                    g = (Random_GetDraw() & 0x1F) + 224;
                    b = (g >> 1) + (Random_GetDraw() & 0x3F);
                }

                vtx->color = (RGB_888) {
                    (r * ring->life) >> 5,
                    (g * ring->life) >> 5,
                    (b * ring->life) >> 5,
                };
            }
            rad >>= 1;
        }

        M_DrawTexturedRing(&draw_ring);
    }
}

static void M_DrawSummonRings(const int32_t angle_base)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_SummonRings); i++) {
        FX_EXPLOSION_RING draw_ring = {};
        FX_EXPLOSION_RING *const ring = &m_SummonRings[i];
        if (ring->on == 0) {
            continue;
        }

        M_InterpolateRing(ring, &draw_ring);

        const int32_t fade = ring->life > 32 ? (64 - ring->life) << 1
            : ring->life < 8                 ? ring->life << 3
                                             : 64;
        int32_t rad = draw_ring.radius;
        for (int32_t band = 0; band < 2; band++) {
            M_BuildRingCircle(&draw_ring, rad, band, true, angle_base);
            if (band == 0) {
                for (int32_t k = 0; k < 8; k++) {
                    FX_EXPLOSION_VERT *const vtx = &draw_ring.verts[k];
                    const int32_t g = (Random_GetDraw() & 0x1F) + 224;
                    const int32_t b = (g >> 2) + (Random_GetDraw() & 0x3F);
                    const int32_t r = Random_GetDraw() & 0x3F;
                    vtx->color = (RGB_888) {
                        (r * fade) >> 7,
                        (g * fade) >> 7,
                        (b * fade) >> 7,
                    };
                }
            }
            rad >>= 1;
        }

        M_DrawFlatRing(&draw_ring);
    }
}

static void M_DrawKnockBackRings(const int32_t angle_base)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_KnockBackRings); i++) {
        FX_EXPLOSION_RING draw_ring = {};
        FX_EXPLOSION_RING *const ring = &m_KnockBackRings[i];
        if (ring->on == 0) {
            continue;
        }

        M_InterpolateRing(ring, &draw_ring);

        int32_t fade = ring->life > 24 ? (32 - ring->life) << 2
            : ring->life < 16          ? ring->life << 1
                                       : 32;
        int32_t rad = draw_ring.radius;
        for (int32_t band = 0; band < 2; band++) {
            M_BuildRingCircle(&draw_ring, rad, band, false, angle_base);
            for (int32_t k = 0; k < 8; k++) {
                FX_EXPLOSION_VERT *const vtx = &draw_ring.verts[band * 8 + k];
                const int32_t g = (Random_GetDraw() & 0x1F) + 224;
                const int32_t b = (g >> 2) + (Random_GetDraw() & 0x3F);
                const int32_t r = Random_GetDraw() & 0x3F;
                vtx->color = (RGB_888) {
                    (r * fade) >> 5,
                    (g * fade) >> 5,
                    (b * fade) >> 5,
                };
            }
            rad >>= 1;
            fade >>= 1;
        }

        M_DrawTexturedRing(&draw_ring);
    }
}

void FX_ExplosionRing_Reset(void)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_ExplosionRings); i++) {
        m_ExplosionRings[i] = (FX_EXPLOSION_RING) { 0 };
        m_SummonRings[i] = (FX_EXPLOSION_RING) { 0 };
    }
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_KnockBackRings); i++) {
        m_KnockBackRings[i] = (FX_EXPLOSION_RING) { 0 };
    }
    m_ExplosionActive = false;
    m_SummonActive = false;
    m_KnockBackActive = false;
}

void FX_ExplosionRing_Sync(FX_EXPLOSION_RING *const ring)
{
    if (ring == nullptr) {
        return;
    }
    M_RememberRing(ring);
}

FX_EXPLOSION_RING *FX_ExplosionRing_GetRing(const int32_t idx)
{
    if (idx < 0 || idx >= (int32_t)ARRAY_SIZE(m_ExplosionRings)) {
        return nullptr;
    }
    m_ExplosionActive = true;
    return &m_ExplosionRings[idx];
}

FX_EXPLOSION_RING *FX_ExplosionRing_GetSummonRing(const int32_t idx)
{
    if (idx < 0 || idx >= (int32_t)ARRAY_SIZE(m_SummonRings)) {
        return nullptr;
    }
    m_SummonActive = true;
    return &m_SummonRings[idx];
}

FX_EXPLOSION_RING *FX_ExplosionRing_GetKnockBackRing(const int32_t idx)
{
    if (idx < 0 || idx >= (int32_t)ARRAY_SIZE(m_KnockBackRings)) {
        return nullptr;
    }
    m_KnockBackActive = true;
    return &m_KnockBackRings[idx];
}

const FX_EXPLOSION_RING *FX_ExplosionRing_PeekKnockBackRing(const int32_t idx)
{
    if (idx < 0 || idx >= (int32_t)ARRAY_SIZE(m_KnockBackRings)) {
        return nullptr;
    }
    return &m_KnockBackRings[idx];
}

void FX_ExplosionRing_Control(void)
{
    if (m_ExplosionActive) {
        M_ControlExplosionRings();
    }

    if (m_SummonActive) {
        M_ControlSummonRings();
    }

    if (m_KnockBackActive) {
        M_ControlKnockBackRings();
    }
}

void FX_ExplosionRing_SpawnKnockBack(const XYZ_32 pos)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_KnockBackRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_KnockBackRings[i];
        ring->on = 1;
        ring->life = 32;
        ring->speed = ((i == 1) + 1) << 4;
        ring->pos.x = pos.x;
        ring->pos.y = pos.y - 512 + (i << 7);
        ring->pos.z = pos.z;
        ring->rot.x = 0;
        ring->rot.z = 0;
        ring->radius = ((i == 1) + 2) << 8;
        FX_ExplosionRing_Sync(ring);
    }
    m_KnockBackActive = true;
}

void FX_ExplosionRing_BounceKnockBack(void)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_KnockBackRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_KnockBackRings[i];
        ring->speed = (int16_t)((-ring->speed) >> 2);
    }
}

bool FX_ExplosionRing_IsKnockBackActive(void)
{
    return m_KnockBackActive;
}

void FX_ExplosionRing_Draw(void)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    const int32_t angle_base = (time4 & 0x3F) << 3;
    M_DrawExplosionRings(angle_base);
    M_DrawSummonRings(angle_base);
    M_DrawKnockBackRings(angle_base);
}
