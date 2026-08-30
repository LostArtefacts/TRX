#pragma once

#include <trx/core/result.h>
#include <trx/game/game_flow/types.h>
#include <trx/game/inventory_ring/types.h>
#include <trx/game/savegame/types.h>

GF_COMMAND GF_EnterPhotoMode(void);
GF_COMMAND GF_PauseGame(void);
GF_COMMAND GF_ShowInventory(INVENTORY_MODE inv_mode);
bool GF_ShowInventoryKeys(OBJECT_ID receptacle_type_id);
// Shows the title screen. Reports a title level that will not load.
RESULT GF_RunTitle(GF_COMMAND *out_cmd);
GF_COMMAND GF_RunDemo(int32_t demo_num);
GF_COMMAND GF_RunCutscene(int32_t cutscene_num, bool cross_fade_in);
GF_COMMAND GF_RunGame(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);
GF_COMMAND GF_RunGlobeSelect(const char *background_path);

// Act on what the flow asks for, starting from the given command, until it
// asks to leave the game or switch mod. Leaves no level loaded.
// Runs the game flow until it says to stop. Reports a game flow with no
// title level to fall back to.
RESULT GF_RunUntilExit(GF_COMMAND gf_cmd);

// Works out what the game does first, from the arguments the player started
// it with or the title level. Reports an argument naming a level that is not
// there.
RESULT GF_DoFrontendSequence(GF_COMMAND *out_cmd);
// Plays a demo. Reports a demo the game flow names but does not have.
RESULT GF_DoDemoSequence(int32_t demo_num, GF_COMMAND *out_cmd);

// Plays a cutscene. Reports a cutscene the game flow names but does not have.
RESULT GF_DoCutsceneSequence(
    int32_t cutscene_num, bool cross_fade_in, GF_COMMAND *out_cmd);

// Puts back what a level's sequence sets up, so that nothing one level asks
// for is carried into the next one.
void GF_ResetLevelSetup(GF_SEQUENCE_CONTEXT seq_ctx);

RESULT GF_InterpretSequence(
    const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx, void *seq_ctx_arg,
    GF_COMMAND *out_cmd);

RESULT GF_DoLevelSequence(
    const GF_LEVEL *start_level, GF_SEQUENCE_CONTEXT seq_ctx,
    GF_COMMAND *out_cmd);

RESULT GF_PlayAvailableStory(SAVEGAME_SLOT_REF slot, GF_COMMAND *out_cmd);
