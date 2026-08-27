#include <trx/game/objects.h>
#include <trx/game/savegame.h>

// Inventory items persisted in savegames, keyed by their legacy JSON names.
const SAVEGAME_INVENTORY_ENTRY g_Savegame_InventoryItems[] = {
// clang-format off
#define X_PICKUP_KEY(n) { O_KEY_ITEM_##n, "key" #n },
#define X_PICKUP_PUZZLE(n) { O_PUZZLE_ITEM_##n, "puzzle" #n },
#define X_PICKUP_PICKUP(n) { O_PICKUP_ITEM_##n, "pickup" #n },
#define X_PICKUP_QUEST(n) { O_QUEST_ITEM_##n, "quest" #n },
#define X_PICKUP_KEY_COMBO(n, c)                                               \
    { O_KEY_ITEM_##n##_COMBO_##c, "key" #n "_combo" #c },
#define X_PICKUP_PUZZLE_COMBO(n, c)                                            \
    { O_PUZZLE_ITEM_##n##_COMBO_##c, "puzzle" #n "_combo" #c },
#define X_PICKUP_PICKUP_COMBO(n, c)                                            \
    { O_PICKUP_ITEM_##n##_COMBO_##c, "pickup" #n "_combo" #c },
#define X_PICKUP_EXAMINE(n) { O_EXAMINE_ITEM_##n, "examine" #n },
#include <trx/game/objects/pickups.def>
#undef X_PICKUP_EXAMINE
#undef X_PICKUP_PICKUP_COMBO
#undef X_PICKUP_PUZZLE_COMBO
#undef X_PICKUP_KEY_COMBO
#undef X_PICKUP_QUEST
#undef X_PICKUP_PICKUP
#undef X_PICKUP_PUZZLE
#undef X_PICKUP_KEY
    { O_LEAD_BAR_ITEM, "leadbar" },
    { O_LASERSIGHT_ITEM, "lasersight" },
    { O_BINOCULARS_ITEM, "binoculars" },
    { O_CROWBAR_ITEM, "crowbar" },
    { O_WATERSKIN_1_EMPTY, "waterskin1" },
    { O_WATERSKIN_2_EMPTY, "waterskin2" },
    { O_SAVE_CRYSTAL_ITEM, "save_crystal" },
    { NO_OBJECT, nullptr },
    // clang-format on
};

const SAVEGAME_RESUME_ITEM g_Savegame_ResumeItems[] = {
    // clang-format off
    { O_SMALL_MEDIPACK_ITEM, "num_medis",         true  },
    { O_LARGE_MEDIPACK_ITEM, "num_big_medis",     true  },
    { O_FLARE_ITEM,          "num_flares",        true  },
    { O_SCION_ITEM_1,        "num_scions",        true  },
    // Introduced in TRX 1.2
    { O_QUEST_ITEM_1,        "num_quest_item_1",  false },
    { O_QUEST_ITEM_2,        "num_quest_item_2",  false },
    { O_QUEST_ITEM_3,        "num_quest_item_3",  false },
    { O_QUEST_ITEM_4,        "num_quest_item_4",  false },
    { O_QUEST_ITEM_5,        "num_quest_item_5",  false },
    { O_QUEST_ITEM_6,        "num_quest_item_6",  false },
    // Introduced in TRX 1.10
    { O_SAVE_CRYSTAL_ITEM,   "num_save_crystals", false },
    { NO_OBJECT,             nullptr,             false },
    // clang-format on
};
