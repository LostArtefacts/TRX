#pragma once

#include <stdbool.h>
#include <stdint.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode; // must be 0xE9
    uint32_t offset;
} JMP;
#pragma pack(pop)

void InjectImpl(bool enable, void (*from)(void), void (*to)(void));

#define INJECT(enable, from, to)                                               \
    {                                                                          \
        InjectImpl(enable, (void (*)(void))from, (void (*)(void))to);          \
    }
