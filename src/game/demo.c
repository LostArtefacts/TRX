#include "game/demo.h"

#include "config.h"
#include "game/control.h"
#include "game/game.h"
#include "game/items.h"
#include "game/setup.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/frontend.h"

#include <stdbool.h>

static int32_t DemoLevel = -1;

int32_t StartDemo()
{
    TEXTSTRING *txt;
    START_INFO start, *s;

    int16_t level_num = DemoLevel;
    do {
        level_num++;
        if (level_num > GF.last_level_num) {
            level_num = GF.first_level_num;
        }
    } while (!GF.levels[level_num].demo);
    DemoLevel = level_num;

    s = &SaveGame.start[DemoLevel];
    start = *s;
    s->available = 1;
    s->got_pistols = 1;
    s->pistol_ammo = 1000;
    s->gun_status = LGS_ARMLESS;
    s->gun_type = LGT_PISTOLS;

    SeedRandomDraw(0xD371F947);
    SeedRandomControl(0xD371F947);

    // changing the controls affects negatively the original game demo data,
    // so temporarily turn off all the T1M enhancements
    int8_t old_enhanced_look = T1MConfig.enable_enhanced_look;
    T1MConfig.enable_enhanced_look = 0;

    if (InitialiseLevel(DemoLevel, GFL_DEMO)) {
        LoadLaraDemoPos();

        SeedRandomDraw(0xD371F947);
        SeedRandomControl(0xD371F947);

        txt = T_Print(0, -16, GF.strings[GS_MISC_DEMO_MODE]);
        T_FlashText(txt, 1, 20);
        T_BottomAlign(txt, 1);
        T_CentreH(txt, 1);

        GameLoop(1);

        T_RemovePrint(txt);

        *s = start;
        S_FadeToBlack();
    }

    T1MConfig.enable_enhanced_look = old_enhanced_look;

    return GF_EXIT_TO_TITLE;
}

void LoadLaraDemoPos()
{
    ITEM_INFO *item = LaraItem;
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

    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
}

bool ProcessDemoInput()
{
    if (DemoCount >= DEMO_COUNT_MAX || DemoPtr[DemoCount] == -1) {
        return false;
    }

    // Translate demo inputs (that use TombATI key values) to T1M inputs.
    Input = (INPUT_STATE) {
        0,
        .forward = (bool)(DemoPtr[DemoCount] & (1 << 0)),
        .back = (bool)(DemoPtr[DemoCount] & (1 << 1)),
        .left = (bool)(DemoPtr[DemoCount] & (1 << 2)),
        .right = (bool)(DemoPtr[DemoCount] & (1 << 3)),
        .jump = (bool)(DemoPtr[DemoCount] & (1 << 4)),
        .draw = (bool)(DemoPtr[DemoCount] & (1 << 5)),
        .action = (bool)(DemoPtr[DemoCount] & (1 << 6)),
        .slow = (bool)(DemoPtr[DemoCount] & (1 << 7)),
        .option = (bool)(DemoPtr[DemoCount] & (1 << 8)),
        .look = (bool)(DemoPtr[DemoCount] & (1 << 9)),
        .step_left = (bool)(DemoPtr[DemoCount] & (1 << 10)),
        .step_right = (bool)(DemoPtr[DemoCount] & (1 << 11)),
        .roll = (bool)(DemoPtr[DemoCount] & (1 << 12)),
        .select = (bool)(DemoPtr[DemoCount] & (1 << 20)),
        .deselect = (bool)(DemoPtr[DemoCount] & (1 << 21)),
        .save = (bool)(DemoPtr[DemoCount] & (1 << 22)),
        .load = (bool)(DemoPtr[DemoCount] & (1 << 23)),
    };

    DemoCount++;
    return true;
}
