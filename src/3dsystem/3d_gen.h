#pragma once

#include "global/types.h"

#include <stdint.h>

int32_t phd_VisibleZClip(PHD_VBUF *vn1, PHD_VBUF *vn2, PHD_VBUF *vn3);
void phd_RotateLight(int16_t pitch, int16_t yaw);
void phd_AlterFOV(PHD_ANGLE fov);
