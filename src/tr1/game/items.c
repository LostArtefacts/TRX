#include "game/items.h"

#include "game/carrier.h"
#include "game/effects.h"
#include "game/random.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

static BOUNDS_16 m_NullBounds = {};
static BOUNDS_16 m_InterpolatedBounds = {};

void Item_Control(void)
{
    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        ITEM *item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->control_func != nullptr) {
            obj->control_func(item_num);
        }
        item_num = item->next_active;
    }

    Carrier_AnimateDrops();
}

void Item_Initialise(int16_t item_num)
{
    ITEM *item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Item_SwitchToAnim(item, 0, 0);
    item->current_anim_state = Item_GetAnim(item)->current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->required_anim_state = 0;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    item->status = IS_INACTIVE;
    item->active = 0;
    item->gravity = 0;
    item->hit_status = 0;
    item->looked_at = 0;
    item->collidable = 1;
    item->hit_points = obj->hit_points;
    item->timer = 0;
    item->mesh_bits = -1;
    item->touch_bits = 0;
    item->data = nullptr;
    item->priv = nullptr;
    item->carried_item = nullptr;
    item->enable_shadow = true;
    item->enable_interpolation = true;

    if (item->flags & IF_INVISIBLE) {
        item->status = IS_INVISIBLE;
        item->flags &= ~IF_INVISIBLE;
    }

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        item->flags &= ~IF_CODE_BITS;
        item->flags |= IF_REVERSE;
        Item_AddActive(item_num);
        item->status = IS_ACTIVE;
    }

    ROOM *const room = Room_Get(item->room_num);
    item->next_item = room->item_num;
    room->item_num = item_num;
    const SECTOR *const sector =
        Room_GetWorldSector(room, item->pos.x, item->pos.z);
    item->floor = sector->floor.height;

    if (Game_IsBonusFlagSet(GBF_NGPLUS)) {
        item->hit_points *= 2;
    }
    if (obj->initialise_func) {
        obj->initialise_func(item_num);
    }

    Interpolation_RememberItem(item);
}

int16_t Item_Spawn(const ITEM *const item, const GAME_OBJECT_ID obj_id)
{
    const int16_t spawn_num = Item_Create();
    if (spawn_num != NO_ITEM) {
        ITEM *const spawn = Item_Get(spawn_num);
        spawn->object_id = obj_id;
        spawn->room_num = item->room_num;
        spawn->pos = item->pos;
        spawn->rot = item->rot;
        Item_Initialise(spawn_num);
        spawn->status = IS_INACTIVE;
        spawn->shade.value_1 = SHADE_NEUTRAL;
    }
    return spawn_num;
}

bool Item_Test3DRange(int32_t x, int32_t y, int32_t z, int32_t range)
{
    return ABS(x) < range && ABS(y) < range && ABS(z) < range
        && (SQUARE(x) + SQUARE(y) + SQUARE(z) < SQUARE(range));
}

ANIM_FRAME *Item_GetBestFrame(const ITEM *item)
{
    ANIM_FRAME *frames[2];
    int32_t rate;
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    return frames[(frac > rate / 2) ? 1 : 0];
}

const BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *item)
{
    int32_t rate;
    ANIM_FRAME *frames[2];
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    if (frames[0] == nullptr) {
        return &m_NullBounds;
    }

    if (frac == 0) {
        return &frames[0]->bounds;
    }

    const BOUNDS_16 *const a = &frames[0]->bounds;
    const BOUNDS_16 *const b = &frames[1]->bounds;
    BOUNDS_16 *const result = &m_InterpolatedBounds;

    result->min.x = a->min.x + (((b->min.x - a->min.x) * frac) / rate);
    result->min.y = a->min.y + (((b->min.y - a->min.y) * frac) / rate);
    result->min.z = a->min.z + (((b->min.z - a->min.z) * frac) / rate);
    result->max.x = a->max.x + (((b->max.x - a->max.x) * frac) / rate);
    result->max.y = a->max.y + (((b->max.y - a->max.y) * frac) / rate);
    result->max.z = a->max.z + (((b->max.z - a->max.z) * frac) / rate);
    return result;
}

int32_t Item_GetFrames(const ITEM *item, ANIM_FRAME *frames[], int32_t *rate)
{
    const ANIM *const anim = Item_GetAnim(item);
    if (anim->frame_ptr == nullptr) {
        frames[0] = nullptr;
        return 0;
    }

    const int32_t cur_frame_num = item->frame_num - anim->frame_base;
    const int32_t last_frame_num = anim->frame_end - anim->frame_base;
    const int32_t key_frame_span = anim->interpolation;
    const int32_t first_key_frame_num = cur_frame_num / key_frame_span;
    const int32_t second_key_frame_num = first_key_frame_num + 1;

    frames[0] = &anim->frame_ptr[first_key_frame_num];
    frames[1] = &anim->frame_ptr[second_key_frame_num];

    const int32_t key_frame_shift = cur_frame_num % key_frame_span;
    const int32_t numerator = key_frame_shift;
    int32_t denominator = key_frame_span;
    if (numerator && second_key_frame_num > anim->frame_end) {
        denominator = anim->frame_end + key_frame_span - second_key_frame_num;
    }

    // OG
    if (g_Config.rendering.fps == 30) {
        *rate = denominator;
        return numerator;
    }

    // interpolated
    if (item != g_LaraItem
        && (!item->active || item->status != IS_ACTIVE
            || !Object_Get(item->object_id)->enable_interpolation)) {
        *rate = denominator;
        return numerator;
    }

    const double clock_ratio = Interpolation_GetWorldRate() - 0.5;
    const double final =
        (key_frame_shift + clock_ratio) / (double)key_frame_span;
    const double interp_frame_num =
        (first_key_frame_num * key_frame_span) + (final * key_frame_span);
    if (interp_frame_num >= last_frame_num) {
        *rate = denominator;
        return numerator;
    }

    *rate = 10;
    return final * 10;
}
