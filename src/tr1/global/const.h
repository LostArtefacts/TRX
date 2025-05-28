#pragma once

#include <libtrx/game/const.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/utils.h>

#define PHD_ONE 0x10000

#define MAX_REQLINES 18
#define LOT_SLOT_COUNT 32
#define MAX_SECRETS 16

#define DEFAULT_RADIUS 10

#define MAX_WIBBLE 2
#define NO_VERT_MOVE 0x2000
#define NO_BOX (-1)
#define PASSPORT_FOV 65
#define PICKUPS_FOV 65

#define DEFAULT_RADIUS 10

#define RINGSWITCH_FRAMES (96 / 2)
#define SELECTING_FRAMES (32 / 2)
#define OPTION_RING_OBJECTS 4
#define TITLE_RING_OBJECTS 5

#if _MSC_VER > 0x500
    #define strdup _strdup // fixes error about POSIX function
#endif
