#include "game/objects/general/window.h"

#include "game/lara.h"
#include "game/math.h"
#include "game/objects/common.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "game/sound.h"
#include "utils.h"

static void M_SetBoxBlocked(const ITEM *const item, const bool blocked)
{
    const ROOM *const room = Room_Get(item->room_num);
    const SECTOR *const sector =
        Room_GetWorldSector(room, item->pos.x, item->pos.z);
    BOX_INFO *const box = Box_GetBox(sector->box);

    if (blocked && (box->overlap_index & BOX_BLOCKABLE) != 0) {
        box->overlap_index |= BOX_BLOCKED;
    } else if (!blocked && (box->overlap_index & BOX_BLOCKED) != 0) {
        box->overlap_index &= ~BOX_BLOCKED;
    }
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->flags = 0;
    item->mesh_bits = 1;
    M_SetBoxBlocked(item, true);
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        if ((item->object_id == O_WINDOW_1 || item->object_id == O_WINDOW_2)
            && (item->flags & IF_ONE_SHOT)) {
            item->mesh_bits = 0x100;
            M_SetBoxBlocked(item, false);
        }
    }
}

static void M_Control1(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->flags & IF_ONE_SHOT) {
        return;
    }

    if (Lara_Vehicle_IsMounted()) {
        if (Lara_IsNearItem(&item->pos, 512)) {
            Window_Smash(item_num);
        }
    } else if (item->touch_bits) {
        item->touch_bits = 0;
        const ITEM *const lara_item = Lara_GetItem();
        const int32_t speed =
            ABS((lara_item->speed * Math_Cos(lara_item->rot.y - item->rot.y))
                >> W2V_SHIFT);
        if (speed >= 50) {
            Window_Smash(item_num);
        }
    }
}

static void M_Control2(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->flags & IF_ONE_SHOT) {
        return;
    }

    M_SetBoxBlocked(item, false);

    item->mesh_bits = ~1;
    item->collidable = false;
    Item_Explode(item_num, 65278, 0);
    Sound_Effect(SFX_BRITTLE_GROUND_BREAK, &item->pos, SPM_NORMAL);

    item->flags |= IF_ONE_SHOT;
    item->status = IS_DEACTIVATED;
    Item_RemoveActive(item_num);
}

static void M_SetupBase(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->handle_save_func = M_HandleSave;
    obj->collision_func = Object_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_Setup1(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->control_func = M_Control1;
}

static void M_Setup2(OBJECT *const obj)
{
    M_SetupBase(obj);
    obj->control_func = M_Control2;
}

void Window_Smash(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_SetBoxBlocked(item, false);

    item->collidable = false;
    item->mesh_bits = ~1;
    Item_Explode(item_num, 0b11111110'11111110, 0);
    Sound_Effect(SFX_GLASS_BREAK, &item->pos, SPM_NORMAL);

    item->flags |= IF_ONE_SHOT;
    if (item->status == IS_ACTIVE) {
        Item_RemoveActive(item_num);
    }
    item->status = IS_DEACTIVATED;
}

REGISTER_OBJECT(O_WINDOW_1, M_Setup1)
REGISTER_OBJECT(O_WINDOW_2, M_Setup2)
