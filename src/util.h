#ifndef T1M_UTIL_H
#define T1M_UTIL_H

#include <stdint.h>
#include <stdio.h>

#define SQUARE(A) ((A) * (A))
#ifndef ABS
    #define ABS(x) (((x) < 0) ? (-(x)) : (x))
    #define MIN(x, y) ((x) <= (y) ? (x) : (y))
    #define MAX(x, y) ((x) >= (y) ? (x) : (y))
#endif
#define CHK_ALL(a, b) (((a) & (b)) == (b))
#define CHK_ANY(a, b) (((a) & (b)) != 0)

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

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode; // must be 0xE9
    uint32_t offset;
} JMP;
#pragma pack(pop)

#define TRACE(...) T1MTraceFunc(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define VAR_U_(address, type) (*(type *)(address))
#define VAR_I_(address, type, value) (*(type *)(address))
#define ARRAY_(address, type, length) (*(type(*) length)(address))

void T1MTraceFunc(
    const char *file, int line, const char *func, const char *fmt, ...);
void T1MInjectFunc(void *from, void *to);
void T1MPrintStackTrace();

#define INJECT(from, to)                                                       \
    {                                                                          \
        T1MInjectFunc((void *)from, (void *)to);                               \
    }

#endif
