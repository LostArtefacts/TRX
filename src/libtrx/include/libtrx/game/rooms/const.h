#pragma once

#define MAX_ROOMS 1024
#define MAX_ROOMS_TO_DRAW 100
#define MAX_FLIP_MAPS 10

#define NO_ROOM_NEG (-1)
#define NO_ROOM 255 // TODO: merge this with NO_ROOM_NEG

#define NO_HEIGHT (-32512)
#define MAX_HEIGHT 32000

#define NEG_TILT(T, H) ((T * (H & (WALL_L - 1))) >> 2)
#define POS_TILT(T, H) ((T * ((WALL_L - 1 - H) & (WALL_L - 1))) >> 2)
