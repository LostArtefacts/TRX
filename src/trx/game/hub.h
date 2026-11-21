#pragma once

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math/types.h>
#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/objects/ids.h>

typedef struct {
    XYZ_32 pos;
    int16_t rot_y;
    int16_t room_num;
} HUB_START_POINT;

void Hub_SetNextLevelIndex(int32_t level_idx);
const GF_LEVEL *Hub_GetNextLevel(void);
// Returns the next level index without consuming it, or 0 for none.
int32_t Hub_GetNextLevelIndex(void);
void Hub_SetLaraStartIndex(int32_t start_idx);
int32_t Hub_GetLaraStartIndex(void);
// Puts Lara at the start point that the finish trigger named, and remembers
// it as the point where she entered the level.
void Hub_InitialiseLaraStart(const GF_LEVEL *level);

// Returns the start point where Lara entered the level, or 0 for its own
// start. A save keeps it, so that a restart puts her there again.
int32_t Hub_GetEntryStartIndex(const GF_LEVEL *level);
void Hub_SetEntryStartIndex(const GF_LEVEL *level, int32_t start_idx);
// Makes the next start of the level put Lara where she entered it.
void Hub_RepeatEntry(const GF_LEVEL *level);

// Sets the places that a finish trigger can start Lara at in the level being
// loaded. A trigger's timer counts them from 1, in the order that the level
// file lists its AI objects in TR4, or its Lara start positions in TR1-3. The
// points must last as long as the level.
void Hub_SetStartPoints(const HUB_START_POINT *points, int32_t count);

// Returns the vehicle that Lara left the last level on, or NO_OBJECT. The
// next level moves its first vehicle of that kind to her.
OBJECT_ID Hub_GetVehicle(void);
void Hub_SetVehicle(OBJECT_ID object_id);

// Remembers the state of the level that Lara is leaving, so that a later
// visit finds the level as she left it. Only a game flow with a reset_hub
// event remembers levels, and only the levels that Lara plays.
void Hub_StoreLevel(const GF_LEVEL *level);
// Reports whether the game remembers the state that Lara left the level in.
bool Hub_HasLevel(const GF_LEVEL *level);
// Puts the level back in the state that Lara left it in. Does nothing for a
// level that the game does not remember.
RESULT Hub_RestoreLevel(const GF_LEVEL *level);
// Forgets the state of every level.
void Hub_Reset(void);

void Hub_Dump(JSON_WRITE_IO *io);
// Replaces the remembered levels with those of a savegame. A savegame without
// them leaves every level fresh.
RESULT Hub_Load(JSON_READ_IO *io);
