// The scene the trx.ui bindings may build in.
#pragma once

// Sets whether trx.ui may add widgets to the current scene.
void LUA_UI_SetDrawing(bool drawing);
bool LUA_UI_IsDrawing(void);

// Opens each region and lets scripts add widgets to it.
void LUA_UI_DrawRegions(void);

// Lets scripts draw into the boxes reserved during scene layout.
void LUA_UI_PaintRegions(void);

// Sets whether trx.ui may schedule draw calls for the current scene.
void LUA_UI_SetPainting(bool painting);
bool LUA_UI_IsPainting(void);
