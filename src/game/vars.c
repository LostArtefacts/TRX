#include "game/vars.h"

#ifdef T1M_FEAT_GAMEPLAY
int16_t StoredLaraHealth = 0;
#endif

#ifdef T1M_FEAT_UI
int16_t BarOffsetY[6];
#endif

char *GF_LevelNames[LV_NUMBER_OF] = {
    "data\\gym.phd",      "data\\level1.phd",   "data\\level2.phd",
    "data\\level3a.phd",  "data\\level3b.phd",  "data\\level4.phd",
    "data\\level5.phd",   "data\\level6.phd",   "data\\level7a.phd",
    "data\\level7b.phd",  "data\\level8a.phd",  "data\\level8b.phd",
    "data\\level8c.phd",  "data\\level10a.phd", "data\\level10b.phd",
    "data\\level10c.phd", "data\\cut1.phd",     "data\\cut2.phd",
    "data\\cut3.phd",     "data\\cut4.phd",     "data\\title.phd",
    "data\\current.phd",
};

char *GF_LevelTitles[LV_NUMBER_OF] = {
    "Lara's Home",
    "Caves",
    "City of Vilcabamba",
    "Lost Valley",
    "Tomb of Qualopec",
    "St. Francis' Folly",
    "Colosseum",
    "Palace Midas",
    "The Cistern",
    "Tomb of Tihocan",
    "City of Khamoon",
    "Obelisk of Khamoon",
    "Sanctuary of the Scion",
    "Natla's Mines",
    "Atlantis",
    "The Great Pyramid",
    "Cut Scene 1",
    "Cut Scene 2",
    "Cut Scene 3",
    "Cut Scene 4",
    "Title",
    "Current Position",
};

char *GF_Key1Strings[LV_NUMBER_OF] = {
    NULL,        NULL, "Silver Key", NULL,       NULL,          "Neptune Key",
    "Rusty Key", NULL, "Gold Key",   "Gold Key", "Saphire Key", "Saphire Key",
    "Gold Key",  NULL, NULL,         NULL,       NULL,          NULL,
    NULL,        NULL, NULL,         NULL,
};

char *GF_Key2Strings[LV_NUMBER_OF] = {
    NULL,         NULL,        NULL, NULL, NULL, "Atlas Key", NULL, NULL,
    "Silver Key", "Rusty Key", NULL, NULL, NULL, NULL,        NULL, NULL,
    NULL,         NULL,        NULL, NULL, NULL, NULL,
};

char *GF_Key3Strings[LV_NUMBER_OF] = {
    NULL,        NULL,        NULL, NULL, NULL, "Damocles Key", NULL, NULL,
    "Rusty Key", "Rusty Key", NULL, NULL, NULL, NULL,           NULL, NULL,
    NULL,        NULL,        NULL, NULL, NULL, NULL,
};

char *GF_Key4Strings[LV_NUMBER_OF] = {
    NULL, NULL, NULL, NULL, NULL, "Thor Key", NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL,       NULL, NULL, NULL, NULL, NULL,
};

char *GF_Puzzle1Strings[LV_NUMBER_OF] = {
    NULL, NULL, "Gold Idol", "Machine Cog",  NULL,   NULL,   NULL, "Gold Bar",
    NULL, NULL, NULL,        "Eye of Horus", "Ankh", "Fuse", NULL, NULL,
    NULL, NULL, NULL,        NULL,           NULL,   NULL,
};

char *GF_Puzzle2Strings[LV_NUMBER_OF] = {
    NULL, NULL, NULL, NULL,     NULL,     NULL,          NULL, NULL,
    NULL, NULL, NULL, "Scarab", "Scarab", "Pyramid Key", NULL, NULL,
    NULL, NULL, NULL, NULL,     NULL,     NULL,
};

char *GF_Puzzle3Strings[LV_NUMBER_OF] = {
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, "Seal of Anubis",
    NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL,
};

char *GF_Puzzle4Strings[LV_NUMBER_OF] = {
    NULL,   NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    "Ankh", NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

char *GF_Pickup1Strings[LV_NUMBER_OF] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

char *GF_Pickup2Strings[LV_NUMBER_OF] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
};

char *GF_GameStrings[GS_NUMBER_OF] = {
    [GS_HEADING_INVENTORY] = "INVENTORY",
    [GS_HEADING_GAME_OVER] = "GAME OVER",
    [GS_HEADING_OPTION] = "OPTION",
    [GS_HEADING_ITEMS] = "ITEMS",
    [GS_PASSPORT_SELECT_LEVEL] = "Select Level",
    [GS_PASSPORT_SELECT_MODE] = "Select Mode",
    [GS_PASSPORT_MODE_NEW_GAME] = "New Game",
    [GS_PASSPORT_MODE_NEW_GAME_PLUS] = "New Game+",
    [GS_PASSPORT_NEW_GAME] = "New Game",
    [GS_PASSPORT_LOAD_GAME] = "Load Game",
    [GS_PASSPORT_SAVE_GAME] = "Save Game",
    [GS_PASSPORT_EXIT_GAME] = "Exit Game",
    [GS_PASSPORT_EXIT_TO_TITLE] = "Exit to Title",
    [GS_DETAIL_SELECT_DETAIL] = "Select Detail",
    [GS_DETAIL_LEVEL_HIGH] = "High",
    [GS_DETAIL_LEVEL_MEDIUM] = "Medium",
    [GS_DETAIL_LEVEL_LOW] = "Low",
    [GS_DETAIL_PERSPECTIVE_FMT] = "Perspective     %s",
    [GS_DETAIL_BILINEAR_FMT] = "Bilinear        %s",
    [GS_DETAIL_VIDEO_MODE_FMT] = "Game Video Mode %s",
    [GS_SOUND_SET_VOLUMES] = "Set Volumes",
    [GS_CONTROL_DEFAULT_KEYS] = "Default Keys",
    [GS_CONTROL_USER_KEYS] = "User Keys",
    [GS_KEYMAP_RUN] = "Run",
    [GS_KEYMAP_BACK] = "Back",
    [GS_KEYMAP_LEFT] = "Left",
    [GS_KEYMAP_RIGHT] = "Right",
    [GS_KEYMAP_STEP_LEFT] = "Step Left",
    [GS_KEYMAP_STEP_RIGHT] = "Step Right",
    [GS_KEYMAP_WALK] = "Walk",
    [GS_KEYMAP_JUMP] = "Jump",
    [GS_KEYMAP_ACTION] = "Action",
    [GS_KEYMAP_DRAW_WEAPON] = "Draw Weapon",
    [GS_KEYMAP_LOOK] = "Look",
    [GS_KEYMAP_ROLL] = "Roll",
    [GS_KEYMAP_INVENTORY] = "Inventory",
    [GS_STATS_TIME_TAKEN_FMT] = "TIME TAKEN %s",
    [GS_STATS_SECRETS_FMT] = "SECRETS %d OF %d",
    [GS_STATS_PICKUPS_FMT] = "PICKUPS %d",
    [GS_STATS_KILLS_FMT] = "KILLS %d",
    [GS_MISC_ON] = "On",
    [GS_MISC_OFF] = "Off",
    [GS_MISC_EMPTY_SLOT_FMT] = "- EMPTY SLOT %d -",
    [GS_MISC_DEMO_MODE] = "Demo Mode",
    [GS_INV_ITEM_MEDI] = "Small Medi Pack",
    [GS_INV_ITEM_BIG_MEDI] = "Large Medi Pack",
    [GS_INV_ITEM_PUZZLE1] = "Puzzle",
    [GS_INV_ITEM_PUZZLE2] = "Puzzle",
    [GS_INV_ITEM_PUZZLE3] = "Puzzle",
    [GS_INV_ITEM_PUZZLE4] = "Puzzle",
    [GS_INV_ITEM_KEY1] = "Key",
    [GS_INV_ITEM_KEY2] = "Key",
    [GS_INV_ITEM_KEY3] = "Key",
    [GS_INV_ITEM_KEY4] = "Key",
    [GS_INV_ITEM_PICKUP1] = "Pickup",
    [GS_INV_ITEM_PICKUP2] = "Pickup",
    [GS_INV_ITEM_LEADBAR] = "Lead Bar",
    [GS_INV_ITEM_SCION] = "Scion",
    [GS_INV_ITEM_PISTOLS] = "Pistols",
    [GS_INV_ITEM_SHOTGUN] = "Shotgun",
    [GS_INV_ITEM_MAGNUM] = "Magnums",
    [GS_INV_ITEM_UZI] = "Uzis",
    [GS_INV_ITEM_GRENADE] = "Grenade",
    [GS_INV_ITEM_PISTOL_AMMO] = "Pistol Clips",
    [GS_INV_ITEM_SHOTGUN_AMMO] = "Shotgun Shells",
    [GS_INV_ITEM_MAGNUM_AMMO] = "Magnum Clips",
    [GS_INV_ITEM_UZI_AMMO] = "Uzi Clips",
    [GS_INV_ITEM_COMPASS] = "Compass",
    [GS_INV_ITEM_GAME] = "Game",
    [GS_INV_ITEM_DETAILS] = "Detail Levels",
    [GS_INV_ITEM_SOUND] = "Sound",
    [GS_INV_ITEM_CONTROLS] = "Controls",
    [GS_INV_ITEM_GAMMA] = "Gamma",
    [GS_INV_ITEM_LARAS_HOME] = "Lara's Home",
};
