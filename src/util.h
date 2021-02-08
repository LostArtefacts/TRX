#ifndef TR1MAIN_UTIL_H
#define TR1MAIN_UTIL_H

#define FEATURE_NOCD_DATA
#define FEATURE_KEEP_HEALTH_BETWEEN_LEVELS
#define FEATURE_DISABLE_MEDPACKS

#include <stdio.h>

#pragma pack(push, 1)
typedef struct {
    __int8 opCode;    // must be 0xE9;
    __int32 offset;   // jump offset
} JMP;
#pragma pack(pop)

#define TRACE(...) { \
    printf("%s:%d %s ", __FILE__, __LINE__, __func__); \
    printf(__VA_ARGS__); \
    printf("\n"); \
    fflush(stdout); \
}

#define VAR_U_(address, type)           (*(type*)(address))
#define VAR_I_(address, type, value)    (*(type*)(address))
#define ARRAY_(address, type, length)   (*(type(*)length)(address))

void InjectFunc(void *from, void *to);
void PrintStackTrace();

#define INJECT(from, to) { \
    InjectFunc((void*)from, (void*)to); \
}

#endif
