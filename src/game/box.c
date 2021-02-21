#include "game/box.h"
#include "game/data.h"
#include "game/game.h"
#include "util.h"

void __cdecl InitialiseCreature(int16_t item_num)
{
    ITEM_INFO* item = &Items[item_num];

    item->pos.y_rot += (PHD_ANGLE)((GetRandomControl() - 0x4000) >> 1);
    item->collidable = 1;
    item->data = NULL;
}

void Tomb1MInjectGameBox()
{
    INJECT(0x0040DA60, InitialiseCreature);
}
