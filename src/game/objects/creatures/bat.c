#include "game/objects/creatures/bat.h"

#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/creature.h"
#include "game/draw.h"
#include "game/effects/blood.h"
#include "game/lot.h"
#include "game/room.h"
#include "global/types.h"
#include "global/vars.h"

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
    ITEM_INFO *item;
    FLOOR_INFO *floor;
    int32_t x, y, z;
    int16_t room_number, ceiling, old_anim, old_frame, bat_height;
    int16_t *bounds;

    item = &g_Items[item_num];
    if (item->status != IS_ACTIVE) {
        x = item->pos.x;
        y = item->pos.y;
        z = item->pos.z;
        room_number = item->room_number;

        floor = Room_GetFloor(x, y, z, &room_number);
        Room_GetHeight(floor, x, y, z);
        ceiling = Room_GetCeiling(floor, x, y, z);

        // The bats animation and frame have to be changed to the hanging
        // one to properly measure them. Save it so it can be restored
        // after.
        old_anim = item->anim_number;
        old_frame = item->frame_number;

        item->anim_number = g_Objects[item->object_number].anim_index;
        item->frame_number = g_Anims[item->anim_number].frame_base;
        bounds = GetBoundsAccurate(item);

        item->anim_number = old_anim;
        item->frame_number = old_frame;

        bat_height = ABS(bounds[FRAME_BOUND_MIN_Y]);

        // Only move the bat if it's above the calculated position,
        // Palace Midas has many bats that aren't intended to be at
        // ceiling level.
        if (item->pos.y < ceiling + bat_height) {
            item->pos.y = ceiling + bat_height;
        }
    }
}

void Bat_Setup(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = Bat_Initialise;
    obj->control = Bat_Control;
    obj->collision = CreatureCollision;
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
        if (!EnableBaddieAI(item_num, 0)) {
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
            item->goal_anim_state = BAT_DEATH;
            item->pos.y = item->floor;
        }
        CreatureAnimation(item_num, 0, 0);
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
                CreatureAnimation(item_num, angle, 0);
                return;
            }
            break;

        case BAT_ATTACK:
            if (item->touch_bits) {
                Creature_Effect(item, &m_BatBite, Effect_Blood);
                g_LaraItem->hit_points -= BAT_ATTACK_DAMAGE;
                g_LaraItem->hit_status = 1;
            } else {
                item->goal_anim_state = BAT_FLY;
                bat->mood = MOOD_BORED;
            }
            break;
        }
    }

    CreatureAnimation(item_num, angle, 0);
}

void Bat_Initialise(int16_t item_num)
{
    Creature_Initialise(item_num);

    // Almost all of the bats in the OG levels are embedded in the ceiling.
    // This will move all bats up to the ceiling of their rooms and down
    // by the height of their hanging animation.
    Bat_FixEmbeddedPosition(item_num);
}
