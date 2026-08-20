#pragma once

#include <trx/game/phase/executor.h>

// Answers whether the sequence took the camera.
bool FlybyMode_Activate(int32_t sequence_idx, bool one_shot);
void FlybyMode_Deactivate(void);

// Stops the sequence without returning the camera to Lara, for callers that
// hand the camera straight to something else. Everything else a sequence held
// - her controls among them - is released as a full deactivation would.
void FlybyMode_Stop(void);
bool FlybyMode_IsActive(void);

// Stops the sequence and runs the heavy triggers its remaining cameras carry,
// then returns the camera to Lara. A track path sequence never stops this way,
// and a sequence the level marks unbreakable stops only where force is
// set. Reports whether the sequence stopped.
bool FlybyMode_Cancel(bool force);

void FlybyMode_PreControl(void);
void FlybyMode_PostControl(void);
