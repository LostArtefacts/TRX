#pragma once

#include <libtrx/gfx/gl/utils.h>

void Output_Textures_Init(void);
void Output_Textures_Shutdown(void);
void Output_Textured_UploadLevel(void);
GLuint Output_Textures_GetSpriteFramesTex(void);
