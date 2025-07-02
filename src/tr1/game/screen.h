#pragma once

#include <stdint.h>

void Screen_Init(void);

int32_t Screen_GetResWidth(void);
int32_t Screen_GetResHeight(void);

bool Screen_CanSetPrevRes(void);
bool Screen_CanSetNextRes(void);
bool Screen_SetPrevRes(void);
bool Screen_SetNextRes(void);
