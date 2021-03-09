#include "inject.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
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
#include "game/lightning.h"
#include "game/lot.h"
#include "game/moveblock.h"
#include "game/natla.h"
#include "game/objects.h"
#include "game/option.h"
#include "game/people.h"
#include "game/pickup.h"
#include "game/rat.h"
#include "game/setup.h"
#include "game/sphere.h"
#include "game/text.h"
#include "game/traps.h"
#include "game/warrior.h"
#include "game/wolf.h"
#include "specific/file.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/shell.h"
#include "specific/sndpc.h"

void T1MInject()
{
    T1MInject3DSystem3DGen();
    T1MInject3DSystemPHDMath();
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
    T1MInjectGameLightning();
    T1MInjectGameMoveBlock();
    T1MInjectGameNatla();
    T1MInjectGameObjects();
    T1MInjectGameOption();
    T1MInjectGamePeople();
    T1MInjectGamePickup();
    T1MInjectGameRat();
    T1MInjectGameSetup();
    T1MInjectGameSphere();
    T1MInjectGameText();
    T1MInjectGameTraps();
    T1MInjectGameWarrior();
    T1MInjectGameWolf();
    T1MInjectSpecificFile();
    T1MInjectSpecificInit();
    T1MInjectSpecificInput();
    T1MInjectSpecificOutput();
    T1MInjectSpecificShell();
    T1MInjectSpecificSndPC();
}
