#pragma once

#include <trx/game/phase/executor.h>

void FlybyMode_Activate(int32_t sequence_idx, bool one_shot);
void FlybyMode_Deactivate(void);
void FlybyMode_PreControl(void);
void FlybyMode_PostControl(void);
