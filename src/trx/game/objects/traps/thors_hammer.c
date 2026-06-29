#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>

#define M_HEAD_DIST (WALL_L * 3) // = 3072
#define M_HEAD_HEIGHT (WALL_L * 2) // = 2048
#define M_DEATH_RADIUS (WALL_L / 2 + 8) // = 520

typedef enum {
    M_STATE_SET,
    M_STATE_TEASE,
    M_STATE_ACTIVE,
    M_STATE_DONE,
} M_STATE;

typedef struct {
    int16_t head_item_num;
} M_PRIV;

static void M_InitialiseHandle(const int16_t item_num)
{
    ITEM *const hand_item = Item_Get(item_num);
    M_PRIV *const p = hand_item->priv;
    const int16_t head_item_num = Item_CreateLevelItem();
    ASSERT(head_item_num != NO_ITEM);
    ITEM *const head_item = Item_Get(head_item_num);
    head_item->object_id = O_THORS_HEAD;
    head_item->room_num = hand_item->room_num;
    head_item->pos = hand_item->pos;
    head_item->rot = hand_item->rot;
    head_item->shade.value_1 = hand_item->shade.value_1;
    Item_Initialise(head_item_num);
    p->head_item_num = head_item_num;
}

static bool M_ShouldKillLara(const ITEM *const item)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item->hit_points < 0 || g_Config.debug.enable_invulnerability) {
        return false;
    }

    const XYZ_32 pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, M_HEAD_DIST);
    if (lara_item->pos.x <= pos.x - M_DEATH_RADIUS
        || lara_item->pos.x >= pos.x + M_DEATH_RADIUS
        || lara_item->pos.z <= pos.z - M_DEATH_RADIUS
        || lara_item->pos.z >= pos.z + M_DEATH_RADIUS) {
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(lara_item);
    const int32_t lara_height = ABS(bounds->max.y - bounds->min.y);
    return lara_item->pos.y - lara_height <= item->pos.y;
}

static void M_KillLara(const ITEM *const item)
{
    ITEM *const lara_item = Lara_GetItem();
    lara_item->hit_points = -1;
    lara_item->pos.y = item->floor;
    lara_item->gravity = false;
    lara_item->enable_shadow = false;
    lara_item->current_anim_state = LS(LS_SPECIAL);
    lara_item->goal_anim_state = LS(LS_SPECIAL);
    Item_SwitchToAnim(lara_item, LA(LA_BOULDER_DEATH), 0);
}

static void M_ControlHandle(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    ITEM *const lara_item = Lara_GetItem();

    switch (item->current_anim_state) {
    case M_STATE_SET:
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = M_STATE_TEASE;
        } else {
            Item_RemoveActive(item_num);
            item->status = IS_INACTIVE;
        }
        break;

    case M_STATE_TEASE:
        if (Item_IsTriggerActive(item)) {
            item->goal_anim_state = M_STATE_ACTIVE;
        } else {
            item->goal_anim_state = M_STATE_SET;
        }
        break;

    case M_STATE_ACTIVE: {
        const int32_t frame_num = Item_GetRelativeFrame(item);
        if (frame_num > 30 && M_ShouldKillLara(item)) {
            M_KillLara(item);
        }
        break;
    }

    case M_STATE_DONE: {
        Room_TestTriggers(item);

        const XYZ_32 old_pos = item->pos;
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, M_HEAD_DIST);
        if (lara_item->hit_points >= 0) {
            Room_AlterFloorHeight(item, -M_HEAD_HEIGHT);
        }
        item->pos = old_pos;

        Item_RemoveActive(item_num);
        item->status = IS_DEACTIVATED;
        break;
    }
    }
    Item_Animate(item);

    M_PRIV *const p = item->priv;
    ITEM *const head_item = Item_Get(p->head_item_num);
    const int16_t relative_anim = Item_GetRelativeAnim(item);
    const int16_t relative_frame = Item_GetRelativeFrame(item);
    Item_SwitchToAnim(head_item, relative_anim, relative_frame);
    head_item->current_anim_state = item->current_anim_state;
}

static void M_CollisionHandle(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push) {
        Lara_Col_ItemPush(item, coll, false, true);
    }
}

static void M_CollisionHead(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (coll->enable_baddie_push
        && item->current_anim_state != M_STATE_ACTIVE) {
        Lara_Col_ItemPush(item, coll, false, true);
    }
}

static void M_SetupHandle(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseHandle;
    obj->control_func = M_ControlHandle;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->collision_func = M_CollisionHandle;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupHead(OBJECT *const obj)
{
    obj->collision_func = M_CollisionHead;
    obj->draw_func = Object_DrawUnclippedItem;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_THORS_HANDLE, M_SetupHandle)
REGISTER_OBJECT(O_THORS_HEAD, M_SetupHead)
