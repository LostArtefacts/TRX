#include "game/objects/ai/bat.h"

#include "config.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/lot.h"
#include "game/objects/effects/blood.h"
#include "global/types.h"
#include "global/vars.h"

BITE_INFO g_BatBite = { 0, 16, 45, 4 };

static void FixEmbeddedBatPosition(int16_t item_num);

void SetupBat(OBJECT_INFO *obj)
{
    if (!obj->loaded) {
        return;
    }
    obj->initialise = InitialiseBat;
    obj->control = BatControl;
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

void BatControl(int16_t item_num)
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

        CreatureAIInfo(item, &info);
        CreatureMood(item, &info, 0);
        angle = CreatureTurn(item, BAT_TURN);

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
                CreatureEffect(item, &g_BatBite, Blood_Spawn);
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

void InitialiseBat(int16_t item_num)
{
    InitialiseCreature(item_num);

    // Almost all of the bats in the OG levels are embedded in the ceiling.
    // This will move all bats up to the ceiling of their rooms and down
    // by the height of their hanging animation.
    FixEmbeddedBatPosition(item_num);
}

static void FixEmbeddedBatPosition(int16_t item_num)
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

        floor = GetFloor(x, y, z, &room_number);
        GetHeight(floor, x, y, z);
        ceiling = GetCeiling(floor, x, y, z);

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
