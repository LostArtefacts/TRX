#include "game/game_flow.h"
#include "game/objects/common.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>

static const OBJECT_BOUNDS m_SaveCrystal_Bounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = -100, .z = -STEP_L, },
        .max = { .x = +STEP_L, .y = +WALL_L, .z = +STEP_L, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_UW_Bounds = {
    .shift = {
        .min = { .x = -STEP_L, .y = -WALL_L, .z = -STEP_L, },
        .max = { .x = +STEP_L, .y = +WALL_L, .z = +STEP_L, },
    },
    .rot = {
        .min = { .x = -DEG_90, .y = 0, .z = 0, },
        .max = { .x = +DEG_90, .y = 0, .z = 0, },
    },
};

static const LARA_TRX_STATE m_StopStates[] = {
    LS_STOP, LS_TREAD, LS_SURF_TREAD,
    LS_TRX_INVALID, // sentinel
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
    if (g_Config.gameplay.enable_save_crystals) {
        Item_AddActive(item_num);
    } else {
        Item_Get(item_num)->status = IS_INVISIBLE;
    }
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    switch (stage) {
    case SAVEGAME_STAGE_AFTER_LOAD:
        if (item->status == IS_DEACTIVATED) {
            const int16_t item_num = Item_GetIndex(item);
            Item_RemoveDrawn(item_num);
        }
        break;

    case SAVEGAME_STAGE_BEFORE_SAVE:
        if (item->data != nullptr) {
            // need to reset the crystal status
            item->status = IS_DEACTIVATED;
            item->data = nullptr;
            const int16_t item_num = Item_GetIndex(item);
            Item_RemoveDrawn(item_num);
        }

    default:
        break;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_Animate(item);
}

static void M_Collision(
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

    int16_t room_num = lara_item->room_num;
    const XYZ_32 pos = lara_item->pos;
    const SECTOR *const sector = Room_GetSector(pos.x, pos.y, pos.z, &room_num);
    const int32_t ceiling = Room_GetCeiling(sector, pos.x, pos.y, pos.z);
    const int32_t floor = Room_GetHeight(sector, pos.x, pos.y, pos.z);
    if (ceiling >= item->pos.y || floor < item->pos.y) {
        return;
    }

    item->data = (void *)1;
    GF_ShowInventory(INV_SAVE_CRYSTAL_MODE);
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    if (g_Config.gameplay.enable_save_crystals) {
        obj->handle_save_func = M_HandleSave;
        obj->control_func = M_Control;
        obj->collision_func = M_Collision;
        obj->save_flags = true;
    }
    obj->bounds_func = M_Bounds;
    Object_SetReflective(O_SAVEGAME_ITEM, true);
}

REGISTER_OBJECT(O_SAVEGAME_ITEM, M_Setup)
