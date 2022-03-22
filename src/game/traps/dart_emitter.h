#pragma once

#include "global/types.h"

#include <stdint.h>

typedef enum {
    DART_EMITTER_IDLE = 0,
    DART_EMITTER_FIRE = 1,
} DART_EMITTER_STATE;

void DartEmitter_Setup(OBJECT_INFO *obj);
void DartEmitter_Control(int16_t item_num);
