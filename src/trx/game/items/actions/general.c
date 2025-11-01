#include <trx/game/game.h>
#include <trx/game/gym.h>
#include <trx/game/items.h>
#include <trx/game/rooms.h>

static void M_FinishLevel(ITEM *const item)
{
    Game_SetIsLevelComplete(true);
}

static void M_FlipMap(ITEM *const item)
{
    Room_FlipMap();
}

static void M_AssaultStart(ITEM *const item)
{
    Gym_StartAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultStop(ITEM *const item)
{
    Gym_StopAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultReset(ITEM *const item)
{
    Gym_ResetAssault();
    Room_SetFlipEffect(-1);
}

static void M_AssaultFinished(ITEM *const item)
{
    Gym_FinishAssault();
    Room_SetFlipEffect(-1);
}

REGISTER_ITEM_ACTION(ITEM_ACTION_FINISH_LEVEL, M_FinishLevel)
REGISTER_ITEM_ACTION(ITEM_ACTION_FLIP_MAP, M_FlipMap)
REGISTER_ITEM_ACTION(ITEM_ACTION_ASSAULT_RESET, M_AssaultReset)
REGISTER_ITEM_ACTION(ITEM_ACTION_ASSAULT_STOP, M_AssaultStop)
REGISTER_ITEM_ACTION(ITEM_ACTION_ASSAULT_START, M_AssaultStart)
REGISTER_ITEM_ACTION(ITEM_ACTION_ASSAULT_FINISHED, M_AssaultFinished)
