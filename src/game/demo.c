#include "game/demo.h"
#include "game/game.h"
#include "game/setup.h"
#include "game/text.h"
#include "game/vars.h"
#include "specific/frontend.h"
#include "util.h"

int32_t StartDemo()
{
    TRACE("");
    TEXTSTRING* txt;
    START_INFO start, *s;

    int16_t level_num = DemoLevel;
    do {
        if (++level_num > 15) {
            level_num = 1;
        }
    } while (!DemoLevels[level_num]);
    DemoLevel = level_num;

    s = &SaveGame[0].start[DemoLevel];
    start = *s;
    s->available = 1;
    s->got_pistols = 1;
    s->pistol_ammo = 1000;
    s->gun_status = LGS_ARMLESS;
    s->gun_type = LGT_PISTOLS;

    SeedRandomDraw(0xD371F947);
    SeedRandomControl(0xD371F947);

    if (InitialiseLevel(DemoLevel)) {
        TitleLoaded = 0;

        LoadLaraDemoPos();

        SeedRandomDraw(0xD371F947);
        SeedRandomControl(0xD371F947);

#ifdef T1M_FEAT_UI
        txt = T_Print(0, -16, 0, "Demo Mode");
        T_FlashText(txt, 1, 20);
        T_BottomAlign(txt, 1);
        T_CentreH(txt, 1);
#else
        txt = T_Print(0, DumpHeight / 2 - 16, 0, "Demo Mode");
        T_FlashText(txt, 1, 20);
        T_CentreV(txt, 1);
        T_CentreH(txt, 1);
#endif

        GameLoop(1);

        T_RemovePrint(txt);

        *s = start;
        S_FadeToBlack();
    }

    return GF_EXIT_TO_TITLE;
}

void T1MInjectGameDemo()
{
    INJECT(0x00415B70, StartDemo);
}
