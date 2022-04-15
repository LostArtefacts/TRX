#include "game/demo.h"

#include "config.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/items.h"
#include "game/output.h"
#include "game/random.h"
#include "game/room.h"
#include "game/setup.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>

static int32_t m_DemoLevel = -1;
static uint32_t *m_DemoPtr = NULL;

int32_t StartDemo(void)
{
    TEXTSTRING *txt;
    RESUME_INFO start, *s;

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

    int16_t level_num = m_DemoLevel;
    do {
        level_num++;
        if (level_num > g_GameFlow.last_level_num) {
            level_num = g_GameFlow.first_level_num;
        }
    } while (!g_GameFlow.levels[level_num].demo);
    m_DemoLevel = level_num;

    s = &g_GameInfo.start[m_DemoLevel];
    start = *s;
    s->flags.available = 1;
    s->flags.got_pistols = 1;
    s->pistol_ammo = 1000;
    s->gun_status = LGS_ARMLESS;
    s->gun_type = LGT_PISTOLS;

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // changing the controls affects negatively the original game demo data,
    // so temporarily turn off all the T1M enhancements
    int8_t old_enhanced_look = g_Config.enable_enhanced_look;
    g_Config.enable_enhanced_look = 0;

    if (InitialiseLevel(m_DemoLevel)) {
        LoadLaraDemoPos();

        Random_SeedDraw(0xD371F947);
        Random_SeedControl(0xD371F947);

        // LaraGun() expects request_gun_type to be set only when it
        // really is needed, not at all times.
        // https://github.com/rr-/Tomb1Main/issues/36
        g_Lara.request_gun_type = LGT_UNARMED;

        txt = Text_Create(0, -16, g_GameFlow.strings[GS_MISC_DEMO_MODE]);
        Text_Flash(txt, 1, 20);
        Text_AlignBottom(txt, 1);
        Text_CentreH(txt, 1);

        GameLoop(GFL_DEMO);

        Text_Remove(txt);

        *s = start;
    }

    g_Config.enable_enhanced_look = old_enhanced_look;

    return GF_EXIT_TO_TITLE;
}

void LoadLaraDemoPos(void)
{
    m_DemoPtr = g_DemoData;
    ITEM_INFO *item = g_LaraItem;
    item->pos.x = *m_DemoPtr++;
    item->pos.y = *m_DemoPtr++;
    item->pos.z = *m_DemoPtr++;
    item->pos.x_rot = *m_DemoPtr++;
    item->pos.y_rot = *m_DemoPtr++;
    item->pos.z_rot = *m_DemoPtr++;
    int16_t room_num = *m_DemoPtr++;

    if (item->room_number != room_num) {
        ItemNewRoom(g_Lara.item_number, room_num);
    }

    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);
}

bool ProcessDemoInput(void)
{
    if (m_DemoPtr >= &g_DemoData[DEMO_COUNT_MAX] || (int)*m_DemoPtr == -1) {
        return false;
    }

    // Translate demo inputs (that use TombATI key values) to T1M inputs.
    g_Input = (INPUT_STATE) {
        0,
        .forward = (bool)(*m_DemoPtr & (1 << 0)),
        .back = (bool)(*m_DemoPtr & (1 << 1)),
        .left = (bool)(*m_DemoPtr & (1 << 2)),
        .right = (bool)(*m_DemoPtr & (1 << 3)),
        .jump = (bool)(*m_DemoPtr & (1 << 4)),
        .draw = (bool)(*m_DemoPtr & (1 << 5)),
        .action = (bool)(*m_DemoPtr & (1 << 6)),
        .slow = (bool)(*m_DemoPtr & (1 << 7)),
        .option = (bool)(*m_DemoPtr & (1 << 8)),
        .look = (bool)(*m_DemoPtr & (1 << 9)),
        .step_left = (bool)(*m_DemoPtr & (1 << 10)),
        .step_right = (bool)(*m_DemoPtr & (1 << 11)),
        .roll = (bool)(*m_DemoPtr & (1 << 12)),
        .select = (bool)(*m_DemoPtr & (1 << 20)),
        .deselect = (bool)(*m_DemoPtr & (1 << 21)),
        .save = (bool)(*m_DemoPtr & (1 << 22)),
        .load = (bool)(*m_DemoPtr & (1 << 23)),
    };

    m_DemoPtr++;
    return true;
}
