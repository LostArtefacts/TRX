#include "game/objects/creatures/bat.h"

#include "game/creature.h"
#include "game/effects/blood.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "util.h"

#include <stdbool.h>

#define BAT_ATTACK_DAMAGE 2
#define BAT_TURN (20 * PHD_DEGREE) // = 3640
#define BAT_HITPOINTS 1
#define BAT_RADIUS (WALL_L / 10) // = 102
#define BAT_SMARTNESS 0x400

typedef enum {
    BAT_EMPTY = 0,
    BAT_STOP = 1,
    BAT_FLY = 2,
    BAT_ATTACK = 3,
    BAT_FALL = 4,
    BAT_DEATH = 5,
} BAT_ANIM;

static BITE_INFO m_BatBite = { 0, 16, 45, 4 };

static void Bat_FixEmbeddedPosition(int16_t item_num);

static void Bat_FixEmbeddedPosition(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    if (item->status == IS_ACTIVE) {
        return;
    }

    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    int16_t room_number = item->room_number;
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_number);
    Room_GetHeight(floor, x, y, z);
    const int16_t ceiling = Room_GetCeiling(floor, x, y, z);

    // The bats animation and frame have to be changed to the hanging
    // one to properly measure them. Save it so it can be restored
    // after.
    int16_t old_anim =
        item->anim_number - g_Objects[item->object_number].anim_index;
    int16_t old_frame =
        item->frame_number - g_Anims[item->anim_number].frame_base;

    Item_SwitchToAnim(item, 0, 0);
    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    Item_SwitchToAnim(item, old_anim, old_frame);

    const int16_t bat_height = ABS(bounds->min.y);

    // Only move the bat if it's above the calculated position,
    // Palace Midas has many bats that aren't intended to be at
    // ceiling level.
    if (item->pos.y < ceiling + bat_height) {
        item->pos.y = ceiling + bat_height;
    }
}

void Bat_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Bat_Initialise;
    obj->control = Bat_Control;
    obj->collision = Creature_Collision;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->hit_points = BAT_HITPOINTS;
    obj->radius = BAT_RADIUS;
    obj->smartness = BAT_SMARTNESS;
    obj->intelligent = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void Bat_Control(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_INVISIBLE) {
        if (!LOT_EnableBaddieAI(item_num, 0)) {
            return;
        }
        item->status = IS_ACTIVE;
    }

    CREATURE_INFO *bat = item->data;
    PHD_ANGLE angle = 0;
    if (item->hit_points <= 0) {
        if (item->pos.y < item->floor) {
            item->gravity_status = 1;
            item->goal_anim_state = BAT_FALL;
            item->speed = 0;
        } else {
            item->gravity_status = 0;
            item->fall_speed = 0;
            item->goal_anim_state = BAT_DEATH;
            item->pos.y = item->floor;
        }
        Creature_Animate(item_num, 0, 0);
        return;
    } else {
        AI_INFO info;
        Creature_AIInfo(item, &info);
        Creature_Mood(item, &info, false);
        angle = Creature_Turn(item, BAT_TURN);

        switch (item->current_anim_state) {
        case BAT_STOP:
            item->goal_anim_state = BAT_FLY;
            break;

        case BAT_FLY:
            if (item->touch_bits) {
                item->goal_anim_state = BAT_ATTACK;
                Creature_Animate(item_num, angle, 0);
                return;
            }
            break;

        case BAT_ATTACK:
            if (item->touch_bits) {
                Creature_Effect(item, &m_BatBite, Effect_Blood);
                Lara_TakeDamage(BAT_ATTACK_DAMAGE, true);
            } else {
                item->goal_anim_state = BAT_FLY;
                bat->mood = MOOD_BORED;
            }
            break;
        }
    }

    Creature_Animate(item_num, angle, 0);
}

void Bat_Initialise(int16_t item_num)
{
    Creature_Initialise(item_num);

    // Almost all of the bats in the OG levels are embedded in the ceiling.
    // This will move all bats up to the ceiling of their rooms and down
    // by the height of their hanging animation.
    Bat_FixEmbeddedPosition(item_num);
}
