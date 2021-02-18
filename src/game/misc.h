#ifndef TOMB1MAIN_GAME_MISC_H
#define TOMB1MAIN_GAME_MISC_H

#define SQUARE(A) ((A) * (A))
#ifndef ABS
    #define ABS(x) (((x) < 0) ? (-(x)) : (x))
    #define MIN(x, y) ((x) <= (y) ? (x) : (y))
    #define MAX(x, y) ((x) >= (y) ? (x) : (y))
#endif

#endif
