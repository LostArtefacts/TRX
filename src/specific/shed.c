#include "specific/shed.h"

#include "global/vars.h"
#include "specific/display.h"

#include <stdlib.h>

void SWRInit()
{
    int32_t res_x = GetScreenWidth();
    int32_t res_y = GetScreenHeight();
    ScrPtr = malloc(res_x * res_y * sizeof(int16_t));
}
