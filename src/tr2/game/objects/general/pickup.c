#include "game/objects/general/pickup.h"

#include "decomp/flares.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/gun/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/inventory_ring.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/misc.h"
#include "game/objects/common.h"
#include "game/output.h"
#include "game/overlay.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/matrix.h>

#define LF_PICKUP_ERASE 42
#define LF_PICKUP_FLARE 58
#define LF_PICKUP_FLARE_UW 20
#define LF_PICKUP_UW 18

int16_t g_PickupBounds[12] = {
    // clang-format off
    -WALL_L / 4,
    +WALL_L / 4,
    -100,
    +100,
    -WALL_L / 4,
    +WALL_L / 4,
    -10 * DEG_1,
    +10 * DEG_1,
    +0,
    +0,
    +0,
    +0,
    // clang-format on
};

static XYZ_32 m_PickupPosition = { .x = 0, .y = 0, .z = -100 };
static XYZ_32 m_PickupPositionUW = { .x = 0, .y = -200, .z = -350 };

static int16_t m_PickupBoundsUW[12] = {
    // clang-format off
    -WALL_L / 2,
    +WALL_L / 2,
    -WALL_L / 2,
    +WALL_L / 2,
    -WALL_L / 2,
    +WALL_L / 2,
    -45 * DEG_1,
    +45 * DEG_1,
    -45 * DEG_1,
    +45 * DEG_1,
    -45 * DEG_1,
    +45 * DEG_1,
    // clang-format on
};

static void M_DoPickup(int16_t item_num);
static void M_DoFlarePickup(int16_t item_num);
static void M_DoAboveWater(int16_t item, ITEM *lara_item);
static void M_DoUnderwater(int16_t item, ITEM *lara_item);
static void M_Setup(OBJECT *obj);
static void M_HandleSave(ITEM *item, SAVEGAME_STAGE stage);
static void M_Activate(ITEM *item);
static void M_Draw(const ITEM *item);

static void M_DoPickup(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->object_id == O_FLARE_ITEM) {
        return;
    }

    Overlay_AddDisplayPickup(item->object_id);
    Inv_AddItem(item->object_id);

    if ((item->object_id == O_SECRET_1 || item->object_id == O_SECRET_2
         || item->object_id == O_SECRET_3)
        && (g_SaveGame.current_stats.secret_flags & 1)
                + ((g_SaveGame.current_stats.secret_flags >> 1) & 1)
                + ((g_SaveGame.current_stats.secret_flags >> 2) & 1)
            >= 3) {
        GF_InventoryModifier_Apply(Game_GetCurrentLevel(), GF_INV_SECRET);
    }

    item->status = IS_INVISIBLE;
    item->flags |= IF_KILLED;
    Item_RemoveDrawn(item_num);
    Item_RemoveActive(item_num);
}

static void M_DoFlarePickup(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    g_Lara.request_gun_type = LGT_FLARE;
    g_Lara.gun_type = LGT_FLARE;
    Gun_InitialiseNewWeapon();
    g_Lara.gun_status = LGS_SPECIAL;
    g_Lara.flare_age = ((int32_t)(intptr_t)item->data) & 0x7FFF;
    Item_Kill(item_num);
}

static void M_DoAboveWater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = 0;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Item_TestPosition(g_PickupBounds, item, lara_item)) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_ERASE)) {
            M_DoPickup(item_num);
        }
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS_FLARE_PICKUP) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_FLARE)
            && item->object_id == O_FLARE_ITEM
            && g_Lara.gun_type != LGT_FLARE) {
            M_DoFlarePickup(item_num);
        }
        goto cleanup;
    }

    if (g_Input.action && !lara_item->gravity
        && lara_item->current_anim_state == LS_STOP
        && g_Lara.gun_status == LGS_ARMLESS
        && (g_Lara.gun_type != LGT_FLARE || item->object_id != O_FLARE_ITEM)) {
        if (item->object_id == O_FLARE_ITEM) {
            lara_item->goal_anim_state = LS_FLARE_PICKUP;
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS_FLARE_PICKUP);
            lara_item->goal_anim_state = LS_STOP;
            g_Lara.gun_status = LGS_HANDS_BUSY;
        } else {
            Item_AlignPosition(&m_PickupPosition, item, lara_item);
            lara_item->goal_anim_state = LS_PICKUP;
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS_PICKUP);
            lara_item->goal_anim_state = LS_STOP;
            g_Lara.gun_status = LGS_HANDS_BUSY;
        }
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_DoUnderwater(const int16_t item_num, ITEM *const lara_item)
{
    ITEM *const item = Item_Get(item_num);
    const XYZ_16 old_rot = item->rot;

    item->rot.x = -25 * DEG_1;
    item->rot.y = lara_item->rot.y;
    item->rot.z = 0;

    if (!Item_TestPosition(m_PickupBoundsUW, item, lara_item)) {
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS_PICKUP) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_UW)) {
            M_DoPickup(item_num);
        }
        goto cleanup;
    }

    if (lara_item->current_anim_state == LS_FLARE_PICKUP) {
        if (Item_TestFrameEqual(lara_item, LF_PICKUP_FLARE_UW)
            && item->object_id == O_FLARE_ITEM
            && g_Lara.gun_type != LGT_FLARE) {
            M_DoFlarePickup(item_num);
            Flare_DrawMeshes();
        }
        goto cleanup;
    }

    if (g_Input.action && lara_item->current_anim_state == LS_TREAD
        && g_Lara.gun_status == LGS_ARMLESS
        && (g_Lara.gun_type != LGT_FLARE || item->object_id != O_FLARE_ITEM)) {
        if (!Lara_MovePosition(&m_PickupPositionUW, item, lara_item)) {
            goto cleanup;
        }

        if (item->object_id == O_FLARE_ITEM) {
            lara_item->fall_speed = 0;
            Item_SwitchToAnim(lara_item, LA_UNDERWATER_FLARE_PICKUP, 0);
            lara_item->goal_anim_state = LS_TREAD;
            lara_item->current_anim_state = LS_FLARE_PICKUP;
        } else {
            if (g_Config.gameplay.fix_pickup_drift_glitch) {
                lara_item->fall_speed = 0;
            }
            lara_item->goal_anim_state = LS_PICKUP;
            do {
                Lara_Animate(lara_item);
            } while (lara_item->current_anim_state != LS_PICKUP);
            lara_item->goal_anim_state = LS_TREAD;
        }
        goto cleanup;
    }

cleanup:
    item->rot = old_rot;
}

static void M_Setup(OBJECT *const obj)
{
    obj->handle_save_func = M_HandleSave;
    obj->activate_func = M_Activate;
    obj->collision_func = Pickup_Collision;
    obj->draw_func = M_Draw;
    obj->save_position = 1;
    obj->save_flags = 1;
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

static void M_Draw(const ITEM *const item)
{
    if (item->flags & IF_INVISIBLE) {
        return;
    }

    if (!g_Config.visuals.enable_3d_pickups) {
        Object_DrawSpriteItem(item);
        return;
    }

    if (!Object_Get(item->object_id)->loaded) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Convert item to menu display item.
    const GAME_OBJECT_ID inv_obj_id = Inv_GetItemOption(item->object_id);
    const OBJECT *const obj = Object_Get(inv_obj_id);
    if (!obj->loaded || obj->mesh_count < 0) {
        Object_DrawSpriteItem(item);
        return;
    }

    // Get the first frame of the first animation, and its bounding box.
    int16_t offset;
    BOUNDS_16 bounds;
    const ANIM_FRAME *frame = nullptr;

    // Some items, such as the Prayer Wheel in Barkhang Monastery, do not have
    // animations, and for such items we need to calculate this information
    // manually.
    if (obj->anim_idx != -1) {
        frame = obj->frame_base;
        bounds = frame->bounds;
        const int16_t y_off = frame->offset.y - bounds.max.y;
        bounds.max.y -= bounds.max.y;
        bounds.min.y -= bounds.max.y;
        offset = item->pos.y + y_off;
    } else {
        bounds = Object_GetBoundingBox(obj, nullptr, item->mesh_bits);
        offset = item->pos.y - (bounds.max.y - bounds.min.y) / 2;
    }

    Matrix_Push();
    Matrix_TranslateAbs(item->pos.x, offset, item->pos.z);
    Matrix_Rot16(item->rot);

    Output_CalculateLight(item->pos, item->room_num);

    const int32_t clip = Output_GetObjectBounds(&bounds);
    if (clip) {
        int32_t bit = 1;

        const XYZ_16 *const mesh_rots =
            frame != nullptr ? frame->mesh_rots : nullptr;
        if (mesh_rots != nullptr) {
            Matrix_Rot16(mesh_rots[0]);
        }

        if (item->mesh_bits & bit) {
            Object_DrawMesh(obj->mesh_idx, clip, false);
        }

        for (int i = 1; i < obj->mesh_count; i++) {
            const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
            if (bone->matrix_pop) {
                Matrix_Pop();
            }

            if (bone->matrix_push) {
                Matrix_Push();
            }

            Matrix_TranslateRel32(bone->pos);
            if (mesh_rots != nullptr) {
                Matrix_Rot16(mesh_rots[i]);
            }

            // Extra rotation is ignored in this case as it's not needed.

            bit <<= 1;
            if (item->mesh_bits & bit) {
                Object_DrawMesh(obj->mesh_idx + i, clip, false);
            }
        }
    }

    Matrix_Pop();
}

void Pickup_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    if (item->flags & IF_INVISIBLE) {
        return;
    }

    if (g_Lara.water_status == LWS_ABOVE_WATER
        || g_Lara.water_status == LWS_WADE) {
        M_DoAboveWater(item_num, lara_item);
    } else if (g_Lara.water_status == LWS_UNDERWATER) {
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
REGISTER_OBJECT(O_SECRET_1, M_Setup)
REGISTER_OBJECT(O_SECRET_2, M_Setup)
REGISTER_OBJECT(O_SECRET_3, M_Setup)
REGISTER_OBJECT(O_SHOTGUN_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_SHOTGUN_ITEM, M_Setup)
REGISTER_OBJECT(O_SMALL_MEDIPACK_ITEM, M_Setup)
REGISTER_OBJECT(O_UZI_AMMO_ITEM, M_Setup)
REGISTER_OBJECT(O_UZI_ITEM, M_Setup)
