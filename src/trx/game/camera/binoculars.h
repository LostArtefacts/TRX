#pragma once

#include <stdbool.h>
#include <stdint.h>

void Camera_Binoculars_Reset(void);
void Camera_Binoculars_Request(void);
void Camera_Binoculars_Control(void);
void Camera_Binoculars_Update(void);
void Camera_Binoculars_Exit(void);
bool Camera_Binoculars_IsActive(void);
int32_t Camera_Binoculars_GetRange(void);
