#include "inject.h"

#include "3dsystem/3d_gen.h"
#include "game/bat.h"
#include "game/bear.h"
#include "game/box.h"
#include "game/camera.h"
#include "game/cinema.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/croc.h"
#include "game/demo.h"
#include "game/dino.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/health.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/option.h"
#include "game/pickup.h"
#include "game/setup.h"
#include "game/text.h"
#include "game/traps.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"

void T1MInject()
{
    T1MInject3DSystem3DGen();
    T1MInjectGameBat();
    T1MInjectGameBear();
    T1MInjectGameBox();
    T1MInjectGameCamera();
    T1MInjectGameCinema();
    T1MInjectGameCollide();
    T1MInjectGameControl();
    T1MInjectGameCroc();
    T1MInjectGameDemo();
    T1MInjectGameDino();
    T1MInjectGameDraw();
    T1MInjectGameEffects();
    T1MInjectGameGame();
    T1MInjectGameHealth();
    T1MInjectGameInvEntry();
    T1MInjectGameInvFunc();
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
    T1MInjectGameTraps();
    T1MInjectSpecificFile();
    T1MInjectSpecificInit();
    T1MInjectSpecificInput();
    T1MInjectSpecificOutput();
}
