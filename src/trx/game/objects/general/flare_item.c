#include <trx/game/objects/general/flare_item.h>

#include <trx/game/gun.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/utils.h>
#include <trx/version.h>

// clang-format off
#define M_FLARE_INTENSITY 12
#define M_FLARE_FALL_OFF  11
#define M_MAX_FLARE_AGE   (60 * LOGIC_FPS)                  // = 1800
#define M_FLARE_OLD_AGE   (M_MAX_FLARE_AGE - 2 * LOGIC_FPS) // = 1740
#define M_FLARE_YOUNG_AGE (LOGIC_FPS)                       // = 30
// clang-format off

static XYZ_32 M_TransformLocalOffset(
    const XYZ_32 pos, const XYZ_16 rot, const XYZ_32 local_offset)
{
    Matrix_PushUnit();
    Matrix_TranslateAbs32(pos);
    Matrix_Rot16(rot);
    Matrix_TranslateRel32(local_offset);
    const XYZ_32 out = {
        .x = g_WMatrixPtr->_03 >> W2V_SHIFT,
        .y = g_WMatrixPtr->_13 >> W2V_SHIFT,
        .z = g_WMatrixPtr->_23 >> W2V_SHIFT,
    };
    Matrix_Pop();
    return out;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const ROOM *const room = Room_Get(item->room_num);
    if (room->flags.swamp) {
        Item_Kill(item_num);
        return;
    }

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

    if (room->flags.underwater) {
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
        Flare_GenerateEffects(&item->pos, item->pos, item->room_num);
    }

    item->data = (void *)(intptr_t)flare_age;
}

static void M_DrawFlash(const CLIP clip)
{
    WEAPON_INFO *const flare_info = &g_Weapons[LGT_FLARE];
    SWAP(flare_info->flash_pos, flare_info->flash_pos_alt);
    Gun_DrawFlash(LGT_FLARE, clip, false);
    SWAP(flare_info->flash_pos, flare_info->flash_pos_alt);
}

static bool M_Draw(const ITEM *const item)
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

    if (clip == CLIP_NOT_VISIBLE) {
        goto end;
    }

    Output_CalculateObjectLighting(item, &frames[0]->bounds);
    Object_DrawMesh(Object_Get(O_FLARE_ITEM)->mesh_idx, clip, false);
    if ((((int32_t)(intptr_t)item->data) & 0x8000) == 0) {
        goto end;
    }

    if (g_TRVersion < 3) {
        M_DrawFlash(clip);
        goto end;
    }

    const XYZ_32 flare_pos = {
        .x = item->interp.result.pos.x,
        .y = item->interp.result.pos.y,
        .z = item->interp.result.pos.z,
    };
    const XYZ_32 tip_local = { .x = -6, .y = 6, .z = 32 };
    const XYZ_32 tip_pos = M_TransformLocalOffset(
        flare_pos, item->interp.result.rot, tip_local);

    const XYZ_32 vel_local = {
        .x = (Random_GetDraw() & 0x7F) - 64,
        .y = (Random_GetDraw() & 0x7F) - 64,
        .z = (Random_GetDraw() & 0x1FF) + 512,
    };
    const XYZ_32 vel_pos = M_TransformLocalOffset(
        flare_pos, item->interp.result.rot, vel_local);
    const XYZ_32 vel = {
        .x = vel_pos.x - flare_pos.x,
        .y = vel_pos.y - flare_pos.y,
        .z = vel_pos.z - flare_pos.z,
    };

    for (int32_t i = 0; i < (Random_GetDraw() & 3) + 4; i++) {
        const bool smoke = (i >> 2) != 0;
        Sparks_TriggerFlareSparks(tip_pos, vel, smoke);
    }

end:
    Matrix_Pop();
    return true;
}

void Flare_GenerateEffects(
    const XYZ_32 *const sound_pos, const XYZ_32 flare_pos, int16_t room_num)
{
    Room_GetSector(flare_pos.x, flare_pos.y, flare_pos.z, &room_num);
    if (Room_Get(room_num)->flags.underwater) {
        Sound_Effect(SFX_LARA_FLARE_BURN, sound_pos, SPM_UNDERWATER);
        if (Random_GetDraw() < 0x4000) {
            Spawn_Bubble(&flare_pos, room_num);
        }
    } else {
        Sound_Effect(SFX_LARA_FLARE_BURN, sound_pos, SPM_NORMAL);
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
