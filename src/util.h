#pragma once

#define SQUARE(A) ((A) * (A))
#ifndef ABS
    #define ABS(x) (((x) < 0) ? (-(x)) : (x))
    #define MIN(x, y) ((x) <= (y) ? (x) : (y))
    #define MAX(x, y) ((x) >= (y) ? (x) : (y))
#endif

#define CLAMPL(a, b)                                                           \
    do {                                                                       \
        if ((a) < (b))                                                         \
            (a) = (b);                                                         \
    } while (0)
#define CLAMPG(a, b)                                                           \
    do {                                                                       \
        if ((a) > (b))                                                         \
            (a) = (b);                                                         \
    } while (0)
#define CLAMP(a, b, c)                                                         \
    do {                                                                       \
        if ((a) < (b))                                                         \
            (a) = (b);                                                         \
        else if ((a) > (c))                                                    \
            (a) = (c);                                                         \
    } while (0)
#define SWAP(a, b, c)                                                          \
    do {                                                                       \
        (c) = (a);                                                             \
        (a) = (b);                                                             \
        (b) = (c);                                                             \
    } while (0)
