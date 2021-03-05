#include "game/box.h"
#include "game/wolf.h"
#include "game/vars.h"
#include "util.h"

#define WOLF_SLEEP_FRAME 96

void InitialiseWolf(int16_t item_num)
{
    Items[item_num].frame_number = WOLF_SLEEP_FRAME;
    InitialiseCreature(item_num);
}

void T1MInjectGameWolf()
{
    INJECT(0x0043DF20, InitialiseWolf);
}
