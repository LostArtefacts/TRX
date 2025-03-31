#pragma once

#include <libtrx/gfx/gl/utils.h>

void Output_Textures_Init(void);
void Output_Textures_Shutdown(void);
void Output_Textures_ObserveLevelLoad(void);
void Output_Textures_Update(void);
void Output_Textures_ApplyRenderSettings(void);
GLuint Output_Textures_GetObjectUVWsTexture(void);
GLuint Output_Textures_GetSpriteUVWsTexture(void);
GLuint Output_Textures_GetAtlasTexture(void);
