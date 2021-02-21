#ifndef TOMB1MAIN_UTIL_H
#define TOMB1MAIN_UTIL_H

#define TOMB1M_FEAT_EXTENDED_MEMORY
#define TOMB1M_FEAT_UI
#define TOMB1M_FEAT_GAMEPLAY
#define TOMB1M_FEAT_LEVEL_FIXES
#define TOMB1M_FEAT_NOCD

#include <stdint.h>
#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode; // must be 0xE9
    uint32_t offset;
} JMP;
#pragma pack(pop)

#define TRACE(...)                                                             \
    {                                                                          \
        printf("%s:%d %s ", __FILE__, __LINE__, __func__);                     \
        printf(__VA_ARGS__);                                                   \
        printf("\n");                                                          \
        fflush(stdout);                                                        \
    }

#define VAR_U_(address, type) (*(type*)(address))
#define VAR_I_(address, type, value) (*(type*)(address))
#define ARRAY_(address, type, length) (*(type(*) length)(address))

void Tomb1MInjectFunc(void* from, void* to);
void Tomb1MPrintStackTrace();

#define INJECT(from, to)                                                       \
    {                                                                          \
        Tomb1MInjectFunc((void*)from, (void*)to);                              \
    }

#endif
