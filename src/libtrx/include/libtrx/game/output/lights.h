#pragma once

#include "../objects/types.h"
#include "../rooms.h"
#include "../viewport.h"

void Output_InitLight(void);
void Output_ShutdownLight(void);
void Output_CalculateLight(XYZ_32 pos, int16_t room_num);
void Output_CalculateStaticLight(int16_t adder);
void Output_CalculateStaticMeshLight(XYZ_32 pos, SHADE shade, const ROOM *room);
void Output_CalculateObjectLighting(const ITEM *item, const BOUNDS_16 *bounds);
int32_t Output_GetRoomLightShade(ROOM_LIGHT_MODE mode);
void Output_LightRoomVertices(const ROOM *room);
void Output_LightRoom(ROOM *room);

int32_t Output_GetSunsetDuration(void);
int32_t Output_GetSunsetTimer(void);
void Output_SetSunsetTimer(int32_t timer);
void Output_SetSunsetEnabled(bool enabled);

void Output_ResetDynamicLights(void);
void Output_AddDynamicLight(XYZ_32 pos, int32_t intensity, int32_t falloff);

void Output_AnimateLights(int32_t num_frames);
