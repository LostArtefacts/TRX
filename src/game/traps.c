#include "3dsystem/phd_math.h"
#include "game/control.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/items.h"
#include "game/sphere.h"
#include "game/traps.h"
#include "game/vars.h"
#include "specific/init.h"
#include "util.h"

void InitialiseRollingBall(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    GAME_VECTOR* old = game_malloc(sizeof(GAME_VECTOR), GBUF_ROLLINGBALL_STUFF);
    item->data = old;
    old->x = item->pos.x;
    old->y = item->pos.y;
    old->z = item->pos.z;
    old->room_number = item->room_number;
}

void RollingBallControl(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];
    if (item->status == IS_ACTIVE) {
        if (item->pos.y < item->floor) {
            if (!item->gravity_status) {
                item->gravity_status = 1;
                item->fall_speed = -10;
            }
        } else if (item->current_anim_state == TRAP_SET) {
            item->goal_anim_state = TRAP_ACTIVATE;
        }

        int32_t oldx = item->pos.x;
        int32_t oldz = item->pos.z;
        AnimateItem(item);

        int16_t room_num = item->room_number;
        FLOOR_INFO* floor =
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
        GAME_VECTOR* old = item->data;
        item->pos.x = old->x;
        item->pos.y = old->y;
        item->pos.z = old->z;
        if (item->room_number != old->room_number) {
            RemoveDrawnItem(item_num);
            ROOM_INFO* r = &RoomInfo[old->room_number];
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

void FlameControl(int16_t fx_num)
{
    FX_INFO* fx = &Effects[fx_num];

    fx->frame_number--;
    if (fx->frame_number <= Objects[O_FLAME].nmeshes) {
        fx->frame_number = 0;
    }

    if (fx->counter < 0) {
#ifdef T1M_FEAT_CHEATS
        if (Lara.water_status == LWS_CHEAT) {
            fx->counter = 0;
            StopSoundEffect(150, NULL);
            KillEffect(fx_num);
        }
#endif

        fx->pos.x = 0;
        fx->pos.y = 0;
        if (fx->counter == -1) {
            fx->pos.z = -100;
        } else {
            fx->pos.z = 0;
        }

        GetJointAbsPosition(LaraItem, (PHD_VECTOR*)&fx->pos, -1 - fx->counter);

#ifdef T1M_FEAT_OG_FIXES
        int32_t y = GetWaterHeight(
            LaraItem->pos.x, LaraItem->pos.y, LaraItem->pos.z,
            LaraItem->room_number);
#else
        int32_t y =
            GetWaterHeight(fx->pos.x, fx->pos.y, fx->pos.z, fx->room_number);
#endif

        if (y != NO_HEIGHT && fx->pos.y > y) {
            fx->counter = 0;
            StopSoundEffect(150, NULL);
            KillEffect(fx_num);
        } else {
            SoundEffect(150, &fx->pos, 0);
            LaraItem->hit_points -= FLAME_ONFIRE_DAMAGE;
            LaraItem->hit_status = 1;
        }
        return;
    }

    SoundEffect(150, &fx->pos, 0);
    if (fx->counter) {
        fx->counter--;
    } else if (ItemNearLara(&fx->pos, 600)) {
#ifdef T1M_FEAT_CHEATS
        if (Lara.water_status == LWS_CHEAT) {
            return;
        }
#endif

        int32_t x = LaraItem->pos.x - fx->pos.x;
        int32_t z = LaraItem->pos.z - fx->pos.z;
        int32_t distance = SQUARE(x) + SQUARE(z);

        LaraItem->hit_points -= FLAME_TOONEAR_DAMAGE;
        LaraItem->hit_status = 1;

        if (distance < SQUARE(300)) {
            fx->counter = 100;

            fx_num = CreateEffect(LaraItem->room_number);
            if (fx_num != -1) {
                fx = &Effects[fx_num];
                fx->frame_number = 0;
                fx->object_number = O_FLAME;
                fx->counter = -1;
            }
        }
    }
}

void LavaBurn(ITEM_INFO* item)
{
#ifdef T1M_FEAT_CHEATS
    if (Lara.water_status == LWS_CHEAT) {
        return;
    }
#endif

    if (item->hit_points < 0) {
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO* floor = GetFloor(item->pos.x, 32000, item->pos.z, &room_num);

    if (item->floor != GetHeight(floor, item->pos.x, 32000, item->pos.z)) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;
    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO* fx = &Effects[fx_num];
            fx->object_number = O_FLAME;
            fx->frame_number =
                (Objects[O_FLAME].nmeshes * GetRandomControl()) / 0x7FFF;
            fx->counter = -1 - GetRandomControl() * 24 / 0x7FFF;
        }
    }
}

void T1MInjectGameTraps()
{
    INJECT(0x0043A010, InitialiseRollingBall);
    INJECT(0x0043A050, RollingBallControl);
    INJECT(0x0043B2A0, FlameControl);
    INJECT(0x0043B430, LavaBurn);
}
