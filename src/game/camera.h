#pragma once

#include "global/types.h"

#include <stdint.h>

void Camera_Initialise(void);
void Camera_Chase(ITEM_INFO *item);
void Camera_Combat(ITEM_INFO *item);
void Camera_Look(ITEM_INFO *item);
void Camera_Fixed(void);
void Camera_Update(void);

void Camera_OffsetAdditionalAngle(int16_t delta);
void Camera_OffsetAdditionalElevation(int16_t delta);
void Camera_OffsetReset(void);

void Camera_LoadCutsceneFrame(void);
void Camera_RefreshFromTrigger(int16_t type, int16_t *data);
