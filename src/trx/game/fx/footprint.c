#include <trx/game/fx/footprint.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/collision.h>
#include <trx/game/fx/common.h>
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
    XYZ_32 pos;
    int16_t room_num;
    int16_t y_rot;
    int16_t life;
} M_FOOTPRINT;

typedef struct {
    M_FOOTPRINT prints[M_MAX_FOOTPRINTS];
    int32_t next_idx;
} M_PRIV;

static M_PRIV m_Priv;
static const SAMPLE_ID m_StepSounds[14] = {
    SFX_FOOTSTEPS_MUD,
    SFX_FOOTSTEPS_SNOW,
    SFX_FOOTSTEPS_SAND_OR_GRASS,
    SFX_FOOTSTEPS_GRAVEL,
    SFX_FOOTSTEPS_ICE,
    NO_CATALOG_ID,
    NO_CATALOG_ID,
    SFX_FOOTSTEPS_WOOD,
    SFX_FOOTSTEPS_METAL,
    NO_CATALOG_ID,
    SFX_FOOTSTEPS_SAND_OR_GRASS,
    NO_CATALOG_ID,
    SFX_FOOTSTEPS_WOOD,
    SFX_FOOTSTEPS_METAL,
};

static void M_GetWorldPoint(
    const M_FOOTPRINT *const print, const XYZ_32 local, XYZ_32 *const out_world)
{
    *out_world = XYZ_32_OffsetLocalYaw(print->pos, local, print->y_rot);
}

static int32_t M_GetVertexYOffset(
    const M_FOOTPRINT *const print, const XYZ_32 world_pos)
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

static void M_Save(JSON_WRITE_IO *const io)
{
    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_MAX_FOOTPRINTS; i++) {
        const M_FOOTPRINT *const print = &m_Priv.prints[i];
        if (print->life == 0) {
            continue;
        }
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", print->pos);
        JSONW_WRITE(io, "room_num", print->room_num);
        JSONW_WRITE(io, "y_rot", print->y_rot);
        JSONW_WRITE(io, "life", print->life);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET_NZ(io, "prints");
}

static RESULT M_Load(JSON_READ_IO *const io)
{
    if (!JSON_ReadIO_HasKey(io, "prints")) {
        return OK;
    }
    MUST(JSON_PUSH(io, "prints"));

    const int32_t count = JSON_ARRAY_LEN(io);
    for (int32_t i = 0; i < count; i++) {
        if (i >= M_MAX_FOOTPRINTS) {
            LOG_WARNING(
                "Malformed save: too many footprints. Extra footprints will "
                "be ignored.");
            break;
        }

        M_FOOTPRINT *const print = &m_Priv.prints[i];
        MUST(JSON_PUSH_INDEX(io, i));
        MUST(JSON_READ(io, "pos", &print->pos));
        MUST(JSON_READ(io, "room_num", &print->room_num));
        MUST(JSON_READ(io, "y_rot", &print->y_rot));
        MUST(JSON_READ(io, "life", &print->life));
        MUST(JSON_POP(io));
        m_Priv.next_idx = (i + 1) % M_MAX_FOOTPRINTS;
    }

    MUST(JSON_POP(io));
    return OK;
}

static void M_Control(void)
{
    if (!g_Config.visuals.enable_footprints) {
        return;
    }
    M_PRIV *const p = &m_Priv;
    for (int32_t i = 0; i < M_MAX_FOOTPRINTS; i++) {
        M_FOOTPRINT *const print = &p->prints[i];
        if (print->life != 0) {
            print->life--;
        }
    }
}

static void M_Draw(void)
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
        const M_FOOTPRINT *const print = &p->prints[i];
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

static void M_Reset(void)
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
        && m_StepSounds[sector->fx] != NO_CATALOG_ID) {
        Sound_Effect(m_StepSounds[sector->fx], &lara_item->pos, SPM_NORMAL);
    }

    if (sector->fx > 4) {
        return;
    }

    const int32_t y = Room_GetHeight(sector, pos);
    if (y == NO_HEIGHT || Room_IsOnWalkable(sector, pos, y, NO_ITEM)) {
        return;
    }

    M_FOOTPRINT *const print = &m_Priv.prints[m_Priv.next_idx];
    print->pos.x = pos.x;
    print->pos.y = y;
    print->pos.z = pos.z;
    print->room_num = room_num;
    print->y_rot = lara_item->rot.y;
    print->life = M_FOOTPRINT_LIFETIME;
    p->next_idx = (p->next_idx + 1) % M_MAX_FOOTPRINTS;
}

static const FX_MODULE m_Module = {
    .control_func = M_Control,
    .draw_func = M_Draw,
    .reset_func = M_Reset,
    .save_key = "footprints",
    .save_func = M_Save,
    .load_func = M_Load,
};

REGISTER_FX(m_Module)
