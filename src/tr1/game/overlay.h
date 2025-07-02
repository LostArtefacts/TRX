#pragma once

#include "global/types.h"

#include <libtrx/config/types.h>
#include <libtrx/game/overlay.h>
#include <libtrx/game/scaler.h>

#include <stdint.h>

typedef struct {
    BAR_TYPE type;
    int32_t value;
    int32_t max_value;
    bool show;
    BAR_SHOW_MODE show_mode;
    bool blink;
    int32_t timer;
    int32_t color;
    BAR_LOCATION location;
    int16_t custom_x;
    int16_t custom_y;
    int16_t custom_width;
    int16_t custom_height;
} BAR_INFO;

void Overlay_DrawGameInfo(void);
