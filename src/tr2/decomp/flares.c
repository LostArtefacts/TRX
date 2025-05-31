#include "decomp/flares.h"

#include "game/gun/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/lara/flare.h"
#include "game/lara/misc.h"
#include "game/output.h"
#include "game/random.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/lara/common.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

#define M_FLARE_INTENSITY 12
#define M_FLARE_FALL_OFF 11
#define M_MAX_FLARE_AGE (60 * LOGIC_FPS) // = 1800
#define M_FLARE_OLD_AGE (M_MAX_FLARE_AGE - 2 * LOGIC_FPS) // = 1740
#define M_FLARE_YOUNG_AGE (LOGIC_FPS) // = 30

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

void Flare_Create(const bool thrown)
{
    const int16_t item_num = Item_Create();
    if (item_num == NO_ITEM) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    item->object_id = O_FLARE_ITEM;
    item->room_num = g_LaraItem->room_num;

    XYZ_32 vec = {
        .x = -16,
        .y = 32,
        .z = 42,
    };
    Lara_GetJointAbsPosition(&vec, LM_HAND_L);

    const SECTOR *const sector =
        Room_GetSector(vec.x, vec.y, vec.z, &item->room_num);
    const int32_t height = Room_GetHeight(sector, vec.x, vec.y, vec.z);
    if (height < vec.y) {
        item->pos.x = g_LaraItem->pos.x;
        item->pos.y = vec.y;
        item->pos.z = g_LaraItem->pos.z;
        item->rot.y = -g_LaraItem->rot.y;
        item->room_num = g_LaraItem->room_num;
    } else {
        item->pos.x = vec.x;
        item->pos.y = vec.y;
        item->pos.z = vec.z;
        if (thrown) {
            item->rot.y = g_LaraItem->rot.y;
        } else {
            item->rot.y = g_LaraItem->rot.y - DEG_45;
        }
    }

    Item_Initialise(item_num);

    item->rot.z = 0;
    item->rot.x = 0;
    item->shade.value_1 = -1;

    if (thrown) {
        item->speed = g_LaraItem->speed + 50;
        item->fall_speed = g_LaraItem->fall_speed - 50;
    } else {
        item->speed = g_LaraItem->speed + 10;
        item->fall_speed = g_LaraItem->fall_speed + 50;
    }

    if (Flare_GenerateLight(item->pos, g_Lara.flare.age)) {
        item->data = (void *)(intptr_t)(g_Lara.flare.age | 0x8000);
    } else {
        item->data = (void *)(intptr_t)(g_Lara.flare.age & ~0x8000);
    }

    Item_AddActive(item_num);
    item->status = IS_ACTIVE;
}

int32_t Flare_GetMaxAge(void)
{
    return M_MAX_FLARE_AGE;
}
