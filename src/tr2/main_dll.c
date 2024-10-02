#include "inject_exec.h"
#include "lib/winmm.h"

#include <libtrx/filesystem.h>
#include <libtrx/log.h>

#include <SDL2/SDL.h>
#include <stdio.h>
#include <windows.h>

BOOL APIENTRY
DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved)
{
    switch (ul_reason_for_call) {
    case DLL_PROCESS_ATTACH:
        Log_Init(File_GetFullPath("TR2X.log"));
        LOG_DEBUG("Injected\n");

        WinMM_Load();
        Inject_Exec();

        break;

    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;

    case DLL_PROCESS_DETACH:
        LOG_DEBUG("Exiting\n");
        break;
    }
    return TRUE;
}
