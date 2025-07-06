#include "game/lara/control.h"

#include "game/gun/gun.h"
#include "game/inventory.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "game/stats.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/lara.h>

void Lara_InitialiseLoad(const int16_t item_num)
{
    g_Lara.item_num = item_num;
    g_LaraItem = Item_Get(item_num);
}
