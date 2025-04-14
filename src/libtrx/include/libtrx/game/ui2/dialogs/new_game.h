#pragma once

// A new game mode selector dialog.

#include "../common.h"
#include "../elements/requester.h"

typedef struct {
    UI2_REQUESTER_STATE req;
} UI2_NEW_GAME_STATE;

// state functions
void UI2_NewGame_Init(UI2_NEW_GAME_STATE *s);
int32_t UI2_NewGame_Control(UI2_NEW_GAME_STATE *s);
void UI2_NewGame_Free(UI2_NEW_GAME_STATE *s);

// draw functions
void UI2_NewGame(UI2_NEW_GAME_STATE *s);
