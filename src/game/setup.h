#pragma once

#include "global/types.h"

#include <stdint.h>

bool InitialiseLevel(int32_t level_num);
void InitialiseGameFlags();
void InitialiseLevelFlags();

void BaddyObjects();
void TrapObjects();
void ObjectObjects();
void InitialiseObjects();
