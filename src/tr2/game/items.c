#include "game/items.h"

#include "game/effects.h"
#include "game/game_flow.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/game.h>
#include <libtrx/game/interpolation.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

static BOUNDS_16 m_NullBounds = {};
static BOUNDS_16 m_InterpolatedBounds = {};

void Item_Control(void)
{
    int16_t item_num = Item_GetNextActive();
    while (item_num != NO_ITEM) {
        const ITEM *const item = Item_Get(item_num);
        const int16_t next = item->next_active;
        const OBJECT *obj = Object_Get(item->object_id);
        if (!(item->flags & IF_KILLED) && obj->control_func != nullptr) {
            obj->control_func(item_num);
        }
        item_num = next;
    }
}

void Item_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const OBJECT *const obj = Object_Get(item->object_id);

    Item_SwitchToAnim(item, 0, 0);
    item->goal_anim_state = Item_GetAnim(item)->current_anim_state;
    item->current_anim_state = item->goal_anim_state;
    item->required_anim_state = 0;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    item->hit_points = obj->hit_points;
    item->timer = 0;
    item->mesh_bits = 0xFFFFFFFF;
    item->touch_bits = 0;
    item->data = nullptr;
    item->priv = nullptr;

    item->active = 0;
    item->status = IS_INACTIVE;
    item->gravity = 0;
    item->hit_status = 0;
    item->collidable = 1;
    item->looked_at = 0;
    item->killed = 0;
    item->enable_interpolation = true;

    if ((item->flags & IF_INVISIBLE) != 0) {
        item->status = IS_INVISIBLE;
        item->flags &= ~IF_INVISIBLE;
    } else if (obj->intelligent) {
        item->status = IS_INVISIBLE;
    }

    if (item->flags & IF_KILLED) {
        item->killed = 1;
        item->flags &= ~IF_KILLED;
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

    if (Game_IsBonusFlagSet(GBF_NGPLUS)
        && GF_GetCurrentLevel()->type != GFL_DEMO) {
        item->hit_points *= 2;
    }

    if (obj->initialise_func != nullptr) {
        obj->initialise_func(item_num);
    }
}

void Item_ClearKilled(void)
{
    // Remove corpses and other killed items. Part of OG performance
    // improvements, generously used in Opera House and Barkhang Monastery
    int16_t link_num = Item_GetPrevActive();
    while (link_num != NO_ITEM) {
        ITEM *const item = Item_Get(link_num);
        Item_Kill(link_num);
        link_num = item->next_active;
        item->next_active = NO_ITEM;
    }
    Item_SetPrevActive(NO_ITEM);
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
    const int32_t key_frame_shift = cur_frame_num % key_frame_span;
    const int32_t first_key_frame_num = cur_frame_num / key_frame_span;
    const int32_t second_key_frame_num = first_key_frame_num + 1;

    const int32_t numerator = key_frame_shift;
    int32_t denominator = key_frame_span;
    if (numerator != 0) {
        // TODO: ??
        const int32_t second_key_frame_num2 =
            (cur_frame_num / key_frame_span + 1) * key_frame_span;
        if (second_key_frame_num2 > anim->frame_end) {
            denominator += anim->frame_end - second_key_frame_num2;
        }
    }

    frames[0] = &anim->frame_ptr[first_key_frame_num];
    frames[1] = &anim->frame_ptr[second_key_frame_num];

    // OG
    if (g_Config.rendering.fps == 30) {
        *rate = denominator;
        return numerator;
    }

    // interpolated
    if (item != g_LaraItem
        && (!item->active || item->status != IS_ACTIVE
            || !item->enable_interpolation
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

const BOUNDS_16 *Item_GetBoundsAccurate(const ITEM *const item)
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

#define CALC(target, b1, b2, prop)                                             \
    target->prop = (b1)->prop + ((((b2)->prop - (b1)->prop) * frac) / rate);

    BOUNDS_16 *const result = &m_InterpolatedBounds;
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, min.x);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, max.x);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, min.y);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, max.y);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, min.z);
    CALC(result, &frames[0]->bounds, &frames[1]->bounds, max.z);
    return result;
}

ANIM_FRAME *Item_GetBestFrame(const ITEM *const item)
{
    ANIM_FRAME *frames[2];
    int32_t rate = 0;
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    return frames[(frac > rate / 2) ? 1 : 0];
}
