#pragma once

#include "../common.h"

#include <stdint.h>

// Make the child widget invisible in the specified interval.

typedef struct {
    int32_t rate;
    int32_t count;
} UI2_FLASH_STATE;

// state functions
void UI2_Flash_Init(UI2_FLASH_STATE *s, int32_t rate);
void UI2_Flash_Free(UI2_FLASH_STATE *s);
void UI2_Flash_Control(UI2_FLASH_STATE *s);

// draw functions
void UI2_BeginFlash(UI2_FLASH_STATE *s);
void UI2_EndFlash(void);
