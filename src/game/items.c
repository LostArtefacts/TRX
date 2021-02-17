#include "game/const.h"
#include "game/data.h"
#include "game/items.h"
#include "util.h"

void __cdecl InitialiseFXArray()
{
    TRACE("");
    NextFxActive = NO_ITEM;
    NextFxFree = 0;
    FX_INFO* fx = Effects;
    for (int i = 1; i < NUM_EFFECTS; i++, fx++) {
        fx->next_fx = i;
    }
    fx->next_fx = NO_ITEM;
}

void TR1MInjectItems()
{
    INJECT(0x00422250, InitialiseFXArray);
}
