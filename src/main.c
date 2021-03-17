#include "init.h"
#include "config.h"
#include "inject.h"
#include "util.h"

#include <stdio.h>
#include <windows.h>

HINSTANCE hInstance = NULL;

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        freopen("./Tomb1Main.log", "w", stdout);
        T1MInit();
        T1MReadConfig();
        TRACE("Attached");
        hInstance = hinstDLL;
        T1MInject();
        break;

    case DLL_PROCESS_DETACH:
        TRACE("Detached");
        break;

    case DLL_THREAD_ATTACH:
        break;

    case DLL_THREAD_DETACH:
        break;
    }

    return TRUE;
}
