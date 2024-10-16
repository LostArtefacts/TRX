#include "game/objects/general/zipline.h"

#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/math.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

#define ZIPLINE_MAX_SPEED 100
#define ZIPLINE_ACCELERATION 5

typedef enum {
    ZIPLINE_EMPTY = 0,
    ZIPLINE_GRAB = 1,
    ZIPLINE_HANG = 2,
} ZIPLINE_STATUS;

void __cdecl Zipline_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (!(g_Input & IN_ACTION) || g_Lara.gun_status != LGS_ARMLESS
        || lara_item->gravity || lara_item->current_anim_state != LS_STOP) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INACTIVE) {
        return;
    }

    if (!Item_TestPosition(g_ZiplineHandleBounds, item, lara_item)) {
        return;
    }

    Item_AlignPosition(&g_ZiplineHandlePosition, item, lara_item);
    g_Lara.gun_status = LGS_HANDS_BUSY;

    lara_item->goal_anim_state = LS_ZIPLINE;
    do {
        Item_Animate(lara_item);
    } while (lara_item->current_anim_state != LS_NULL);

    if (!item->active) {
        Item_AddActive(item_num);
    }

    item->status = IS_ACTIVE;
    item->flags |= IF_ONE_SHOT;
}

void __cdecl Zipline_Control(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];
    if (item->status != IS_ACTIVE) {
        return;
    }

    if (!(item->flags & IF_ONE_SHOT)) {
        const GAME_VECTOR *const old = item->data;
        item->pos = old->pos;
        if (old->room_num != item->room_num) {
            Item_NewRoom(item_num, old->room_num);
        }
        item->status = IS_INACTIVE;
        item->goal_anim_state = ZIPLINE_GRAB;
        item->current_anim_state = ZIPLINE_GRAB;
        item->anim_num = g_Objects[item->object_id].anim_idx;
        item->frame_num = g_Anims[item->anim_num].frame_base;
        Item_RemoveActive(item_num);
        return;
    }

    if (item->current_anim_state == ZIPLINE_GRAB) {
        Item_Animate(item);
        return;
    }

    Item_Animate(item);
    if (item->fall_speed < ZIPLINE_MAX_SPEED) {
        item->fall_speed += ZIPLINE_ACCELERATION;
    }

    const int32_t c = Math_Cos(item->rot.y);
    const int32_t s = Math_Sin(item->rot.y);
    item->pos.x += (item->fall_speed * s) >> W2V_SHIFT;
    item->pos.z += (item->fall_speed * c) >> W2V_SHIFT;
    item->pos.y += item->fall_speed >> 2;

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_num) {
        Item_NewRoom(item_num, room_num);
    }

    const bool lara_on_zipline = g_LaraItem->current_anim_state == LS_ZIPLINE;
    if (lara_on_zipline) {
        g_LaraItem->pos = item->pos;
    }

    const int32_t x = item->pos.x + ((s * WALL_L) >> W2V_SHIFT);
    const int32_t z = item->pos.z + ((c * WALL_L) >> W2V_SHIFT);
    const int32_t y = item->pos.y + (STEP_L >> 2);

    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    if (Room_GetHeight(sector, x, y, z) > y + STEP_L
        && Room_GetCeiling(sector, x, y, z) < y - STEP_L) {
        Sound_Effect(SFX_ZIPLINE_GO, &item->pos, SPM_ALWAYS);
        return;
    }

    if (lara_on_zipline) {
        g_LaraItem->goal_anim_state = LS_FORWARD_JUMP;
        Lara_Animate(g_LaraItem);
        g_LaraItem->gravity = 1;
        g_LaraItem->speed = item->fall_speed;
        g_LaraItem->fall_speed = item->fall_speed >> 2;
    }
    Sound_Effect(SFX_ZIPLINE_STOP, &item->pos, SPM_ALWAYS);
    Item_RemoveActive(item_num);
    item->status = IS_INACTIVE;
    item->flags &= ~IF_ONE_SHOT;
}
