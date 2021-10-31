#include "specific/shed.h"

#include "global/vars.h"

#include <stdlib.h>

void SWRInit()
{
    int32_t res_x = AvailableResolutions[HiRes].width;
    int32_t res_y = AvailableResolutions[HiRes].height;
    ScrPtr = malloc(res_x * res_y * sizeof(int16_t));
}
