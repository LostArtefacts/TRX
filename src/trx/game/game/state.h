#pragma once

#include <trx/game/game/enum.h>
#include <trx/game/game_flow.h>

extern int32_t g_OverlayFlag;

// Sets the game's playing state, which in turn toggles random draw lock, and
// certain overlay displays/animations, such as bars and pickups.
void Game_SetIsPlaying(bool is_playing);

// Returns true if the game is in a playing state - i.e. not suspended, such as
// during pause, photo mode, or while the inventory is open.
bool Game_IsPlaying(void);

const GF_LEVEL *Game_GetCurrentLevel(void);
void Game_SetCurrentLevel(const GF_LEVEL *level);

bool Game_IsInGym(void);

// Returns true if an FMV is not playing and the current level is not the title.
bool Game_IsLoaded(void);

// Returns true if an FMV is not playing, if the level type is neither the
// title, a demo or a cutscene, and if Lara is loaded and controllable.
bool Game_IsPlayable(void);

// Whether the engine is populating the item pool for a level - the initial
// cast at load, plus a save overlaid on top. The item lifecycle events read
// this so the bulk setup stays quiet; it is set as the level loads and cleared
// once live play begins. Distinct from a suspended game (pause, photo mode,
// inventory), where a script may still drive an item and its events must fire.
void Game_SetIsSettingUpItems(bool value);
bool Game_IsSettingUpItems(void);

GAME_BONUS_FLAG Game_GetBonusFlag(void);
void Game_SetBonusFlag(GAME_BONUS_FLAG flag);
bool Game_IsBonusFlagSet(GAME_BONUS_FLAG flag);

void Game_SetIsLevelComplete(bool is_complete);
bool Game_IsLevelComplete(void);
