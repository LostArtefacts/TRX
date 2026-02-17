#include <trx/game/fx/explosion_ring.h>

#include <trx/core/math/func.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>

static FX_EXPLOSION_RING m_ExplosionRings[6] = {};

void FX_ExplosionRing_Reset(void)
{
    for (int32_t i = 0;
         i < (int32_t)(sizeof(m_ExplosionRings) / sizeof(m_ExplosionRings[0]));
         i++) {
        m_ExplosionRings[i] = (FX_EXPLOSION_RING) { 0 };
    }
}

FX_EXPLOSION_RING *FX_ExplosionRing_GetRing(const int32_t idx)
{
    if (idx < 0
        || idx >= (int32_t)(sizeof(m_ExplosionRings)
                            / sizeof(m_ExplosionRings[0]))) {
        return nullptr;
    }
    return &m_ExplosionRings[idx];
}

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

void FX_ExplosionRing_Control(void)
{
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_ExplosionRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_ExplosionRings[i];
        if (ring->on == 0) {
            continue;
        }

        ring->life--;
        if (ring->life == 0) {
            ring->on = 0;
            continue;
        }

        ring->radius += ring->speed;
    }
}

void FX_ExplosionRing_Draw(void)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    const int32_t sprite_base = Object_Get(O_EXPLOSION_1)->mesh_idx;
    const int32_t sprite_idx = sprite_base + 4 + ((time4 >> 4) & 3);

    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(m_ExplosionRings); i++) {
        FX_EXPLOSION_RING *const ring = &m_ExplosionRings[i];
        if (ring->on == 0) {
            continue;
        }

        int32_t rad = ring->radius;
        for (int32_t band = 0; band < 2; band++) {
            int32_t ang = (time4 & 0x3F) << 3;
            for (int32_t k = 0; k < 8; k++) {
                const int32_t idx = band * 8 + k;
                FX_EXPLOSION_VERT *const vtx = &ring->verts[idx];

                vtx->pos.x = (int16_t)((rad * Math_Sin(ang << 4)) >> W2V_SHIFT);
                vtx->pos.z = (int16_t)((rad * Math_Cos(ang << 4)) >> W2V_SHIFT);

                int32_t r = 0;
                int32_t g = 0;
                int32_t b = 0;
                if (ring->on == 2) {
                    // Tony
                    r = (Random_GetDraw() & 0x1F) + 224;
                    g = (r >> 2) + (Random_GetDraw() & 0x3F);
                    b = Random_GetDraw() & 0x3F;
                } else if (ring->on == 3) {
                    // Sophia
                    r = Random_GetDraw() & 0x3F;
                    g = (Random_GetDraw() & 0x1F) + 224;
                    b = (g >> 2) + (Random_GetDraw() & 0x3F);
                } else if (ring->on == 4) {
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

                r = (r * ring->life) >> 5;
                g = (g * ring->life) >> 5;
                b = (b * ring->life) >> 5;
                vtx->color = (RGB_888) { r, g, b };

                ang = (ang + 512) & 0xFFF;
            }
            rad >>= 1;
        }

        const int32_t rot_z = ring->rot.z << 4;
        const int32_t rot_x = ring->rot.x << 4;

        for (int32_t j = 0; j < 8; j++) {
            const int32_t j2 = (j == 7) ? 0 : (j + 1);
            const FX_EXPLOSION_VERT *const o0 = &ring->verts[j];
            const FX_EXPLOSION_VERT *const o1 = &ring->verts[j2];
            const FX_EXPLOSION_VERT *const i0 = &ring->verts[8 + j];
            const FX_EXPLOSION_VERT *const i1 = &ring->verts[8 + j2];

            if ((o0->color.r | o0->color.g | o0->color.b | o1->color.r
                 | o1->color.g | o1->color.b | i0->color.r | i0->color.g
                 | i0->color.b | i1->color.r | i1->color.g | i1->color.b)
                == 0U) {
                continue;
            }

            XYZ_32 p_local[4] = {
                { o0->pos.x, 0, o0->pos.z },
                { o1->pos.x, 0, o1->pos.z },
                { i1->pos.x, 0, i1->pos.z },
                { i0->pos.x, 0, i0->pos.z },
            };
            XYZ_32 p_rot[4];
            XYZ_32 p_world[4];
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
}
