#ifndef TOMB1MAIN_UTIL_H
#define TOMB1MAIN_UTIL_H

#include <stdint.h>
#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    uint8_t opcode; // must be 0xE9
    uint32_t offset;
} JMP;
#pragma pack(pop)

#define TRACE(...) Tomb1MTraceFunc(__FILE__, __LINE__, __func__, __VA_ARGS__)
#define VAR_U_(address, type) (*(type*)(address))
#define VAR_I_(address, type, value) (*(type*)(address))
#define ARRAY_(address, type, length) (*(type(*) length)(address))

void Tomb1MTraceFunc(
    const char* file, int line, const char* func, const char* fmt, ...);
void T1MInjectFunc(void* from, void* to);
void Tomb1MPrintStackTrace();

#define INJECT(from, to)                                                       \
    {                                                                          \
        T1MInjectFunc((void*)from, (void*)to);                                 \
    }

#endif
