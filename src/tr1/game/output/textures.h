#pragma once

#include <libtrx/gfx/gl/utils.h>

void Output_Textures_Init(void);
void Output_Textures_Shutdown(void);
void Output_Textures_ObserveLevelLoad(void);
void Output_Textures_UpdateEnvironmentMap(void);
void Output_Textures_CycleAnimations(void);
void Output_Textures_ApplyRenderSettings(void);
GLuint Output_Textures_GetObjectUVWsTexture(void);
GLuint Output_Textures_GetSpriteUVWsTexture(void);
GLuint Output_Textures_GetAtlasTexture(void);
GLuint Output_Textures_GetEnvMapTexture(void);
