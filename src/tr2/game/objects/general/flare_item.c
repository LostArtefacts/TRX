#include "game/objects/general/flare_item.h"

#include "game/objects/general/pickup.h"

#include <libtrx/game/matrix.h>
#include <libtrx/game/objects.h>
#include <libtrx/game/output.h>
#include <libtrx/game/random.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/spawn.h>
#include <libtrx/utils.h>

#define M_FLARE_INTENSITY 12
#define M_FLARE_FALL_OFF 11
#define M_MAX_FLARE_AGE (60 * LOGIC_FPS) // = 1800
#define M_FLARE_OLD_AGE (M_MAX_FLARE_AGE - 2 * LOGIC_FPS) // = 1740
#define M_FLARE_YOUNG_AGE (LOGIC_FPS) // = 30

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->fall_speed) {
        item->rot.x += DEG_1 * 3;
        item->rot.z += DEG_1 * 5;
    } else {
        item->rot.x = 0;
        item->rot.z = 0;
    }

    const int32_t x = item->pos.x;
    const int32_t y = item->pos.y;
    const int32_t z = item->pos.z;
    item->pos.z += (item->speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;
    item->pos.x += (item->speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;

    if (Room_Get(item->room_num)->flags & RF_UNDERWATER) {
        item->fall_speed += (5 - item->fall_speed) / 2;
        item->speed = item->speed + (5 - item->speed) / 2;
    } else {
        item->fall_speed += GRAVITY;
    }
    item->pos.y += item->fall_speed;

    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int32_t height =
        Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
    if (item->pos.y < height) {
        const int32_t ceiling =
            Room_GetCeiling(sector, item->pos.x, item->pos.y, item->pos.z);
        if (item->pos.y < ceiling) {
            item->fall_speed = -item->fall_speed;
            item->pos.y = ceiling;
        }
    } else {
        if (y > height) {
            item->pos.x = x;
            item->pos.y = y;
            item->pos.z = z;
            item->rot.y += DEG_180;
            item->speed /= 2;
            room_num = item->room_num;
        } else {
            if (item->fall_speed > 40) {
                item->fall_speed = 40 - item->fall_speed;
                CLAMPL(item->fall_speed, -100);
            } else {
                item->fall_speed = 0;
                item->speed -= 3;
                CLAMPL(item->speed, 0);
            }
            item->pos.y = height;
        }
    }

    Item_UpdateRoom(item_num, room_num);

    int32_t flare_age = ((int32_t)(intptr_t)item->data) & 0x7FFF;
    if (flare_age < Flare_GetMaxAge()) {
        flare_age++;
    } else if (item->fall_speed == 0 && item->speed == 0) {
        Item_Kill(item_num);
        return;
    }

    if (Flare_GenerateLight(item->pos, flare_age)) {
        flare_age |= 0x8000u;
        Flare_GenerateEffects(item->pos, item->pos, item->room_num);
    }

    item->data = (void *)(intptr_t)flare_age;
}

static void M_Draw(const ITEM *const item)
{
    int32_t rate;
    ANIM_FRAME *frames[2];
    Item_GetFrames(item, frames, &rate);
    Matrix_Push();
    Matrix_TranslateAbs32(item->interp.result.pos);
    Matrix_Rot16(item->interp.result.rot);
    const CLIP clip = Output_CheckBoundsClip(&frames[0]->bounds);

    const XYZ_32 flare_size = {
        .x = frames[0]->bounds.max.x - frames[0]->bounds.min.x,
        .y = frames[0]->bounds.max.y - frames[0]->bounds.min.y,
        .z = frames[0]->bounds.max.z - frames[0]->bounds.min.z,
    };
    const XYZ_32 flare_offset = {
        .x = -flare_size.x,
        .y = -flare_size.y,
        .z = -flare_size.z,
    };
    Matrix_TranslateRel32(flare_offset);

    if (clip != CLIP_NOT_VISIBLE) {
        Output_CalculateObjectLighting(item, &frames[0]->bounds);
        Object_DrawMesh(Object_Get(O_FLARE_ITEM)->mesh_idx, clip, false);
        if (((int32_t)(intptr_t)item->data) & 0x8000) {
            Matrix_TranslateRel(-6, 6, 80);
            Matrix_RotX(-90 * DEG_1);
            Matrix_RotY((int16_t)(2 * Random_GetDraw()));
            Output_CalculateStaticLight(8 * 256);
            Object_DrawMesh(Object_Get(O_FLARE_FIRE)->mesh_idx, clip, false);
        }
    }
    Matrix_Pop();
}

void Flare_GenerateEffects(
    const XYZ_32 sound_pos, const XYZ_32 flare_pos, int16_t room_num)
{
    Room_GetSector(flare_pos.x, flare_pos.y, flare_pos.z, &room_num);
    if ((Room_Get(room_num)->flags & RF_UNDERWATER) != 0) {
        Sound_Effect(SFX_LARA_FLARE_BURN, &sound_pos, SPM_UNDERWATER);
        if (Random_GetDraw() < 0x4000) {
            Spawn_Bubble(&flare_pos, room_num);
        }
    } else {
        Sound_Effect(SFX_LARA_FLARE_BURN, &sound_pos, SPM_NORMAL);
    }
}

bool Flare_GenerateLight(const XYZ_32 pos, const int32_t flare_age)
{
    if (flare_age >= M_MAX_FLARE_AGE) {
        return false;
    }

    const int32_t random = Random_GetDraw();
    const XYZ_32 light_pos = {
        .x = pos.x + (random & 0xA0),
        .y = pos.y,
        .z = pos.z,
    };

    if (flare_age < M_FLARE_YOUNG_AGE) {
        const int32_t intensity = M_FLARE_INTENSITY
                * (flare_age - M_FLARE_YOUNG_AGE) / (2 * M_FLARE_YOUNG_AGE)
            + M_FLARE_INTENSITY;
        Output_AddDynamicLight(light_pos, intensity, M_FLARE_FALL_OFF);
        return true;
    }

    if (flare_age < M_FLARE_OLD_AGE) {
        Output_AddDynamicLight(light_pos, M_FLARE_INTENSITY, M_FLARE_FALL_OFF);
        return true;
    }

    if (random > 0x2000) {
        Output_AddDynamicLight(
            light_pos, M_FLARE_INTENSITY - (random & 3), M_FLARE_FALL_OFF);
        return true;
    }

    Output_AddDynamicLight(light_pos, M_FLARE_INTENSITY, M_FLARE_FALL_OFF / 2);
    return false;
}

int32_t Flare_GetMaxAge(void)
{
    return M_MAX_FLARE_AGE;
}

static void M_Setup(OBJECT *const obj)
{
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;
    obj->save_position = true;
    obj->save_flags = true;

    if (obj->loaded) {
        for (int32_t i = 0; i < obj->mesh_count; i++) {
            OBJECT_MESH *const obj_mesh = Object_GetMesh(obj->mesh_idx + i);
            obj_mesh->depth_adjustment = -0.5;
        }
    }
}

REGISTER_OBJECT(O_FLARE_ITEM, M_Setup)
