#include "specific/shed.h"

#include "global/vars.h"

#include <stdlib.h>

void SWRInit()
{
    ScrPtr = malloc(GameVidWidth * GameVidHeight * sizeof(int16_t));
}
