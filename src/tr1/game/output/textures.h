#pragma once

#include <libtrx/gfx/gl/utils.h>

void Output_Textures_Init(void);
void Output_Textures_Shutdown(void);
void Output_Textures_ObserveLevelLoad(void);
void Output_Textures_UpdateEnvironmentMap(void);
void Output_Textures_CycleAnimations(void);
void Output_Textures_ApplyRenderSettings(void);
GLuint Output_Textures_GetUVWsTexture(void);
GLuint Output_Textures_GetAtlasTexture(void);
GLuint Output_Textures_GetAtlasSizesTexture(void);
GLuint Output_Textures_GetEnvMapTexture(void);
int32_t Output_Textures_GetSpritesUVWsBase(void);
