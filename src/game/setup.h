#ifndef T1M_GAME_SETUP_H
#define T1M_GAME_SETUP_H

#include "game/types.h"

#include <stdint.h>

int32_t InitialiseLevel(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
void InitialiseGameFlags();
void InitialiseLevelFlags();

void BaddyObjects();
void TrapObjects();
void ObjectObjects();
void InitialiseObjects();

void T1MInjectGameSetup();

#endif
