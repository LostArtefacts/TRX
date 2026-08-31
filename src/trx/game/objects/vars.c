#include <trx/game/objects/vars.h>

const GAME_OBJECT_PAIR g_KeyItemToReceptacleMap[] = {
#define X_RECEPTACLE(option, receptacle) { option, receptacle },
#include <trx/game/objects/pickups.def>
#undef X_RECEPTACLE
    // clang-format off
    { O_LEAD_BAR_OPTION, O_MIDAS_TOUCH },
    { O_KEY_OPTION_2, O_GONG },
    { O_KEY_OPTION_2, O_DETONATOR_BOX },
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_ReceptacleToReceptacleDoneMap[] = {
#define X_RECEPTACLE_DONE(receptacle, done) { receptacle, done },
#include <trx/game/objects/pickups.def>
#undef X_RECEPTACLE_DONE
    // clang-format off
    { NO_OBJECT, NO_OBJECT },
    // clang-format on
};

const GAME_OBJECT_PAIR g_GunAmmoObjectMap[] = {
#define X_PICKUP_GUN_AMMO(gun_item, item, option) { gun_item, item },
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_GUN_AMMO
    { NO_OBJECT, NO_OBJECT },
};

const GAME_OBJECT_PAIR g_ItemToInvObjectMap[] = {
#define X_PICKUP(item, option) { item, option },
#include <trx/game/objects/pickups.def>
#undef X_PICKUP
    // The crystal is collected by its own control routine rather than the
    // generic pickup code, so it is not part of pickups.def.
    { O_SAVE_CRYSTAL_ITEM, O_SAVE_CRYSTAL_OPTION },
    { NO_OBJECT, NO_OBJECT },
};
