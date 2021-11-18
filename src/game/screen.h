#ifndef T1M_GAME_SCREEN_H
#define T1M_GAME_SCREEN_H

#include <stdbool.h>
#include <stdint.h>

void Screen_SetupSize();

bool Screen_SetGameResIdx(int32_t idx);
bool Screen_SetPrevGameRes();
bool Screen_SetNextGameRes();
int32_t Screen_GetGameResIdx();
int32_t Screen_GetGameResWidth();
int32_t Screen_GetGameResHeight();

int32_t Screen_GetResIdx();
int32_t Screen_GetResWidth();
int32_t Screen_GetResHeight();

void Screen_SetResolution(int32_t hi_res);
void Screen_RestoreResolution();

int32_t GetRenderScaleGLRage(int32_t unit);
int32_t GetRenderScale(int32_t unit);
int32_t GetRenderHeightDownscaled();
int32_t GetRenderWidthDownscaled();
int32_t GetRenderHeight();
int32_t GetRenderWidth();

#endif
