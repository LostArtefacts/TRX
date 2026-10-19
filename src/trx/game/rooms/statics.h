#pragma once

#include <trx/core/result.h>
#include <trx/game/rooms/types.h>

// A static that reaches through a portal is copied into each room it reaches.
// Every copy keeps the draw number of the static, so all copies shatter
// together.
void Room_ResetShatteredStatics(void);
bool Room_IsStaticShattered(const STATIC_MESH *mesh);

// Shatters a static. The static stops drawing and colliding, the heavy
// triggers under it fire, and its sector stops blocking creatures. The debris
// and the sound belong to the caller.
void Room_ShatterStatic(const STATIC_MESH *mesh);

void Room_SaveShatteredStatics(JSON_WRITE_IO *io);
// Shatters the statics that a savegame records, without firing the triggers
// under them.
RESULT Room_LoadShatteredStatics(JSON_READ_IO *io);
