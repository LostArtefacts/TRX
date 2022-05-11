#pragma once

#include "global/types.h"

#include <stdint.h>

bool InitialiseLevel(int32_t level_num);
void InitialiseGameFlags(void);

void BaddyObjects(void);
void TrapObjects(void);
void ObjectObjects(void);
void InitialiseObjects(void);
