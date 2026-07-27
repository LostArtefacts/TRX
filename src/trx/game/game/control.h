#pragma once

#include <trx/game/game_flow/types.h>

bool Game_Start(const GF_LEVEL *level, GF_SEQUENCE_CONTEXT seq_ctx);
void Game_End(void);

GF_COMMAND Game_Control(bool demo_mode);

// The steps of a simulation tick that gameplay, cutscenes and the live title
// scene share, in the order they run. Each caller drives its own player and
// HUD around them. Cutscenes update their camera and Lara's gun between the
// frame start and the world step, because the gun flash adds a dynamic light,
// and so run no post-control step, keeping their own FX_Control call.
void Game_TickBeginFrame(void);
void Game_TickWorld(void);
void Game_TickPostControl(void);
void Game_TickEndFrame(void);

void Game_ProcessInput(void);
