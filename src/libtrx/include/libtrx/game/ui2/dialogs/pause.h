#pragma once

// A pause exit confirmation dialog.

#include "../common.h"
#include "../elements/requester.h"

typedef struct {
    int32_t phase;
    UI2_REQUESTER_STATE req;
} UI2_PAUSE_STATE;

typedef enum {
    UI2_PAUSE_NOOP,
    UI2_PAUSE_RESUME_PAUSE,
    UI2_PAUSE_EXIT_TO_GAME,
    UI2_PAUSE_EXIT_TO_TITLE,
} UI2_PAUSE_EXIT_CHOICE;

// state functions
void UI2_Pause_Init(UI2_PAUSE_STATE *s);
UI2_PAUSE_EXIT_CHOICE UI2_Pause_Control(UI2_PAUSE_STATE *s);
void UI2_Pause_Free(UI2_PAUSE_STATE *s);

// draw functions
void UI2_Pause(UI2_PAUSE_STATE *s);
