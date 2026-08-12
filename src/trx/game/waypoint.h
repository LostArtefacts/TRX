// How far along a level's scripted progression Lara has got.
//
// TR4 marks the points of a level with flip effects, and its guides read them:
// Von Croy waits at a waypoint until Lara has reached it, speaks the line that
// belongs to it, and only then moves on. Nothing about it is positional, so it
// belongs to the playthrough rather than to Lara.
#pragma once

#include <stdint.h>

// Before Lara has reached anywhere.
#define WAYPOINT_NONE (-1)
// No pad was crossed this frame.
#define WAYPOINT_PAD_NONE (-128)

// Where Lara has reached. Kept in savegames.
int32_t Waypoint_Get(void);

// Sets it, raising the furthest reached to match where that is further on.
void Waypoint_Set(int32_t num);

// The furthest she has ever reached, which a level uses to tell a first visit
// from a return. Kept in savegames.
int32_t Waypoint_GetHighest(void);

// Only a savegame sets this on its own: everywhere else it follows where Lara
// has reached, and a load has to put back a mark she has since walked back
// from.
void Waypoint_SetHighest(int32_t num);

// The pad she crossed, which lasts the one frame: the flip effect names it,
// and Lara's own control clears it again on the next tick. It says where she
// is now rather than how far she has got, so it is not saved.
int32_t Waypoint_GetPad(void);

// Names the pad, and where she has reached along with it. The furthest reached
// is left alone, as the original engine leaves it.
void Waypoint_SetPad(int32_t num);

void Waypoint_ClearPad(void);

// Drops everything back to the start of a playthrough.
void Waypoint_Reset(void);
