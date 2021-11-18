#include "game/traps/lava.h"

#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/random.h"
#include "game/sound.h"
#include "global/vars.h"

void SetupLavaEmitter(OBJECT_INFO *obj)
{
    obj->control = LavaEmitterControl;
    obj->draw_routine = DrawDummyItem;
    obj->collision = ObjectCollision;
}

void SetupLava(OBJECT_INFO *obj)
{
    obj->control = LavaControl;
}

void SetupLavaWedge(OBJECT_INFO *obj)
{
    obj->control = LavaWedgeControl;
    obj->collision = CreatureCollision;
    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}

void LavaBurn(ITEM_INFO *item)
{
    if (Lara.water_status == LWS_CHEAT) {
        return;
    }

    if (item->hit_points < 0) {
        return;
    }

    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = GetFloor(item->pos.x, 32000, item->pos.z, &room_num);

    if (item->floor != GetHeight(floor, item->pos.x, 32000, item->pos.z)) {
        return;
    }

    item->hit_points = -1;
    item->hit_status = 1;
    for (int i = 0; i < 10; i++) {
        int16_t fx_num = CreateEffect(item->room_number);
        if (fx_num != NO_ITEM) {
            FX_INFO *fx = &Effects[fx_num];
            fx->object_number = O_FLAME;
            fx->frame_number =
                (Objects[O_FLAME].nmeshes * Random_GetControl()) / 0x7FFF;
            fx->counter = -1 - Random_GetControl() * 24 / 0x7FFF;
        }
    }
}

void LavaEmitterControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];
    int16_t fx_num = CreateEffect(item->room_number);
    if (fx_num != NO_ITEM) {
        FX_INFO *fx = &Effects[fx_num];
        fx->pos.x = item->pos.x;
        fx->pos.y = item->pos.y;
        fx->pos.z = item->pos.z;
        fx->pos.y_rot = (Random_GetControl() - 0x4000) * 2;
        fx->speed = Random_GetControl() >> 10;
        fx->fall_speed = -Random_GetControl() / 200;
        fx->frame_number = -4 * Random_GetControl() / 0x7FFF;
        fx->object_number = O_LAVA;
        Sound_Effect(SFX_LAVA_FOUNTAIN, &item->pos, SPM_NORMAL);
    }
}

void LavaControl(int16_t fx_num)
{
    FX_INFO *fx = &Effects[fx_num];
    fx->pos.z += (fx->speed * phd_cos(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->pos.x += (fx->speed * phd_sin(fx->pos.y_rot)) >> W2V_SHIFT;
    fx->fall_speed += GRAVITY;
    fx->pos.y += fx->fall_speed;

    int16_t room_num = fx->room_number;
    FLOOR_INFO *floor = GetFloor(fx->pos.x, fx->pos.y, fx->pos.z, &room_num);
    if (fx->pos.y >= GetHeight(floor, fx->pos.x, fx->pos.y, fx->pos.z)
        || fx->pos.y < GetCeiling(floor, fx->pos.x, fx->pos.y, fx->pos.z)) {
        KillEffect(fx_num);
    } else if (ItemNearLara(&fx->pos, 200)) {
        LaraItem->hit_points -= LAVA_GLOB_DAMAGE;
        LaraItem->hit_status = 1;
        KillEffect(fx_num);
    } else if (room_num != fx->room_number) {
        EffectNewRoom(fx_num, room_num);
    }
}

void LavaWedgeControl(int16_t item_num)
{
    ITEM_INFO *item = &Items[item_num];

    int16_t room_num = item->room_number;
    GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    if (room_num != item->room_number) {
        ItemNewRoom(item_num, room_num);
    }

    if (item->status != IS_DEACTIVATED) {
        int32_t x = item->pos.x;
        int32_t z = item->pos.z;

        switch (item->pos.y_rot) {
        case 0:
            item->pos.z += LAVA_WEDGE_SPEED;
            z += 2 * WALL_L;
            break;
        case -PHD_180:
            item->pos.z -= LAVA_WEDGE_SPEED;
            z -= 2 * WALL_L;
            break;
        case PHD_90:
            item->pos.x += LAVA_WEDGE_SPEED;
            x += 2 * WALL_L;
            break;
        default:
            item->pos.x -= LAVA_WEDGE_SPEED;
            x -= 2 * WALL_L;
            break;
        }

        FLOOR_INFO *floor = GetFloor(x, item->pos.y, z, &room_num);
        if (GetHeight(floor, x, item->pos.y, z) != item->pos.y) {
            item->status = IS_DEACTIVATED;
        }
    }

    if (Lara.water_status == LWS_CHEAT) {
        item->touch_bits = 0;
    }

    if (item->touch_bits) {
        if (LaraItem->hit_points > 0) {
            LavaBurn(LaraItem);
        }

        Camera.item = item;
        Camera.flags = CHASE_OBJECT;
        Camera.type = CAM_FIXED;
        Camera.target_angle = -PHD_180;
        Camera.target_distance = WALL_L * 3;
    }
}
