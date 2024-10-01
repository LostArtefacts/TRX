#pragma once

#include "global/types.h"

#include <stdint.h>

void Camera_Initialise(void);
void Camera_Reset(void);
void Camera_ResetPosition(void);
void Camera_Chase(ITEM *item);
void Camera_Combat(ITEM *item);
void Camera_Look(ITEM *item);
void Camera_Fixed(void);
void Camera_Update(void);
void Camera_UpdateCutscene(void);
void Camera_OffsetReset(void);
void Camera_RefreshFromTrigger(const TRIGGER *trigger);
void Camera_MoveManual(void);
void Camera_Apply(void);
