#pragma once

#include <libtrx/gfx/gl/utils.h>

#pragma pack(push, 1)
typedef struct {
    float x0;
    float y0;
    float x1;
    float y1;
} OUTPUT_TEXTURE_SIZE;

typedef struct {
    float u;
    float v;
    float w;
} OUTPUT_UVW;
#pragma pack(pop)

void Output_Textures_Init(void);
void Output_Textures_Shutdown(void);
void Output_Textures_ObserveLevelLoad(void);
void Output_Textures_UpdateEnvironmentMap(void);
void Output_Textures_CycleAnimations(void);
void Output_Textures_ApplyRenderSettings(void);
GLuint Output_Textures_GetAtlasTexture(void);
GLuint Output_Textures_GetEnvMapTexture(void);
int32_t Output_Textures_GetSpritesUVWsBase(void);

OUTPUT_UVW Output_Textures_GetUVW(int32_t uvw_idx);
OUTPUT_TEXTURE_SIZE Output_Textures_GetAtlasSize(int32_t uvw_idx);
bool Output_Textures_IsObjectTextureAnimated(int32_t texture_idx);
bool Output_Textures_IsSpriteTextureAnimated(int32_t sprite_idx);
