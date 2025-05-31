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

int32_t Flare_GetMaxAge(void)
{
    return M_MAX_FLARE_AGE;
}
