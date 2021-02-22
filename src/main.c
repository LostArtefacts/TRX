#include <stdio.h>
#include <windows.h>

#include "config.h"
#include "util.h"

#include "game/bat.h"
#include "game/bear.h"
#include "game/box.h"
#include "game/control.h"
#include "game/demo.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/health.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/option.h"
#include "game/setup.h"
#include "game/text.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"

HINSTANCE hInstance = NULL;

static void Tomb1MInject()
{
    Tomb1MInjectGameBat();
    Tomb1MInjectGameBear();
    Tomb1MInjectGameBox();
    Tomb1MInjectGameControl();
    Tomb1MInjectGameDemo();
    Tomb1MInjectGameDraw();
    Tomb1MInjectGameEffects();
    Tomb1MInjectGameHealth();
    Tomb1MInjectGameItems();
    Tomb1MInjectGameLOT();
    Tomb1MInjectGameLara();
    Tomb1MInjectGameLaraFire();
    Tomb1MInjectGameLaraGun1();
    Tomb1MInjectGameLaraGun2();
    Tomb1MInjectGameLaraMisc();
    Tomb1MInjectGameLaraSurf();
    Tomb1MInjectGameLaraSwim();
    Tomb1MInjectGameOption();
    Tomb1MInjectGameSetup();
    Tomb1MInjectGameText();
    Tomb1MInjectSpecificFile();
    Tomb1MInjectSpecificGame();
    Tomb1MInjectSpecificInit();
    Tomb1MInjectSpecificInput();
    Tomb1MInjectSpecificOutput();
}

BOOL APIENTRY DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        freopen("./Tomb1Main.log", "w", stdout);
        T1MReadConfig();
        TRACE("Attached");
        hInstance = hinstDLL;
        Tomb1MInject();
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
