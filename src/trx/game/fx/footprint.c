#include <trx/game/fx/footprint.h>

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/collision.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

#include <stdlib.h>
#include <string.h>

#define M_MAX_FOOTPRINTS 32
#define M_FOOTPRINT_LIFETIME 512
#define M_FOOTPRINT_Z_DEPTH_ADJUST -0.5f

typedef struct {
    FX_FOOTPRINT prints[M_MAX_FOOTPRINTS];
    int32_t next_idx;
} M_PRIV;

static M_PRIV m_Priv;
static const SAMPLE_TRX_ID m_StepSounds[14] = {
    SFX_FOOTSTEPS_MUD,
    SFX_FOOTSTEPS_SNOW,
    SFX_FOOTSTEPS_SAND_OR_GRASS,
    SFX_FOOTSTEPS_GRAVEL,
    SFX_FOOTSTEPS_ICE,
    SFX_TRX_INVALID,
    SFX_TRX_INVALID,
    SFX_FOOTSTEPS_WOOD,
    SFX_FOOTSTEPS_METAL,
    SFX_TRX_INVALID,
    SFX_FOOTSTEPS_SAND_OR_GRASS,
    SFX_TRX_INVALID,
    SFX_FOOTSTEPS_WOOD,
    SFX_FOOTSTEPS_METAL,
};

void FX_Footprint_Reset(void)
{
    M_PRIV *const p = &m_Priv;
    memset(p, 0, sizeof(*p));
}

void FX_Footprint_Add(const ITEM *const lara_item, const bool is_left_foot)
{
    M_PRIV *const p = &m_Priv;
    if (lara_item == nullptr) {
        return;
    }

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(
        lara_item, &pos, is_left_foot ? LM_FOOT_L : LM_FOOT_R);

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    if (sector == nullptr) {
        return;
    }

    if (sector->fx < ARRAY_SIZE(m_StepSounds)
        && m_StepSounds[sector->fx] != SFX_TRX_INVALID) {
        Sound_Effect(m_StepSounds[sector->fx], &lara_item->pos, SPM_NORMAL);
    }

    if (sector->fx > 4) {
        return;
    }

    const int32_t y = Room_GetHeight(sector, pos);
    if (y == NO_HEIGHT || Room_IsOnWalkable(sector, pos, y, NO_ITEM)) {
        return;
    }

    FX_FOOTPRINT *const print = &m_Priv.prints[m_Priv.next_idx];
    print->pos.x = pos.x;
    print->pos.y = y;
    print->pos.z = pos.z;
    print->room_num = room_num;
    print->y_rot = lara_item->rot.y;
    print->life = M_FOOTPRINT_LIFETIME;
    p->next_idx = (p->next_idx + 1) % M_MAX_FOOTPRINTS;
}

static void M_GetWorldPoint(
    const FX_FOOTPRINT *const print, const XYZ_32 local,
    XYZ_32 *const out_world)
{
    const int32_t s = Math_Sin(print->y_rot);
    const int32_t c = Math_Cos(print->y_rot);
    const int32_t dx = TRIGMULT2(local.x, c) + TRIGMULT2(local.z, s);
    const int32_t dz = TRIGMULT2(local.z, c) - TRIGMULT2(local.x, s);
    out_world->x = print->pos.x + dx;
    out_world->y = print->pos.y + local.y;
    out_world->z = print->pos.z + dz;
}

static int32_t M_GetVertexYOffset(
    const FX_FOOTPRINT *const print, const XYZ_32 world_pos)
{
    int16_t room_num = print->room_num;
    const XYZ_32 pos = { world_pos.x, print->pos.y, world_pos.z };
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    if (sector == nullptr) {
        return 0;
    }

    const int32_t height = Room_GetHeight(sector, pos);
    if (height == NO_HEIGHT) {
        return 0;
    }

    int32_t dy = height - print->pos.y;
    if (ABS(dy) > 128) {
        dy = 0;
    }
    return dy;
}

void FX_Footprint_Control(void)
{
    if (!g_Config.visuals.enable_footprints) {
        return;
    }
    M_PRIV *const p = &m_Priv;
    for (int32_t i = 0; i < M_MAX_FOOTPRINTS; i++) {
        FX_FOOTPRINT *const print = &p->prints[i];
        if (print->life != 0) {
            print->life--;
        }
    }
}

void FX_Footprint_Draw(void)
{
    if (!g_Config.visuals.enable_footprints) {
        return;
    }

    const M_PRIV *const p = &m_Priv;
    const int32_t sprite_idx = Sparks_GetSpriteIndex(SPARK_TYPE_FOOTPRINT);
    if (sprite_idx == NO_ITEM) {
        return;
    }

    const XYZ_32 corners[3] = {
        { .x = 0, .y = 0, .z = -64 },
        { .x = -128, .y = 0, .z = 64 },
        { .x = 128, .y = 0, .z = 64 },
    };

    for (int32_t i = 0; i < M_MAX_FOOTPRINTS; i++) {
        const FX_FOOTPRINT *const print = &p->prints[i];
        if (print->life == 0) {
            continue;
        }

        XYZ_32 world[3] = {};
        for (int32_t j = 0; j < 3; j++) {
            M_GetWorldPoint(print, corners[j], &world[j]);
            world[j].y += M_GetVertexYOffset(print, world[j]);
        }

        int32_t c = print->life < 29 ? (print->life << 2) : 112;
        CLAMP(c, 0, 255);

        const RGBA_8888 color = { c, c, c, 255 };
        const RGBA_8888 tri_color[3] = { color, color, color };

        for (int32_t j = 0; j < 4; j++) {
            OutputSource_PolyFX_StageSpriteTriWorldDepth(
                sprite_idx, world, tri_color, M_FOOTPRINT_Z_DEPTH_ADJUST,
                DRAW_BLEND_SUB);
        }
    }
}

bool FX_Footprint_HasActivePrints(void)
{
    for (int32_t i = 0; i < M_MAX_FOOTPRINTS; i++) {
        if (m_Priv.prints[i].life != 0) {
            return true;
        }
    }
    return false;
}

FX_FOOTPRINT *FX_Footprint_GetPrint(const int32_t idx)
{
    if (idx < 0 || idx >= M_MAX_FOOTPRINTS) {
        return nullptr;
    }
    return &m_Priv.prints[idx];
}
