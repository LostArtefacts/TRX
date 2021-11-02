#ifndef T1M_SPECIFIC_INPUT_H
#define T1M_SPECIFIC_INPUT_H

#include "global/types.h"

#include <stdint.h>

typedef enum INPUT_LAYOUT {
    INPUT_LAYOUT_DEFAULT,
    INPUT_LAYOUT_USER,
    INPUT_LAYOUT_NUMBER_OF,
} INPUT_LAYOUT;

extern int16_t Layout[INPUT_LAYOUT_NUMBER_OF][KEY_NUMBER_OF];
extern int32_t ConflictLayout[KEY_NUMBER_OF];
extern INPUT_STATE OldInputDB;

void InputInit();
int16_t KeyGet();
void S_UpdateInput();
INPUT_STATE GetDebouncedInput(INPUT_STATE input);

#endif
