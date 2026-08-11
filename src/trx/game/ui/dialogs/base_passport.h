// Base passport dialog functions.
// Does not implement a function on its own, and is used mostly for placement
// and sizing of the larger dialogs such as load/save game.

#pragma once

#include <trx/game/ui/common.h>
#include <trx/game/ui/elements/requester.h>

// state functions

// Sets the requester up for the passport, sizing its list to the room the
// screen leaves. footer_height is what the dialog draws under the list, if
// anything, so that the rows make way for it.
void UI_BasePassportDialog_Init(
    UI_REQUESTER_STATE *req, size_t max_rows, float footer_height);
void UI_BasePassportDialog_Control(UI_REQUESTER_STATE *req);

// draw functions
void UI_BeginBasePassportDialog(const UI_REQUESTER_STATE *req);
void UI_EndBasePassportDialog(void);
