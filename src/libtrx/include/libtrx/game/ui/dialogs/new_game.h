#pragma once

// A new game mode selector dialog.

#include "../common.h"
#include "../elements/requester.h"

typedef struct {
    UI_REQUESTER_STATE req;
} UI_NEW_GAME_STATE;

// state functions
void UI_NewGame_Init(UI_NEW_GAME_STATE *s);
int32_t UI_NewGame_Control(UI_NEW_GAME_STATE *s);
void UI_NewGame_Free(UI_NEW_GAME_STATE *s);

// draw functions
void UI_NewGame(UI_NEW_GAME_STATE *s);
