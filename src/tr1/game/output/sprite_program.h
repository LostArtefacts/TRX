#pragma once

#include <libtrx/game/matrix.h>
#include <libtrx/game/output/types.h>

void Output_SpriteProgram_Init(void);
void Output_SpriteProgram_Shutdown(void);
void Output_SpriteProgram_Bind(void);
void Output_SpriteProgram_UploadCommonUniforms(void);
void Output_SpriteProgram_UploadProjectionMatrix(void);

// TODO: these functions are poor design.
void Output_SpriteProgram_UploadMatrix(const MATRIX *source);
void Output_SpriteProgram_UploadTint(RGB_F tint);
