#include "game/control.h"
#include "game/demo.h"
#include "game/game.h"
#include "game/items.h"
#include "game/setup.h"
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

void LoadLaraDemoPos()
{
    ITEM_INFO* item = LaraItem;
    item->pos.x = DemoPtr[0];
    item->pos.y = DemoPtr[1];
    item->pos.z = DemoPtr[2];
    item->pos.x_rot = DemoPtr[3];
    item->pos.y_rot = DemoPtr[4];
    item->pos.z_rot = DemoPtr[5];
    int16_t room_num = DemoPtr[6];
    DemoCount += 7;

    if (item->room_number != room_num) {
        ItemNewRoom(Lara.item_number, room_num);
    }

    FLOOR_INFO* floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
}

void GetDemoInput()
{
    if (DemoCount >= DEMO_COUNT_MAX) {
        Input = -1;
    } else {
        Input = DemoPtr[DemoCount];
    }
    if (Input != -1) {
        DemoCount++;
    }
}

void T1MInjectGameDemo()
{
    INJECT(0x00415B70, StartDemo);
    INJECT(0x00415CB0, LoadLaraDemoPos);
    INJECT(0x00415D70, GetDemoInput);
}
