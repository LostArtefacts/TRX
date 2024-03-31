#pragma once

#include "global/types.h"

#include <stdint.h>

void Camera_Initialise(void);
void Camera_Reset(void);
void Camera_Chase(ITEM_INFO *item);
void Camera_Combat(ITEM_INFO *item);
void Camera_Look(ITEM_INFO *item);
void Camera_Fixed(void);
void Camera_Update(void);
void Camera_UpdateCutscene(void);
void Camera_OffsetReset(void);
void Camera_RefreshFromTrigger(int16_t type, int16_t *data);
void Camera_MoveManual(void);
void Camera_Apply(void);
