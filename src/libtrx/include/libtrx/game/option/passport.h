#pragma once

#include "../inventory_ring/types.h"

#if TR_VERSION == 1
typedef enum {
    PASSPORT_MODE_BROWSE = 0,
    PASSPORT_MODE_LOAD_GAME = 1,
    PASSPORT_MODE_SELECT_LEVEL = 2,
    PASSPORT_MODE_STORY_SO_FAR = 3,
    PASSPORT_MODE_SAVE_GAME = 4,
    PASSPORT_MODE_NEW_GAME = 5,
    PASSPORT_MODE_RESTART = 6,
    PASSPORT_MODE_EXIT_TITLE = 7,
    PASSPORT_MODE_EXIT_GAME = 8,
    PASSPORT_MODE_UNAVAILABLE = 9,
} PASSPORT_MODE;
#endif

typedef struct {
#if TR_VERSION == 1
    PASSPORT_MODE passport_selection;
#else
    int32_t passport_page;
#endif
    int32_t select_save_slot;
    int32_t select_level_num;
    bool ask_for_save;
} PASSPORT;

extern PASSPORT g_Passport; // TODO: meh

extern void Option_Passport_Control(INVENTORY_ITEM *inv_item, bool is_busy);
extern void Option_Passport_Draw(INVENTORY_ITEM *inv_item);
extern void Option_Passport_Close(void);
