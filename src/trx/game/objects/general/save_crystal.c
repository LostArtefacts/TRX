#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/lara/poison.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/stats.h>
#include <trx/version.h>

#define M_PC_MESH 0b00000000'00000001
#define M_PS_MESH 0b00000000'00000010

typedef struct {
    bool initialised;
    bool counted_for_stats;
    bool used_for_save;
    int16_t initial_angle;
} M_PRIV;

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    JSON_SHOULD(JSON_READ(io, "counted_for_stats", &p->counted_for_stats));
    JSON_SHOULD(JSON_READ(io, "used_for_save", &p->used_for_save));
    JSON_SHOULD(JSON_READ(io, "initialised", &p->initialised));
    JSON_SHOULD(JSON_READ(io, "initial_angle", &p->initial_angle));
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "counted_for_stats", p->counted_for_stats);
    JSONW_WRITE(io, "used_for_save", p->used_for_save);
    JSONW_WRITE(io, "initialised", p->initialised);
    JSONW_WRITE(io, "initial_angle", p->initial_angle);
}

static void M_CountCrystal(M_PRIV *const p)
{
    if (p->counted_for_stats) {
        return;
    }
    p->counted_for_stats = true;
    Stats_AddCrystal();
}

static const OBJECT_BOUNDS m_SaveCrystal_Bounds = {
    .shift = {
        .min = { .x = -STEP_L*3/2, .y = -100, .z = -STEP_L*3/2, },
        .max = { .x = +STEP_L*3/2, .y = +WALL_L, .z = +STEP_L*3/2, },
    },
    .rot = {
        .min = { .x = -DEG_45, .y = 0, .z = 0, },
        .max = { .x = +DEG_45, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_UW_Bounds = {
    .shift = {
        .min = { .x = -STEP_L*3/2, .y = -WALL_L, .z = -STEP_L*3/2, },
        .max = { .x = +STEP_L*3/2, .y = +WALL_L, .z = +STEP_L*3/2, },
    },
    .rot = {
        .min = { .x = -DEG_90, .y = 0, .z = 0, },
        .max = { .x = +DEG_90, .y = 0, .z = 0, },
    },
};

static const LARA_TRX_STATE m_StopStates[] = {
    // clang-format off
    LS_STOP,
    LS_TREAD,
    LS_SURF_TREAD,
    LS_TRX_INVALID, // sentinel
    // clang-format on
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    return lara->water_status == LWS_ABOVE_WATER
            || lara->water_status == LWS_WADE
        ? &m_SaveCrystal_Bounds
        : &m_UW_Bounds;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->initialised = false;
    p->counted_for_stats = false;
    p->used_for_save = false;
    p->initial_angle = 0;

    if (g_TRVersion != 3) {
        if (g_Config.gameplay.enable_save_crystals) {
            Item_AddSimulated(item_num);
        } else {
            Item_SetVisible(Item_Get(item_num), false);
        }
    }
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    switch (stage) {
    case SAVEGAME_STAGE_AFTER_LOAD:
        if (item->is_finished) {
            const int16_t item_num = Item_GetIndex(item);
            Item_DetachFromRoom(item_num);
        }
        break;

    case SAVEGAME_STAGE_BEFORE_SAVE:
        M_PRIV *const p = item->priv;
        if (p->used_for_save) {
            // need to reset the crystal status
            Item_SetFinished(item, true);
            p->used_for_save = false;
            const int16_t item_num = Item_GetIndex(item);
            Item_DetachFromRoom(item_num);
        }

    default:
        break;
    }
}

static void M_ControlHeal(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!item->is_visible || item->clear_body) {
        return;
    }

    M_PRIV *const p = item->priv;
    if (!p->initialised) {
        p->initialised = true;
        p->initial_angle = item->pos.y;
    }

    item->rot.y += 1024;
    const int32_t timer = Output_GetTimeInGame();
    const int16_t angle = Math_Cos((timer & 0x3F) << 10);
    int32_t c = ABS(angle >> 9);
    CLAMPG(c, 31);
    c <<= 3;

    item->pos.y = p->initial_angle - ABS(angle >> 6) - 64;

    Output_AddDynamicLightRGB(item->pos, 8, (RGB_888) { 0, c, 0 });

    ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = ABS(item->pos.x - lara_item->pos.x);
    const int32_t dy = ABS(item->pos.y - lara_item->pos.y);
    const int32_t dz = ABS(item->pos.z - lara_item->pos.z);
    if (dx < STEP_L && dy < WALL_L && dz < STEP_L) {
        M_CountCrystal(p);

        Lara_Poison_Cure();
        lara_item->hit_points += LARA_MAX_HITPOINTS / 2;
        CLAMPG(lara_item->hit_points, LARA_MAX_HITPOINTS);

        // PS1: SFX_SAVE_CRYSTAL, PC: SFX_MENU_MEDI
        Sound_Effect(SFX_MENU_MEDI, &lara_item->pos, SPM_NORMAL);

        Item_Destroy(item_num);
    }
}

static void M_ControlSave(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->mesh_bits = g_Config.visuals.enable_ps1_crystals
            && Object_Get(item->object_id)->mesh_count > 1
        ? M_PS_MESH
        : M_PC_MESH;
    Item_Animate(item);
}

static void M_CollisionSave(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Object_Collision(item_num, lara_item, coll);

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS
        || lara_item->gravity) {
        return;
    }

    if (!Lara_HasState(m_StopStates)) {
        return;
    }

    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;
    item->rot.x = 0;
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    if (g_Config.flow.load_save_disabled) {
        Lara_RefuseInteraction();
        return;
    }

    int16_t room_num = lara_item->room_num;
    const XYZ_32 pos = lara_item->pos;
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, pos);
    const int32_t floor = Room_GetHeight(sector, pos);
    if (ceiling >= item->pos.y || floor < item->pos.y) {
        return;
    }

    M_PRIV *const p = item->priv;
    M_CountCrystal(p);
    p->used_for_save = true;
    GF_ShowInventory(INV_SAVE_CRYSTAL_MODE);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->priv_size = sizeof(M_PRIV);
    if (g_TRVersion == 3) {
        obj->control_func = M_ControlHeal;
        obj->collision_func = nullptr;
        obj->save_flags = true;
    } else if (g_Config.gameplay.enable_save_crystals) {
        obj->control_func = M_ControlSave;
        obj->collision_func = M_CollisionSave;
        obj->save_flags = true;
        Object_SetReflective(O_SAVE_CRYSTAL_ITEM, true);
    }
    obj->bounds_func = M_Bounds;
}

REGISTER_OBJECT(O_SAVE_CRYSTAL_ITEM, M_Setup)
