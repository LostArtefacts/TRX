#pragma once

#include <stdint.h>

// TODO: consolidate this API
#if TR_VERSION == 1
extern int16_t Viewport_GetFOV(void);
extern void Viewport_SetFOV(int16_t view_angle);
#elif TR_VERSION == 2
extern int16_t Viewport_GetFOV(bool resolve_user_fov);
extern void Viewport_AlterFOV(int16_t view_angle);
#endif

extern int32_t Viewport_GetWidth(void);
extern int32_t Viewport_GetHeight(void);
extern int32_t Viewport_GetMaxX(void);
extern int32_t Viewport_GetMaxY(void);
