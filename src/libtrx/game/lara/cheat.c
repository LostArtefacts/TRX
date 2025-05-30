#include "game/lara/cheat.h"

#include "game/console.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/sound.h"

static void M_GiveAllKeysImpl(void);

static void M_GiveAllKeysImpl(void)
{
    Inv_AddItem(O_PUZZLE_ITEM_1);
    Inv_AddItem(O_PUZZLE_ITEM_2);
    Inv_AddItem(O_PUZZLE_ITEM_3);
    Inv_AddItem(O_PUZZLE_ITEM_4);
    Inv_AddItem(O_KEY_ITEM_1);
    Inv_AddItem(O_KEY_ITEM_2);
    Inv_AddItem(O_KEY_ITEM_3);
    Inv_AddItem(O_KEY_ITEM_4);
    Inv_AddItem(O_PICKUP_ITEM_1);
    Inv_AddItem(O_PICKUP_ITEM_2);
#if TR_VERSION == 1
    Inv_AddItem(O_LEADBAR_ITEM);
#endif
}

bool Lara_Cheat_GiveAllKeys(void)
{
    if (Lara_GetItem() == nullptr) {
        return false;
    }

    M_GiveAllKeysImpl();

    Sound_Effect(SFX_LARA_KEY, nullptr, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_KEYS));
    return true;
}

void Lara_Cheat_EndLevel(void)
{
    Game_SetIsLevelComplete(true);
    Console_Log(GS(OSD_COMPLETE_LEVEL));
}
