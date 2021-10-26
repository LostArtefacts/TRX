#include "inject.h"

#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "3dsystem/scalespr.h"
#include "game/box.h"
#include "game/camera.h"
#include "game/cinema.h"
#include "game/collide.h"
#include "game/control.h"
#include "game/demo.h"
#include "game/draw.h"
#include "game/game.h"
#include "game/health.h"
#include "game/inv.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/lot.h"
#include "game/mnsound.h"
#include "game/option.h"
#include "game/savegame.h"
#include "game/setup.h"
#include "game/sphere.h"
#include "game/text.h"
#include "specific/ati.h"
#include "specific/display.h"
#include "specific/file.h"
#include "specific/frontend.h"
#include "specific/hwr.h"
#include "specific/init.h"
#include "specific/input.h"
#include "specific/output.h"
#include "specific/shell.h"
#include "specific/smain.h"
#include "specific/sndpc.h"

void T1MInject()
{
    T1MInject3DSystem3DGen();
    T1MInject3DSystemPHDMath();
    T1MInject3DSystemScaleSpr();
    T1MInjectGameBox();
    T1MInjectGameCamera();
    T1MInjectGameCinema();
    T1MInjectGameCollide();
    T1MInjectGameControl();
    T1MInjectGameDemo();
    T1MInjectGameDraw();
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
    T1MInjectGameMNSound();
    T1MInjectGameOption();
    T1MInjectGameSaveGame();
    T1MInjectGameSetup();
    T1MInjectGameSphere();
    T1MInjectGameText();
    T1MInjectSpecificATI();
    T1MInjectSpecificDisplay();
    T1MInjectSpecificFile();
    T1MInjectSpecificFrontend();
    T1MInjectSpecificHWR();
    T1MInjectSpecificInit();
    T1MInjectSpecificInput();
    T1MInjectSpecificOutput();
    T1MInjectSpecificSMain();
    T1MInjectSpecificShell();
    T1MInjectSpecificSndPC();
}
