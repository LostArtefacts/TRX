#pragma once

#include "global/types.h"

#include <stdint.h>

int32_t InitialiseLevel(int32_t level_num, GAMEFLOW_LEVEL_TYPE level_type);
void InitialiseGameFlags();
void InitialiseLevelFlags();

void BaddyObjects();
void TrapObjects();
void ObjectObjects();
void InitialiseObjects();
