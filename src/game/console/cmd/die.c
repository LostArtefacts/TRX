#include "game/console/cmd/die.h"

#include "game/effects.h"
#include "game/effects/exploding_death.h"
#include "game/objects/common.h"
#include "game/sound.h"
#include "global/vars.h"

static COMMAND_RESULT M_Entrypoint(const char *args);

static COMMAND_RESULT M_Entrypoint(const char *args)
{
    if (!g_Objects[O_LARA].loaded) {
        return CR_UNAVAILABLE;
    }

    if (g_LaraItem->hit_points <= 0) {
        return CR_UNAVAILABLE;
    }

    Effect_ExplodingDeath(g_Lara.item_num, -1, 0);
    Sound_Effect(SFX_EXPLOSION_CHEAT, &g_LaraItem->pos, SPM_NORMAL);
    Sound_Effect(SFX_LARA_FALL, &g_LaraItem->pos, SPM_NORMAL);
    g_LaraItem->hit_points = 0;
    g_LaraItem->flags |= IS_INVISIBLE;
    return CR_SUCCESS;
}

CONSOLE_COMMAND g_Console_Cmd_Die = {
    .prefix = "abortion|natlastinks",
    .proc = M_Entrypoint,
};
