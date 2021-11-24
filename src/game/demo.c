#include "game/demo.h"

#include "config.h"
#include "game/control.h"
#include "game/game.h"
#include "game/input.h"
#include "game/items.h"
#include "game/random.h"
#include "game/setup.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "specific/s_frontend.h"

#include <stdbool.h>

static int32_t DemoLevel = -1;
static uint32_t *DemoPtr = NULL;

int32_t StartDemo()
{
    TEXTSTRING *txt;
    START_INFO start, *s;

    bool any_demos = false;
    for (int i = g_GameFlow.first_level_num; i < g_GameFlow.last_level_num;
         i++) {
        if (g_GameFlow.levels[i].demo) {
            any_demos = true;
        }
    }
    if (!any_demos) {
        return GF_EXIT_TO_TITLE;
    }

    int16_t level_num = DemoLevel;
    do {
        level_num++;
        if (level_num > g_GameFlow.last_level_num) {
            level_num = g_GameFlow.first_level_num;
        }
    } while (!g_GameFlow.levels[level_num].demo);
    DemoLevel = level_num;

    s = &g_SaveGame.start[DemoLevel];
    start = *s;
    s->available = 1;
    s->got_pistols = 1;
    s->pistol_ammo = 1000;
    s->gun_status = LGS_ARMLESS;
    s->gun_type = LGT_PISTOLS;

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // changing the controls affects negatively the original game demo data,
    // so temporarily turn off all the T1M enhancements
    int8_t old_enhanced_look = g_Config.enable_enhanced_look;
    g_Config.enable_enhanced_look = 0;

    if (InitialiseLevel(DemoLevel, GFL_DEMO)) {
        LoadLaraDemoPos();

        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);

        txt = Text_Create(0, -16, g_GameFlow.strings[GS_MISC_DEMO_MODE]);
        Text_Flash(txt, 1, 20);
        Text_AlignBottom(txt, 1);
        Text_CentreH(txt, 1);

        GameLoop(1);

        Text_Remove(txt);

        *s = start;
        S_FadeToBlack();
    }

    g_Config.enable_enhanced_look = old_enhanced_look;

    return GF_EXIT_TO_TITLE;
}

void LoadLaraDemoPos()
{
    DemoPtr = g_DemoData;
    ITEM_INFO *item = g_LaraItem;
    item->pos.x = *DemoPtr++;
    item->pos.y = *DemoPtr++;
    item->pos.z = *DemoPtr++;
    item->pos.x_rot = *DemoPtr++;
    item->pos.y_rot = *DemoPtr++;
    item->pos.z_rot = *DemoPtr++;
    int16_t room_num = *DemoPtr++;

    if (item->room_number != room_num) {
        ItemNewRoom(g_Lara.item_number, room_num);
    }

    FLOOR_INFO *floor =
        GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
}

bool ProcessDemoInput()
{
    if (DemoPtr >= &g_DemoData[DEMO_COUNT_MAX] || (int)*DemoPtr == -1) {
        return false;
    }

    // Translate demo inputs (that use TombATI key values) to T1M inputs.
    g_Input = (INPUT_STATE) {
        0,
        .forward = (bool)(*DemoPtr & (1 << 0)),
        .back = (bool)(*DemoPtr & (1 << 1)),
        .left = (bool)(*DemoPtr & (1 << 2)),
        .right = (bool)(*DemoPtr & (1 << 3)),
        .jump = (bool)(*DemoPtr & (1 << 4)),
        .draw = (bool)(*DemoPtr & (1 << 5)),
        .action = (bool)(*DemoPtr & (1 << 6)),
        .slow = (bool)(*DemoPtr & (1 << 7)),
        .option = (bool)(*DemoPtr & (1 << 8)),
        .look = (bool)(*DemoPtr & (1 << 9)),
        .step_left = (bool)(*DemoPtr & (1 << 10)),
        .step_right = (bool)(*DemoPtr & (1 << 11)),
        .roll = (bool)(*DemoPtr & (1 << 12)),
        .select = (bool)(*DemoPtr & (1 << 20)),
        .deselect = (bool)(*DemoPtr & (1 << 21)),
        .save = (bool)(*DemoPtr & (1 << 22)),
        .load = (bool)(*DemoPtr & (1 << 23)),
    };

    DemoPtr++;
    return true;
}
