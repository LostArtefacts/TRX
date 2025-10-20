#pragma once

#include "../inventory_ring/types.h"

typedef enum {
    PASSPORT_ROLE_LOAD_GAME,
    PASSPORT_ROLE_SELECT_LEVEL,
    PASSPORT_ROLE_STORY_SO_FAR,
    PASSPORT_ROLE_SAVE_GAME,
    PASSPORT_ROLE_NEW_GAME,
    PASSPORT_ROLE_RESTART,
    PASSPORT_ROLE_EXIT_TITLE,
    PASSPORT_ROLE_EXIT_GAME,
} PASSPORT_ROLE;

typedef struct {
    PASSPORT_ROLE select_role;
    int32_t select_slot;
    bool ask_for_save;
} PASSPORT;

extern PASSPORT g_Passport; // TODO: meh

extern void Option_Passport_Control(INVENTORY_ITEM *inv_item, bool is_busy);
extern void Option_Passport_Draw(INVENTORY_ITEM *inv_item);
extern void Option_Passport_Close(void);
