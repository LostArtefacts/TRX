// The scene the trx.ui bindings may build in.
#pragma once

#include <trx/game/ui/common.h>

// Sets whether trx.ui may add widgets to the current scene.
void LUA_UI_SetReserving(bool reserving);
bool LUA_UI_IsReserving(void);

// Opens each region and lets scripts add widgets to it.
void LUA_UI_ReserveRegions(void);

// Hides the script interface from the next drawn frame so a screenshot contains
// only the picture. The console still draws.
void LUA_UI_HideNextFrame(void);

// Lets scripts draw into the boxes reserved during scene layout. Runs once
// per layer, so that a script can paint under or over the engine UI.
void LUA_UI_PaintRegions(UI_LAYER layer);

// Sets whether trx.ui may schedule draw calls for the current scene.
void LUA_UI_SetPainting(bool painting);
bool LUA_UI_IsPainting(void);
