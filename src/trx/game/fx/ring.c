#include <trx/game/fx/ring.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/log.h>
#include <trx/core/math/func.h>
#include <trx/core/utils.h>
#include <trx/game/const.h>
#include <trx/game/fx/common.h>
#include <trx/game/interpolation.h>
#include <trx/game/objects/common.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>

#include <string.h>

#define M_MAX_RINGS 6

static FX_RING m_Rings[FX_RING_TYPE_NUMBER_OF][M_MAX_RINGS] = {};
static bool m_Active[FX_RING_TYPE_NUMBER_OF] = {};

static void M_RememberRing(FX_RING *const ring)
{
    ring->prev_radius = ring->radius;
    ring->prev_rot = ring->rot;
    ring->prev_pos = ring->pos;
}

static void M_InterpolateRing(const FX_RING *const ring, FX_RING *const out)
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
    FX_RING *const ring, const int32_t radius, const int32_t band,
    const bool clear_inner, const int32_t angle_base)
{
    int32_t angle = angle_base;
    for (int32_t i = 0; i < 8; i++) {
        FX_RING_VERT *const vtx = &ring->verts[band * 8 + i];
        const XYZ_32 point =
            XYZ_32_RotateYaw((XYZ_32) { .z = radius }, angle << 4);
        vtx->pos.x = point.x;
        vtx->pos.z = point.z;
        if (clear_inner && band != 0) {
            vtx->color = COLOR_RGB_888_BLACK;
        }
        angle = (angle + 512) & 0xFFF;
    }
}

static void M_DrawTexturedRing(const FX_RING *const ring)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    const int32_t offset = (time4 >> 4) & 3;
    const int32_t sprite_idx =
        Sparks_GetSpriteIndex(SPARK_TYPE_SMALL_SPLASH + offset);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    const int32_t rot_z = ring->rot.z << 4;
    const int32_t rot_x = ring->rot.x << 4;

    for (int32_t j = 0; j < 8; j++) {
        const int32_t j2 = (j == 7) ? 0 : (j + 1);
        const FX_RING_VERT *const o0 = &ring->verts[j];
        const FX_RING_VERT *const o1 = &ring->verts[j2];
        const FX_RING_VERT *const i0 = &ring->verts[8 + j];
        const FX_RING_VERT *const i1 = &ring->verts[8 + j2];

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
            p_rot[c] =
                XYZ_32_Rotate(p_local[c], (XYZ_16) { .x = rot_x, .z = rot_z });
            p_world[c] = XYZ_32_Add(ring->pos, p_rot[c]);
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

static void M_DrawFlatRing(const FX_RING *const ring)
{
    const int32_t rot_z = ring->rot.z << 4;
    const int32_t rot_x = ring->rot.x << 4;

    for (int32_t j = 0; j < 8; j++) {
        const int32_t j2 = (j == 7) ? 0 : (j + 1);
        const FX_RING_VERT *const o0 = &ring->verts[j];
        const FX_RING_VERT *const o1 = &ring->verts[j2];
        const FX_RING_VERT *const i0 = &ring->verts[8 + j];
        const FX_RING_VERT *const i1 = &ring->verts[8 + j2];

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
            p_rot[c] =
                XYZ_32_Rotate(p_local[c], (XYZ_16) { .x = rot_x, .z = rot_z });
            p_world[c] = XYZ_32_Add(ring->pos, p_rot[c]);
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
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_BLAST][i];
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
        m_Active[FX_RING_TYPE_BLAST] = false;
    }
}

static void M_ControlSummonRings(void)
{
    bool any_active = false;
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_SUMMON][i];
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
        m_Active[FX_RING_TYPE_SUMMON] = false;
    }
}

static void M_ControlKnockBackRings(void)
{
    bool any_active = false;
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_KNOCKBACK][i];
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
        m_Active[FX_RING_TYPE_KNOCKBACK] = false;
    }
}

static void M_DrawExplosionRings(const int32_t angle_base)
{
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING draw_ring = {};
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_BLAST][i];
        if (ring->on == 0) {
            continue;
        }

        M_InterpolateRing(ring, &draw_ring);

        int32_t rad = draw_ring.radius;
        for (int32_t band = 0; band < 2; band++) {
            M_BuildRingCircle(&draw_ring, rad, band, false, angle_base);
            for (int32_t k = 0; k < 8; k++) {
                FX_RING_VERT *const vtx = &draw_ring.verts[band * 8 + k];

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
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING draw_ring = {};
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_SUMMON][i];
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
                    FX_RING_VERT *const vtx = &draw_ring.verts[k];
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
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING draw_ring = {};
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_KNOCKBACK][i];
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
                FX_RING_VERT *const vtx = &draw_ring.verts[band * 8 + k];
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

static void M_SaveRings(
    JSON_WRITE_IO *const io, const FX_RING_TYPE type, const char *const key)
{
    if (!m_Active[type]) {
        return;
    }

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        const FX_RING *const ring = &m_Rings[type][i];
        if (!ring->on) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "on", ring->on);
        JSONW_WRITE(io, "life", ring->life);
        JSONW_WRITE(io, "speed", ring->speed);
        JSONW_WRITE(io, "radius", ring->radius);
        JSONW_WRITE(io, "prev_radius", ring->prev_radius);
        JSONW_WRITE(io, "rot", ((XYZ_16) { ring->rot.x, 0, ring->rot.z }));
        JSONW_WRITE(
            io, "prev_rot",
            ((XYZ_16) { ring->prev_rot.x, 0, ring->prev_rot.z }));
        JSONW_WRITE(io, "pos", ring->pos);
        JSONW_WRITE(io, "prev_pos", ring->prev_pos);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, key);
}

static void M_Save(JSON_WRITE_IO *const io)
{
    M_SaveRings(io, FX_RING_TYPE_BLAST, "blast");
    M_SaveRings(io, FX_RING_TYPE_KNOCKBACK, "knockback");
    M_SaveRings(io, FX_RING_TYPE_SUMMON, "summon");
}

static RESULT M_LoadRing(JSON_READ_IO *const io, FX_RING *const ring)
{
    MUST(JSON_READ(io, "on", &ring->on));
    MUST(JSON_READ(io, "life", &ring->life));
    MUST(JSON_READ(io, "speed", &ring->speed));
    MUST(JSON_READ(io, "radius", &ring->radius));
    MUST(JSON_READ(io, "prev_radius", &ring->prev_radius));

    XYZ_16 rot = {};
    MUST(JSON_READ(io, "rot", &rot));
    ring->rot = (XZ_16) { rot.x, rot.z };

    XYZ_16 prev_rot = {};
    MUST(JSON_READ(io, "prev_rot", &prev_rot));
    ring->prev_rot = (XZ_16) { prev_rot.x, prev_rot.z };

    MUST(JSON_READ(io, "pos", &ring->pos));
    MUST(JSON_READ(io, "prev_pos", &ring->prev_pos));
    return OK;
}

static RESULT M_LoadRings(
    JSON_READ_IO *const io, const FX_RING_TYPE type, const char *const key)
{
    if (!JSON_ReadIO_HasKey(io, key)) {
        return OK;
    }
    MUST(JSON_PUSH(io, key));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        MUST(JSON_PUSH_INDEX(io, i));
        FX_RING *const ring = FX_Ring_GetRing(type, i);
        if (ring != nullptr) {
            MUST(M_LoadRing(io, ring));
        } else {
            LOG_WARNING(
                "Malformed save: too many %s rings. Extra rings will be "
                "ignored.",
                key);
        }
        MUST(JSON_POP(io));
    }

    MUST(JSON_POP(io));
    return OK;
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    MUST(M_LoadRings(io, FX_RING_TYPE_BLAST, "blast"));
    MUST(M_LoadRings(io, FX_RING_TYPE_KNOCKBACK, "knockback"));
    MUST(M_LoadRings(io, FX_RING_TYPE_SUMMON, "summon"));
    return OK;
}

static void M_Control(void)
{
    if (m_Active[FX_RING_TYPE_BLAST]) {
        M_ControlExplosionRings();
    }

    if (m_Active[FX_RING_TYPE_SUMMON]) {
        M_ControlSummonRings();
    }

    if (m_Active[FX_RING_TYPE_KNOCKBACK]) {
        M_ControlKnockBackRings();
    }
}

static void M_Reset(void)
{
    memset(m_Rings, 0, sizeof(m_Rings));
    memset(m_Active, 0, sizeof(m_Active));
}

void FX_Ring_Sync(FX_RING *const ring)
{
    if (ring == nullptr) {
        return;
    }
    M_RememberRing(ring);
}

FX_RING *FX_Ring_GetRing(const FX_RING_TYPE type, const int32_t idx)
{
    if (idx < 0 || idx >= M_MAX_RINGS) {
        return nullptr;
    }
    m_Active[type] = true;
    return &m_Rings[type][idx];
}

FX_RING *FX_Ring_PeekRing(const FX_RING_TYPE type, const int32_t idx)
{
    if (idx < 0 || idx >= M_MAX_RINGS) {
        return nullptr;
    }
    return &m_Rings[type][idx];
}

void FX_Ring_SpawnKnockBack(const XYZ_32 pos)
{
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_KNOCKBACK][i];
        ring->on = 1;
        ring->life = 32;
        ring->speed = ((i == 1) + 1) << 4;
        ring->pos.x = pos.x;
        ring->pos.y = pos.y - 512 + (i << 7);
        ring->pos.z = pos.z;
        ring->rot.x = 0;
        ring->rot.z = 0;
        ring->radius = ((i == 1) + 2) << 8;
        FX_Ring_Sync(ring);
    }
    m_Active[FX_RING_TYPE_KNOCKBACK] = true;
}

void FX_Ring_BounceKnockBack(void)
{
    for (int32_t i = 0; i < M_MAX_RINGS; i++) {
        FX_RING *const ring = &m_Rings[FX_RING_TYPE_KNOCKBACK][i];
        ring->speed = (int16_t)((-ring->speed) >> 2);
    }
}

bool FX_Ring_IsRingActive(const FX_RING_TYPE type)
{
    return m_Active[type];
}

void FX_Ring_Draw(void)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    const int32_t angle_base = (time4 & 0x3F) << 3;
    M_DrawExplosionRings(angle_base);
    M_DrawSummonRings(angle_base);
    M_DrawKnockBackRings(angle_base);
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = FX_Ring_Draw,
    .reset_func = M_Reset,
    .save_key = "rings",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
