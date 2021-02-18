#include "game/data.h"
#include "specific/init.h"
#include "util.h"
#include <stdarg.h>
#include <windows.h>

void __cdecl DB_Log(char* a1, ...)
{
    va_list va;
    char buffer[256] = { 0 };

    va_start(va, a1);
    if (!dword_45A1F0) {
        vsprintf(buffer, a1, va);
        TRACE(buffer);
        OutputDebugStringA(buffer);
        OutputDebugStringA("\n");
    }
}

void __cdecl init_game_malloc()
{
    TRACE("");
    GameAllocMemPointer = GameMemoryPointer;
    GameAllocMemFree = GameMemorySize;
    GameAllocMemUsed = 0;
}

void __cdecl game_free(int free_size)
{
    TRACE("");
    GameAllocMemPointer -= free_size;
    GameAllocMemFree += free_size;
    GameAllocMemUsed -= free_size;
}

void Tomb1MInjectSpecificInit()
{
    INJECT(0x0042A2C0, DB_Log);
    INJECT(0x0041E2C0, init_game_malloc);
    INJECT(0x0041E3B0, game_free);
}
