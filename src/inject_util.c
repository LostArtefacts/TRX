#include "inject_util.h"

#include "log.h"

#include <windows.h>

void T1MInjectFunc(void (*from)(void), void (*to)(void))
{
    if (from == to) {
        return;
    }
    DWORD tmp;
    LOG_DEBUG("Patching %p to %p", from, to);
    VirtualProtect(from, sizeof(JMP), PAGE_EXECUTE_READWRITE, &tmp);
    HANDLE hCurrentProcess = GetCurrentProcess();
    JMP buf;
    buf.opcode = 0xE9;
    buf.offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
    WriteProcessMemory(hCurrentProcess, from, &buf, sizeof(JMP), &tmp);
    CloseHandle(hCurrentProcess);
    // arsunt style - doesn't work because of GLRage calling VirtualProtect
    //((JMP*)(from))->opcode = 0xE9;
    //((JMP*)(from))->offset = (DWORD)(to) - ((DWORD)(from) + sizeof(JMP));
}
