#include "game/traps/rolling_ball.h"

#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/effects/blood.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sphere.h"
#include "global/vars.h"
#include "specific/init.h"

void SetupRollingBall(OBJECT_INFO *obj)
{
    obj->initialise = InitialiseRollingBall;
    obj->control = RollingBallControl;
    obj->collision = RollingBallCollision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void InitialiseRollingBall(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    GAME_VECTOR *old = game_malloc(sizeof(GAME_VECTOR), GBUF_ROLLINGBALL_STUFF);
    item->data = old;
    old->x = item->pos.x;
    old->y = item->pos.y;
    old->z = item->pos.z;
    old->room_number = item->room_number;
}

void RollingBallControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    if (item->status == IS_ACTIVE) {
        if (item->pos.y < item->floor) {
            if (!item->gravity_status) {
                item->gravity_status = 1;
                item->fall_speed = -10; //(-10/ANIM_SCALE);
            }
        } else if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_ACTIVATE;
        }

        int32_t oldx = item->pos.x;
        int32_t oldz = item->pos.z;
        AnimateItem(item);

        int16_t room_num = item->room_number;
        FLOOR_INFO *floor =
            GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
        if (item->room_number != room_num) {
            ItemNewRoom(item_num, room_num);
        }

        item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

        TestTriggers(TriggerIndex, 1);

        if (item->pos.y >= item->floor - STEP_L) {
            item->gravity_status = 0;
            item->fall_speed = 0;
            item->pos.y = item->floor;
        }

        int32_t x = item->pos.x
            + (((WALL_L / 2) * phd_sin(item->pos.y_rot)) >> W2V_SHIFT);
        int32_t z = item->pos.z
            + (((WALL_L / 2) * phd_cos(item->pos.y_rot)) >> W2V_SHIFT);
        floor = GetFloor(x, item->pos.y, z, &room_num);
        if (GetHeight(floor, x, item->pos.y, z) < item->pos.y) {
            item->status = IS_DEACTIVATED;
            item->pos.x = oldx;
            item->pos.y = item->floor;
            item->pos.z = oldz;
            item->speed = 0;
            item->fall_speed = 0;
            item->touch_bits = 0;
        }
    } else if (item->status == IS_DEACTIVATED && !TriggerActive(item)) {
        item->status = IS_NOT_ACTIVE;
        GAME_VECTOR *old = item->data;
        item->pos.x = old->x;
        item->pos.y = old->y;
        item->pos.z = old->z;
        if (item->room_number != old->room_number) {
            RemoveDrawnItem(item_num);
            ROOM_INFO *r = &RoomInfo[old->room_number];
            item->next_item = r->item_number;
            r->item_number = item_num;
            item->room_number = old->room_number;
        }
        item->current_anim_state = TRAP_SET;
        item->goal_anim_state = TRAP_SET;
        item->anim_number = Objects[item->object_number].anim_index;
        item->frame_number = Anims[item->anim_number].frame_base;
        item->current_anim_state = Anims[item->anim_number].current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        item->required_anim_state = TRAP_SET;
        RemoveActiveItem(item_num);
    }
}

void RollingBallCollision(
    int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &Items[item_num];

    if (item->status != IS_ACTIVE) {
        if (item->status != IS_INVISIBLE) {
            ObjectCollision(item_num, lara_item, coll);
        }
        return;
    }

    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    int32_t x, y, z, d;
    if (lara_item->gravity_status) {
        if (coll->enable_baddie_push) {
            ItemPushLara(item, lara_item, coll, coll->enable_spaz, 1);
        }
        lara_item->hit_points -= ROLLINGBALL_DAMAGE_AIR;
        x = lara_item->pos.x - item->pos.x;
        z = lara_item->pos.z - item->pos.z;
        y = (lara_item->pos.y - 350) - (item->pos.y - WALL_L / 2);
        d = phd_sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
        if (d < WALL_L / 2) {
            d = WALL_L / 2;
        }
        x = item->pos.x + (x << WALL_SHIFT) / 2 / d;
        z = item->pos.z + (z << WALL_SHIFT) / 2 / d;
        y = item->pos.y - WALL_L / 2 + (y << WALL_SHIFT) / 2 / d;
        DoBloodSplat(x, y, z, item->speed, item->pos.y_rot, item->room_number);
    } else {
        lara_item->hit_status = 1;
        if (lara_item->hit_points > 0) {
            lara_item->hit_points = -1;
            if (lara_item->room_number != item->room_number) {
                ItemNewRoom(Lara.item_number, item->room_number);
            }

            lara_item->pos.x_rot = 0;
            lara_item->pos.z_rot = 0;
            lara_item->pos.y_rot = item->pos.y_rot;

            lara_item->current_anim_state = AS_SPECIAL;
            lara_item->goal_anim_state = AS_SPECIAL;
            lara_item->anim_number = AA_RBALL_DEATH;
            lara_item->frame_number = AF_RBALL_DEATH * ANIM_SCALE;

            Camera.flags = FOLLOW_CENTRE;
            Camera.target_angle = 170 * PHD_DEGREE;
            Camera.target_elevation = -25 * PHD_DEGREE;
            for (int i = 0; i < 15; i++) {
                x = lara_item->pos.x + (GetRandomControl() - 0x4000) / 256;
                z = lara_item->pos.z + (GetRandomControl() - 0x4000) / 256;
                y = lara_item->pos.y - GetRandomControl() / 64;
                d = item->pos.y_rot + (GetRandomControl() - 0x4000) / 8;
                DoBloodSplat(x, y, z, item->speed * 2, d, item->room_number);
            }
        }
    }
}
