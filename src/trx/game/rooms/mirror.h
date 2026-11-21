#pragma once

#include <trx/core/math/types.h>

#include <stdint.h>

// Operations on the mirror room: a room whose wall at a fixed z position
// reflects Lara and the effects around her. The level holds the reflected
// copy of the room geometry, so only the moving parts are drawn twice.

void Room_ResetMirror(void);
void Room_SetMirror(int16_t room_num, int32_t z_plane);

// Returns whether the room is the mirror room of the level.
bool Room_IsMirror(int16_t room_num);
int32_t Room_GetMirrorPlane(void);

// Returns the position reflected across the mirror plane.
XYZ_32 Room_ReflectInMirror(XYZ_32 pos);
