#pragma once

#include <libtrx/game/game_flow/sequencer.h>

void GF_InitSequencer(void);

GF_COMMAND GF_DoLevelSequence(
    const GF_LEVEL *start_level, GF_SEQUENCE_CONTEXT seq_ctx);

GF_COMMAND GF_PlayAvailableStory(int32_t slot_num);

// Returns true if any story cutscenes or FMVs occur before gameplay in any
// main level up to the level in the specified save slot.
bool GF_HasAvailableStory(int32_t slot_num);
