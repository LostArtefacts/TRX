#pragma once

#include <libtrx/game/const.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/utils.h>

#define MAX_REQLINES 18
#define LOT_SLOT_COUNT 32

#define MAX_WIBBLE 2

#define NO_BOX (-1)

#if _MSC_VER > 0x500
    #define strdup _strdup // fixes error about POSIX function
#endif
