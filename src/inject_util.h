#ifndef T1M_INJECT_UTIL_H
#define T1M_INJECT_UTIL_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode; // must be 0xE9
    uint32_t offset;
} JMP;
#pragma pack(pop)

void T1MInjectFunc(void (*from)(void), void (*to)(void));

#define INJECT(from, to)                                                       \
    {                                                                          \
        T1MInjectFunc((void (*)(void))from, (void (*)(void))to);               \
    }

#endif
