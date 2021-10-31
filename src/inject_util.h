#ifndef T1M_INJECT_UTIL_H
#define T1M_INJECT_UTIL_H

#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode; // must be 0xE9
    uint32_t offset;
} JMP;
#pragma pack(pop)

#define VAR_U_(address, type) (*(type *)(address))
#define VAR_I_(address, type, value) (*(type *)(address))
#define ARRAY_(address, type, length) (*(type(*) length)(address))

void T1MInjectFunc(void (*from)(void), void (*to)(void));

#define INJECT(from, to)                                                       \
    {                                                                          \
        T1MInjectFunc((void (*)(void))from, (void (*)(void))to);               \
    }

#endif
