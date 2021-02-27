#include "inject.h"

#include "game/bat.h"
#include "game/bear.h"
#include "game/box.h"
#include "game/camera.h"
#include "game/collide.h"
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
#include "game/pickup.h"
#include "game/setup.h"
#include "game/text.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"

void T1MInject()
{
    T1MInjectGameBat();
    T1MInjectGameBear();
    T1MInjectGameBox();
    T1MInjectGameCamera();
    T1MInjectGameCollide();
    T1MInjectGameControl();
    T1MInjectGameDemo();
    T1MInjectGameDraw();
    T1MInjectGameEffects();
    T1MInjectGameHealth();
    T1MInjectGameItems();
    T1MInjectGameLOT();
    T1MInjectGameLara();
    T1MInjectGameLaraFire();
    T1MInjectGameLaraGun1();
    T1MInjectGameLaraGun2();
    T1MInjectGameLaraMisc();
    T1MInjectGameLaraSurf();
    T1MInjectGameLaraSwim();
    T1MInjectGameOption();
    T1MInjectGamePickup();
    T1MInjectGameSetup();
    T1MInjectGameText();
    T1MInjectSpecificFile();
    T1MInjectSpecificGame();
    T1MInjectSpecificInit();
    T1MInjectSpecificInput();
    T1MInjectSpecificOutput();
}
