#pragma once

#include "../inventory_ring/types.h"

typedef enum {
    PASSPORT_ACTION_LOAD_GAME,
    PASSPORT_ACTION_SELECT_LEVEL,
    PASSPORT_ACTION_STORY_SO_FAR,
    PASSPORT_ACTION_SAVE_GAME,
    PASSPORT_ACTION_NEW_GAME,
    PASSPORT_ACTION_RESTART,
    PASSPORT_ACTION_EXIT_TO_TITLE,
    PASSPORT_ACTION_EXIT_GAME,
} PASSPORT_ACTION;

typedef struct {
    PASSPORT_ACTION select_action;
    int32_t select_slot;
    bool ask_for_save;
} PASSPORT;

extern PASSPORT g_Passport; // TODO: meh

extern void Option_Passport_Control(INVENTORY_ITEM *inv_item, bool is_busy);
extern void Option_Passport_Draw(INVENTORY_ITEM *inv_item);
extern void Option_Passport_Close(void);
