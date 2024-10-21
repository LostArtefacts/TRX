#include "decomp/skidoo.h"

#include "game/input.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/room.h"
#include "global/funcs.h"
#include "global/vars.h"

typedef enum {
    SKIDOO_GET_ON_NONE = 0,
    SKIDOO_GET_ON_LEFT = 1,
    SKIDOO_GET_ON_RIGHT = 2,
} SKIDOO_GET_ON_SIDE;

typedef enum {
    // clang-format off
    LARA_STATE_SKIDOO_SIT       = 0,
    LARA_STATE_SKIDOO_GET_ON    = 1,
    LARA_STATE_SKIDOO_LEFT      = 2,
    LARA_STATE_SKIDOO_RIGHT     = 3,
    LARA_STATE_SKIDOO_FALL      = 4,
    LARA_STATE_SKIDOO_HIT       = 5,
    LARA_STATE_SKIDOO_GET_ON_L  = 6,
    LARA_STATE_SKIDOO_GET_OFF_L = 7,
    LARA_STATE_SKIDOO_STILL     = 8,
    LARA_STATE_SKIDOO_GET_OFF   = 9,
    LARA_STATE_SKIDOO_LETGO     = 10,
    LARA_STATE_SKIDOO_DEATH     = 11,
    LARA_STATE_SKIDOO_FALLOFF   = 12,
    // clang-format on
} LARA_SKIDOO_STATE;

typedef enum {
    // clang-format off
    LA_SKIDOO_GET_ON_L = 1,
    LA_SKIDOO_GET_ON_R = 18,
    // clang-format on
} LARA_ANIM_SKIDOO;

#define SKIDOO_RADIUS 500
#define M_TARGET_DIST (WALL_L * 2) // = 2048

static bool M_IsNearby(const ITEM *item_1, const ITEM *item_2);
static bool M_CheckBaddieCollision(ITEM *item, const ITEM *skidoo);

static bool M_IsNearby(const ITEM *const item_1, const ITEM *const item_2)
{
    const int32_t dx = item_1->pos.x - item_2->pos.x;
    const int32_t dy = item_1->pos.y - item_2->pos.y;
    const int32_t dz = item_1->pos.z - item_2->pos.z;
    return dx > -M_TARGET_DIST && dx < M_TARGET_DIST && dy > -M_TARGET_DIST
        && dy < M_TARGET_DIST && dz > -M_TARGET_DIST && dz < M_TARGET_DIST;
}

static bool M_CheckBaddieCollision(ITEM *const item, const ITEM *const skidoo)
{
    if (!item->collidable || item->status == IS_INVISIBLE || item == g_LaraItem
        || item == skidoo) {
        return false;
    }

    const OBJECT *const object = Object_GetObject(item->object_id);
    const bool is_availanche = item->object_id == O_ROLLING_BALL_2;
    if (object->collision == NULL || (!object->intelligent && !is_availanche)) {
        return false;
    }

    if (!M_IsNearby(item, skidoo)) {
        return false;
    }

    if (!Item_TestBoundsCollide(item, skidoo, SKIDOO_RADIUS)) {
        return false;
    }

    if (item->object_id == O_SKIDOO_ARMED) {
        SkidooDriver_Push(item, skidoo, SKIDOO_RADIUS);
    } else if (is_availanche) {
        if (item->current_anim_state == TRAP_ACTIVATE) {
            Lara_TakeDamage(100, true);
        }
    } else {
        DoLotsOfBlood(
            item->pos.x, skidoo->pos.y - STEP_L, item->pos.z, skidoo->speed,
            skidoo->rot.y, item->room_num, 3);
        item->hit_points = 0;
    }
    return true;
}

void __cdecl Skidoo_Initialise(const int16_t item_num)
{
    ITEM *const item = &g_Items[item_num];

    SKIDOO_INFO *const skidoo_data =
        game_malloc(sizeof(SKIDOO_INFO), GBUF_SKIDOO_STUFF);
    skidoo_data->skidoo_turn = 0;
    skidoo_data->right_fallspeed = 0;
    skidoo_data->left_fallspeed = 0;
    skidoo_data->extra_rotation = 0;
    skidoo_data->momentum_angle = item->rot.y;
    skidoo_data->track_mesh = 0;
    skidoo_data->pitch = 0;

    item->data = skidoo_data;
}

int32_t __cdecl Skidoo_CheckGetOn(const int16_t item_num, COLL_INFO *const coll)
{
    if (!(g_Input & IN_ACTION) || g_Lara.gun_status != LGS_ARMLESS
        || g_LaraItem->gravity) {
        return SKIDOO_GET_ON_NONE;
    }

    ITEM *const item = &g_Items[item_num];
    const int16_t angle = item->rot.y - g_LaraItem->rot.y;

    SKIDOO_GET_ON_SIDE get_on = SKIDOO_GET_ON_NONE;
    if (angle > PHD_45 && angle < PHD_135) {
        get_on = SKIDOO_GET_ON_LEFT;
    } else if (angle > -PHD_135 && angle < -PHD_45) {
        get_on = SKIDOO_GET_ON_RIGHT;
    }

    if (!Item_TestBoundsCollide(item, g_LaraItem, coll->radius)) {
        return SKIDOO_GET_ON_NONE;
    }

    if (!Collide_TestCollision(item, g_LaraItem)) {
        return SKIDOO_GET_ON_NONE;
    }

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (height < -32000) {
        return SKIDOO_GET_ON_NONE;
    }

    return get_on;
}

void __cdecl Skidoo_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    if (lara_item->hit_points < 0 || g_Lara.skidoo != NO_ITEM) {
        return;
    }

    const SKIDOO_GET_ON_SIDE get_on = Skidoo_CheckGetOn(item_num, coll);
    if (get_on == SKIDOO_GET_ON_NONE) {
        Object_Collision(item_num, lara_item, coll);
        return;
    }

    g_Lara.skidoo = item_num;
    if (g_Lara.gun_type == LGT_FLARE) {
        Flare_Create(false);
        Flare_UndrawMeshes();
        g_Lara.flare_control_left = 0;
        g_Lara.gun_type = LGT_UNARMED;
        g_Lara.request_gun_type = LGT_UNARMED;
    }
    if (get_on == SKIDOO_GET_ON_LEFT) {
        lara_item->anim_num =
            g_Objects[O_LARA_SKIDOO].anim_idx + LA_SKIDOO_GET_ON_L;
    } else if (get_on == SKIDOO_GET_ON_RIGHT) {
        lara_item->anim_num =
            g_Objects[O_LARA_SKIDOO].anim_idx + LA_SKIDOO_GET_ON_R;
    }
    lara_item->current_anim_state = LARA_STATE_SKIDOO_GET_ON;
    lara_item->frame_num = g_Anims[lara_item->anim_num].frame_base;
    g_Lara.gun_status = LGS_ARMLESS;

    ITEM *const item = &g_Items[item_num];
    lara_item->pos.x = item->pos.x;
    lara_item->pos.y = item->pos.y;
    lara_item->pos.z = item->pos.z;
    lara_item->rot.y = item->rot.y;
    item->hit_points = 1;
}

void __cdecl Skidoo_BaddieCollision(const ITEM *const skidoo)
{
    int16_t roomies[12];
    const int32_t roomies_count =
        Room_GetAdjoiningRooms(skidoo->room_num, roomies, 12);

    for (int32_t i = 0; i < roomies_count; i++) {
        const ROOM *const room = Room_Get(roomies[i]);
        int16_t item_num = room->item_num;
        while (item_num != NO_ITEM) {
            ITEM *item = Item_Get(item_num);
            M_CheckBaddieCollision(item, skidoo);
            item_num = item->next_item;
        }
    }
}
