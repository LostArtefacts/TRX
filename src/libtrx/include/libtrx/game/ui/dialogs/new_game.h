#pragma once

// A new game mode selector dialog.

#include "../common.h"
#include "../elements/requester.h"

typedef struct UI_NEW_GAME_STATE UI_NEW_GAME_STATE;

// state functions
UI_NEW_GAME_STATE *UI_NewGame_Init(void);
int32_t UI_NewGame_Control(UI_NEW_GAME_STATE *s);
void UI_NewGame_Free(UI_NEW_GAME_STATE *s);

// draw functions
void UI_NewGame(UI_NEW_GAME_STATE *s);
