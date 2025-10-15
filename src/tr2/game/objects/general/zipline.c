#include <libtrx/game/game_buf.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/game/rooms.h>
#include <libtrx/game/sound.h>

#define ZIPLINE_MAX_SPEED 100
#define ZIPLINE_ACCELERATION 5

typedef enum {
    ZIPLINE_STATE_EMPTY = 0,
    ZIPLINE_STATE_GRAB = 1,
    ZIPLINE_STATE_HANG = 2,
} ZIPLINE_STATE;

static XYZ_32 m_ZiplineHandlePosition = {
    .x = 0,
    .y = 0,
    .z = WALL_L / 2 - 141,
};

static const OBJECT_BOUNDS m_ZiplineHandleBounds = {
    .shift = {
        .min = { .x = -WALL_L / 4, .y = -100, .z = +WALL_L / 4, },
        .max = { .x = +WALL_L / 4, .y = +100, .z = +WALL_L / 2, },
    },
    .rot = {
        .min = { .x = +0, .y = -25 * DEG_1, .z = +0, },
        .max = { .x = +0, .y = +25 * DEG_1, .z = +0, },
    },
};

static const OBJECT_BOUNDS *M_Bounds(void)
{
    return &m_ZiplineHandleBounds;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    GAME_VECTOR *const data =
        GameBuf_Alloc(sizeof(GAME_VECTOR), GBUF_ITEM_DATA);
    data->pos = item->pos;
    data->room_num = item->room_num;
    item->data = data;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_ACTIVE) {
        return;
    }

    if (!(item->flags & IF_ONE_SHOT)) {
        const GAME_VECTOR *const old = item->data;
        item->pos = old->pos;
        Item_UpdateRoom(item_num, old->room_num);
        item->status = IS_INACTIVE;
        item->goal_anim_state = ZIPLINE_STATE_GRAB;
        item->current_anim_state = ZIPLINE_STATE_GRAB;
        Item_SwitchToAnim(item, 0, 0);
        Item_RemoveActive(item_num);
        return;
    }

    if (item->current_anim_state == ZIPLINE_STATE_GRAB) {
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
    Item_UpdateRoom(item_num, room_num);

    ITEM *const lara_item = Lara_GetItem();
    const bool lara_on_zipline =
        lara_item->current_anim_state == LS(LS_ZIPLINE);
    if (lara_on_zipline) {
        lara_item->pos = item->pos;
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
        lara_item->goal_anim_state = LS(LS_JUMP_FORWARD);
        Lara_Animate(lara_item);
        lara_item->gravity = 1;
        lara_item->speed = item->fall_speed;
        lara_item->fall_speed = item->fall_speed >> 2;
    }
    Sound_Effect(SFX_ZIPLINE_STOP, &item->pos, SPM_ALWAYS);
    Item_RemoveActive(item_num);
    item->status = IS_INACTIVE;
    item->flags &= ~IF_ONE_SHOT;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    if (!g_Input.action || lara->gun_status != LGS_ARMLESS || lara_item->gravity
        || lara_item->current_anim_state != LS(LS_STOP)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    if (item->status != IS_INACTIVE) {
        return;
    }

    const OBJECT *const obj = Object_Get(item->object_id);
    if (!Lara_TestPosition(item, obj->bounds_func())) {
        return;
    }

    Lara_AlignPosition(item, &m_ZiplineHandlePosition);
    lara->gun_status = LGS_HANDS_BUSY;

    lara_item->goal_anim_state = LS(LS_ZIPLINE);
    do {
        Item_Animate(lara_item);
    } while (lara_item->current_anim_state != LS(LS_PULL_UP));

    if (!item->active) {
        Item_AddActive(item_num);
    }

    item->status = IS_ACTIVE;
    item->flags |= IF_ONE_SHOT;
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->bounds_func = M_Bounds;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_ZIPLINE_HANDLE, M_Setup)
