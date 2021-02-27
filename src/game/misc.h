#ifndef T1M_GAME_MISC_H
#define T1M_GAME_MISC_H

#define SQUARE(A) ((A) * (A))
#ifndef ABS
    #define ABS(x) (((x) < 0) ? (-(x)) : (x))
    #define MIN(x, y) ((x) <= (y) ? (x) : (y))
    #define MAX(x, y) ((x) >= (y) ? (x) : (y))
#endif
#define CHK_ALL(a, b) (((a) & (b)) == (b))
#define CHK_ANY(a, b) (((a) & (b)) != 0)

#endif
