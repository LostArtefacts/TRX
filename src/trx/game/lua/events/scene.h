// The 3D scene the trx.scene bindings may draw into.
#pragma once

// Lets scripts draw shapes into the scene the camera is looking at. Runs once
// per drawn frame, after the rooms and the items in them.
void LUA_Scene_Paint(void);

// Sets whether trx.scene may schedule draw calls for the current scene.
void LUA_Scene_SetPainting(bool painting);
bool LUA_Scene_IsPainting(void);
