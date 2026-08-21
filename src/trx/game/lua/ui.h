// The scene the trx.ui bindings may build in.
#pragma once

// Says whether a UI scene is open. The trx.ui bindings may add to a scene only
// while one is, and report an error at any other time.
void LUA_UI_SetDrawing(bool drawing);
bool LUA_UI_IsDrawing(void);
