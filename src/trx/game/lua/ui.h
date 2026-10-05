// The scene the trx.ui bindings may build in.
#pragma once

#include <trx/game/ui/common.h>

// Sets whether trx.ui may add widgets to the current scene.
void LUA_UI_SetDrawing(bool drawing);
bool LUA_UI_IsDrawing(void);

// Opens each region and lets scripts add widgets to it.
void LUA_UI_DrawRegions(void);

// Hides the script interface from the next drawn frame so a screenshot contains
// only the picture.
void LUA_UI_HideNextFrame(void);

// Lets scripts draw into the boxes reserved during scene layout. Runs once
// per layer, so that a script can paint under or over the engine UI.
void LUA_UI_PaintRegions(UI_PAINT_LAYER layer);

// The depth that the paint pass under the engine interface draws at. It keeps
// script drawing behind every engine widget, and leaves room for
// push_depth in trxc.ui to bring a script's layer nearer.
#define LUA_UI_UNDER_DEPTH 4096

// Sets whether trx.ui may schedule draw calls for the current scene.
void LUA_UI_SetPainting(bool painting);
bool LUA_UI_IsPainting(void);

// Sets the depth added to every z that a script draws with, for one paint pass.
// A lower depth draws nearer.
void LUA_UI_SetPaintDepth(int32_t depth);
