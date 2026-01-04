#include <trx/game/footprint_fx.h>
#include <trx/game/items/actions.h>

static void M_Footprint(ITEM *const item)
{
    if (item == nullptr) {
        return;
    }

    const bool is_left = ItemAction_GetFXType() == 0x4000;
    FootprintFX_Add(item, is_left);
}

REGISTER_ITEM_ACTION(ITEM_ACTION_FOOTPRINT, M_Footprint)
