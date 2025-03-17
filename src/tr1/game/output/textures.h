#pragma once

#include <libtrx/gfx/gl/utils.h>

void Output_Textures_Init(void);
void Output_Textures_Shutdown(void);
void Output_Textures_UploadLevel(void);
void Output_Textures_Update(void);
GLuint Output_Textures_GetSpriteUVWsTexture(void);
GLuint Output_Textures_GetAtlasTexture(void);
