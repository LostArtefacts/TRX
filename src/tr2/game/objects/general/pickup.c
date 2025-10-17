#include "game/objects/general/pickup.h"

#include "game/game.h"
#include "game/inventory.h"
#include "game/objects/common.h"
#include "game/stats.h"

#include <libtrx/config.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/lua/common.h>
#include <libtrx/game/lua/events.h>
#include <libtrx/game/matrix.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/output.h>
#include <libtrx/game/overlay.h>

#define LF_PICKUP_ERASE 42
#define LF_PICKUP_FLARE 58
#define LF_PICKUP_FLARE_UW 20
#define LF_PICKUP_UW 18

static XYZ_32 m_PickupPosition = { .x = 0, .y = 0, .z = -100 };
static XYZ_32 m_PickupPositionUW = { .x = 0, .y = -200, .z = -350 };

static const OBJECT_BOUNDS m_PickUpBounds = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -100, .z = -WALL_L / 4, },
        .max = { .x = +WALL_L / 4, .y = +100, .z = +WALL_L / 4, },
    },
    .rot = {
        .min = { .x = -10 * DEG_1, .y = 0, .z = 0, },
        .max = { .x = +10 * DEG_1, .y = 0, .z = 0, },
    },
};

static const OBJECT_BOUNDS m_PickUpBoundsUW = {
    .shift = {
        .min = { .x = -WALL_L / 2, .y = -WALL_L / 2, .z = -WALL_L / 2, },
        .max = { .x = +WALL_L / 2, .y = +WALL_L / 2, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = -45 * DEG_1, .y = -45 * DEG_1, .z = -45 * DEG_1, },
        .max = { .x = +45 * DEG_1, .y = +45 * DEG_1, .z = +45 * DEG_1, },
    },
};

static void M_DoPickup(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_FLARE_ITEM) {
        return;
    }

    Overlay_AddDisplayPickup(item->object_id);
    Inv_AddPickup(item);
    // Notify Lua pickup listeners
    Lua_FireEvent(LUA_EVENT_PICKUP, item_num); // LUA uses 1-indexing

    item->status = IS_INVISIBLE;
    item->flags |= IF_KILLED;
    Item_RemoveDrawn(item_num);
    Item_RemoveActive(item_num);
}

static void M_DoFlarePickup(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->request_gun_type = LGT_FLARE;
    lara->gun_type = LGT_FLARE;
    Gun_InitialiseNewWeapon();
    lara->gun_status = LGS_SPECIAL;
    lara->flare.age = ((int32_t)(intptr_t)item->data) & 0x7FFF;
    Item_Kill(item_num);
}

static void M_DoAboveWater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS(LS_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_ERASE)) {
            M_DoPickup(item_num);
        }
        goto cleanup;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item->current_anim_state == LS(LS_FLARE_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_FLARE)
            && item->object_id == O_FLARE_ITEM && lara->gun_type != LGT_FLARE) {
            M_DoFlarePickup(item_num);
        }
        goto cleanup;
    }

    if (g_Input.action && !lara_item->gravity
        && lara_item->current_anim_state == LS(LS_STOP)
        && lara->gun_status == LGS_ARMLESS
        && (lara->gun_type != LGT_FLARE || item->object_id != O_FLARE_ITEM)) {
        if (item->object_id == O_FLARE_ITEM) {
            lara_item->goal_anim_state = LS(LS_FLARE_PICKUP);
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS(LS_FLARE_PICKUP));
            lara_item->goal_anim_state = LS(LS_STOP);
            lara->gun_status = LGS_HANDS_BUSY;
        } else {
            Lara_AlignPosition(item, &m_PickupPosition);
            lara_item->goal_anim_state = LS(LS_PICKUP);
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS(LS_PICKUP));
            lara_item->goal_anim_state = LS(LS_STOP);
            lara->gun_status = LGS_HANDS_BUSY;
        }
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_DoUnderwater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = -25 * DEG_1;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS(LS_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_UW)) {
            M_DoPickup(item_num);
        }
        goto cleanup;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item->current_anim_state == LS(LS_FLARE_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_FLARE_UW)
            && item->object_id == O_FLARE_ITEM && lara->gun_type != LGT_FLARE) {
            M_DoFlarePickup(item_num);
            Lara_Flare_DrawMeshes();
        }
        goto cleanup;
    }

    if (g_Input.action && lara_item->current_anim_state == LS(LS_TREAD)
        && lara->gun_status == LGS_ARMLESS
        && (lara->gun_type != LGT_FLARE || item->object_id != O_FLARE_ITEM)) {
        if (!Lara_MovePosition(item, &m_PickupPositionUW)) {
            goto cleanup;
        }

        if (item->object_id == O_FLARE_ITEM) {
            lara_item->fall_speed = 0;
            Item_SwitchToAnim(lara_item, LA(LA_UNDERWATER_FLARE_PICKUP), 0);
            lara_item->goal_anim_state = LS(LS_TREAD);
            lara_item->current_anim_state = LS(LS_FLARE_PICKUP);
        } else {
            if (g_Config.gameplay.fix_lara_pickup_embed) {
                lara_item->fall_speed = 0;
            }
            lara_item->goal_anim_state = LS(LS_PICKUP);
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS(LS_PICKUP));
            lara_item->goal_anim_state = LS(LS_TREAD);
        }
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if (item->status == IS_DEACTIVATED) {
            const int16_t item_num = Item_GetIndex(item);
            Item_RemoveDrawn(item_num);
        }
    }
}

static void M_Activate(ITEM *const item)
{
    if (item->status == IS_INVISIBLE) {
        item->touch_bits = 0;
        item->status = IS_ACTIVE;
        const int16_t item_num = Item_GetIndex(item);
        Item_AddActive(item_num);
    } else {
        item->status = IS_INVISIBLE;
        item->flags |= IF_KILLED;
    }
}

static void M_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status == IS_INVISIBLE || item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
    }
}

const OBJECT_BOUNDS *Pickup_Bounds(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        return &m_PickUpBoundsUW;
    } else {
        return &m_PickUpBounds;
    }
}

void Pickup_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    if (item->flags & IF_INVISIBLE) {
        return;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_ABOVE_WATER
        || lara->water_status == LWS_WADE) {
        M_DoAboveWater(item_num, lara_item);
    } else if (
        lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        M_DoUnderwater(item_num, lara_item);
    }
}

bool Pickup_Trigger(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INVISIBLE) {
        return false;
    }

    item->status = IS_DEACTIVATED;
    return true;
}

static void M_Setup(OBJECT *const obj)
{
    obj->handle_save_func = M_HandleSave;
    obj->activate_func = M_Activate;
    obj->control_func = M_Control;
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->draw_func = Object_DrawPickupItem;
    obj->save_position = true;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_EXPLOSIVE_ITEM, M_Setup)
REGISTER_OBJECT(O_FLARES_ITEM, M_Setup)
REGISTER_OBJECT(O_GRENADE_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_GRENADE_ITEM, M_Setup)
REGISTER_OBJECT(O_HARPOON_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_HARPOON_ITEM, M_Setup)
REGISTER_OBJECT(O_KEY_ITEM_1, M_Setup)
REGISTER_OBJECT(O_KEY_ITEM_2, M_Setup)
REGISTER_OBJECT(O_KEY_ITEM_3, M_Setup)
REGISTER_OBJECT(O_KEY_ITEM_4, M_Setup)
REGISTER_OBJECT(O_LARGE_MEDIPACK_ITEM, M_Setup)
REGISTER_OBJECT(O_LEADBAR_ITEM, M_Setup)
REGISTER_OBJECT(O_M16_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_M16_ITEM, M_Setup)
REGISTER_OBJECT(O_MAGNUM_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_MAGNUM_ITEM, M_Setup)
REGISTER_OBJECT(O_PICKUP_ITEM_1, M_Setup)
REGISTER_OBJECT(O_PICKUP_ITEM_2, M_Setup)
REGISTER_OBJECT(O_PISTOL_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_PISTOL_ITEM, M_Setup)
REGISTER_OBJECT(O_PUZZLE_ITEM_1, M_Setup)
REGISTER_OBJECT(O_PUZZLE_ITEM_2, M_Setup)
REGISTER_OBJECT(O_PUZZLE_ITEM_3, M_Setup)
REGISTER_OBJECT(O_PUZZLE_ITEM_4, M_Setup)
REGISTER_OBJECT(O_SCION_ITEM_2, M_Setup)
REGISTER_OBJECT(O_SECRET_1, M_Setup)
REGISTER_OBJECT(O_SECRET_2, M_Setup)
REGISTER_OBJECT(O_SECRET_3, M_Setup)
REGISTER_OBJECT(O_SHOTGUN_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_SHOTGUN_ITEM, M_Setup)
REGISTER_OBJECT(O_SMALL_MEDIPACK_ITEM, M_Setup)
REGISTER_OBJECT(O_UZI_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_UZI_ITEM, M_Setup)
