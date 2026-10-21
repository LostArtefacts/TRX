#pragma once

#include <stdint.h>

void Camera_Binoculars_Reset(void);
void Camera_Binoculars_Request(void);
void Camera_Binoculars_Control(void);
void Camera_Binoculars_Update(void);
void Camera_Binoculars_Exit(void);
bool Camera_Binoculars_IsActive(void);
// Whether the view is the laser sight's: the view of a weapon that carries
// the laser sight, which Lara aims and fires through while LOOK is held.
bool Camera_Binoculars_IsLaserSight(void);
int32_t Camera_Binoculars_GetRange(void);
