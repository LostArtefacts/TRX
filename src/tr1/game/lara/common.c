#include "game/lara/common.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/gun.h"
#include "game/input.h"
#include "game/inventory.h"
#include "game/item_actions.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/math.h>
#include <libtrx/log.h>
#include <libtrx/utils.h>

LARA_INFO *Lara_GetLaraInfo(void)
{
    return &g_Lara;
}

ITEM *Lara_GetItem(void)
{
    return g_LaraItem;
}

void Lara_InitialiseLoad(int16_t item_num)
{
    g_Lara.item_num = item_num;
    if (item_num == NO_ITEM) {
        g_LaraItem = nullptr;
    } else {
        g_LaraItem = Item_Get(item_num);
    }
}
