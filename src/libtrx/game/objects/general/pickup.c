#include "game/objects/general/pickup.h"

#include "config.h"
#include "game/effects.h"
#include "game/gun.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/lua.h"
#include "game/overlay.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/savegame.h"

// clang-format off
#define M_LF_PICKUP_ERASE       42
#define M_LF_PICKUP_FLARE       58
#define M_LF_PICKUP_FLARE_UW    20
#define M_LF_PICKUP_UW          18
#define M_AID_DIST_MIN          (STEP_L * 5)      // 1280
#define M_AID_DIST_MAX          (WALL_L * 8)      // 8192
#define M_AID_WAIT_MIN          (LOGIC_FPS * 2.5) // 75
#define M_AID_WAIT_MAX          (LOGIC_FPS * 5)   // 150
#define M_AID_WAIT_BREAK_CHANCE 0x1200
// clang-format on

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

static const OBJECT_BOUNDS m_PickUpBoundsControlled = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -200, .z = -WALL_L / 4, },
        .max = { .x = +WALL_L / 4, .y = +200, .z = +WALL_L / 4, },
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

static const XYZ_32 m_PickupPosition = { .x = 0, .y = 0, .z = -100 };
static const XYZ_32 m_PickupPositionUW = { .x = 0, .y = -200, .z = -350 };

extern void Stats_AddPickup(void);

static void M_Initialise(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->priv = (void *)(intptr_t)(-1);
    if (item->status != IS_INVISIBLE) {
        Item_AddActive(item_num);
    }
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

static void M_SpawnPickupAid(const ITEM *const item)
{
    const OBJECT_ID obj_id =
        Object_GetCognate(item->object_id, g_ItemToInvObjectMap);
    if (obj_id == NO_OBJECT) {
        return;
    }

    const OBJECT *const obj = Object_Get(obj_id);
    const ANIM_FRAME *const frame = obj->frame_base;

    const GAME_VECTOR pos = {
        .x = item->pos.x + 20 * (Random_GetDraw() - 0x4000) / 0x4000,
        .y = item->pos.y - ABS(frame->bounds.max.y - frame->bounds.min.y)
            - 10 * (1 + (Random_GetDraw() - 0x4000) / 0x4000),
        .z = item->pos.z + 20 * (Random_GetDraw() - 0x4000) / 0x4000,
        .room_num = item->room_num,
    };

    const int16_t effect_num = Effect_Create(pos.room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->room_num = pos.room_num;
        effect->pos = pos.pos;
        effect->counter = 0;
        effect->object_id = O_PICKUP_AID;
        effect->frame_num = 0;
    }
}

static void M_Control(int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status == IS_INVISIBLE || item->status == IS_DEACTIVATED) {
        Item_RemoveActive(item_num);
        return;
    }

    if (!g_Config.gameplay.enable_pickup_aids || item->room_num == NO_ROOM) {
        return;
    }

    const ITEM *const lara = Lara_GetItem();
    if (item->fall_speed != 0 || lara == nullptr
        || !Object_Get(O_PICKUP_AID)->loaded) {
        return;
    }

    const int32_t distance = Item_GetDistance(lara, &item->pos);
    if (distance < M_AID_DIST_MIN || distance > M_AID_DIST_MAX) {
        return;
    }

    int32_t timer = (int32_t)(intptr_t)item->priv;
    if (timer <= 0
        || (timer < M_AID_WAIT_MIN
            && Random_GetDraw() < M_AID_WAIT_BREAK_CHANCE)) {
        M_SpawnPickupAid(item);
        timer = M_AID_WAIT_MAX;
    } else {
        timer--;
    }

    item->priv = (void *)(intptr_t)(int32_t)timer;
}

static void M_DoPickup(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_FLARE_ITEM) {
        return;
    }

    Overlay_AddDisplayPickup(item->object_id);
    Inv_AddPickup(item);
#if TR_VERSION == 1
    Stats_AddPickup();
#endif
    // Notify Lua pickup listeners
    Lua_FireEvent(LUA_EVENT_PICKUP, item_num); // LUA uses 1-indexing

    item->status = IS_INVISIBLE;
    item->flags |= IF_KILLED;
    Item_RemoveDrawn(item_num);
    Item_RemoveActive(item_num);

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->interact_target.is_moving = false;
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
    lara->interact_target.is_moving = false;
}

static void M_GetAllAtLaraPos(const ITEM *const item)
{
    int16_t pickup_num = Room_Get(item->room_num)->item_num;
    while (pickup_num != NO_ITEM) {
        ITEM *const check_item = Item_Get(pickup_num);
        if (check_item->pos.x == item->pos.x && check_item->pos.z == item->pos.z
            && Object_Get(check_item->object_id)->collision_func
                == Pickup_Collision) {
            M_DoPickup(pickup_num);
        }
        pickup_num = check_item->next_item;
    }
}

static void M_DoControlled(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if ((g_Input.action && lara->gun_status == LGS_ARMLESS
         && !lara_item->gravity && lara_item->current_anim_state == LS(LS_STOP)
         && !lara->interact_target.is_moving)
        || (lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num)) {
        const OBJECT *const obj = Object_Get(item->object_id);
        if (Lara_TestPosition(item, obj->bounds_func())) {
            const XYZ_32 pos = {
                .x = m_PickupPosition.x,
                .y = lara_item->pos.y - item->pos.y,
                .z = m_PickupPosition.z,
            };
            if (Lara_MovePosition(item, &pos)) {
                Item_SwitchToAnim(lara_item, LA(LA_PICKUP), 0);
                lara_item->current_anim_state = LS(LS_PICKUP);
                lara->head_rot.y = 0;
                lara->head_rot.x = 0;
                lara->torso_rot.y = 0;
                lara->torso_rot.x = 0;
                lara->interact_target.is_moving = false;
                lara->gun_status = LGS_HANDS_BUSY;
            }
            lara->interact_target.item_num = item_num;
        } else if (
            lara->interact_target.is_moving
            && lara->interact_target.item_num == item_num) {
            lara->interact_target.is_moving = false;
            lara->interact_target.item_num = NO_ITEM;
            lara->gun_status = LGS_ARMLESS;
        }

        goto cleanup;
    }

    if (lara->interact_target.item_num != item_num) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS(LS_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, M_LF_PICKUP_ERASE)) {
            M_GetAllAtLaraPos(item);
        }
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_DoAboveWater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    if (g_Config.gameplay.enable_walk_to_items
        && item->object_id != O_FLARE_ITEM) {
        M_DoControlled(item_num, lara_item);
        return;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Lara_TestPosition(item, obj->bounds_func())) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS(LS_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, M_LF_PICKUP_ERASE)) {
            M_DoPickup(item_num);
        }
        goto cleanup;
    }

    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item->current_anim_state == LS(LS_FLARE_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, M_LF_PICKUP_FLARE)
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
            Lara_AnimateUntil(lara_item, LS(LS_FLARE_PICKUP));
        } else {
            Lara_AlignPosition(item, &m_PickupPosition);
            Lara_AnimateUntil(lara_item, LS(LS_PICKUP));
        }
        lara_item->goal_anim_state = LS(LS_STOP);
        lara->gun_status = LGS_HANDS_BUSY;
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
        if (Item_TestFrameEqual(lara_item, M_LF_PICKUP_UW)) {
            M_DoPickup(item_num);
        }
        goto cleanup;
    }

    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara_item->current_anim_state == LS(LS_FLARE_PICKUP)) {
        if (Item_TestFrameEqual(lara_item, M_LF_PICKUP_FLARE_UW)
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
            lara_item->current_anim_state = LS(LS_FLARE_PICKUP);
        } else {
            if (g_Config.gameplay.fix_lara_pickup_embed) {
                lara_item->fall_speed = 0;
            }
            Lara_AnimateUntil(lara_item, LS(LS_PICKUP));
        }
        lara_item->goal_anim_state = LS(LS_TREAD);
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_Setup(OBJECT *const obj)
{
    obj->activate_func = M_Activate;
    obj->control_func = M_Control;
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->draw_func = Object_DrawPickupItem;
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->save_position = true;
    obj->save_flags = true;
}

const OBJECT_BOUNDS *Pickup_Bounds(void)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->water_status == LWS_UNDERWATER
        || lara->water_status == LWS_CHEAT) {
        return &m_PickUpBoundsUW;
    } else if (g_Config.gameplay.enable_walk_to_items) {
        return &m_PickUpBoundsControlled;
    } else {
        return &m_PickUpBounds;
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

void Pickup_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_INVISIBLE) != 0) {
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
