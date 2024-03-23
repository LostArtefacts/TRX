#include "game/game.h"

#include "config.h"
#include "game/camera.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/items.h"
#include "game/level.h"
#include "game/phase/phase.h"
#include "game/random.h"
#include "game/room.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

static int32_t m_DemoLevel = -1;
static uint32_t *m_DemoPtr = NULL;

static void Game_Demo_Initialise(void);

static void Game_Demo_Initialise(void)
{
    g_OverlayFlag = 1;
    Camera_Initialise();

    m_DemoPtr = g_DemoData;

    ITEM_INFO *item = g_LaraItem;
    item->pos.x = *m_DemoPtr++;
    item->pos.y = *m_DemoPtr++;
    item->pos.z = *m_DemoPtr++;
    item->rot.x = *m_DemoPtr++;
    item->rot.y = *m_DemoPtr++;
    item->rot.z = *m_DemoPtr++;
    int16_t room_num = *m_DemoPtr++;

    if (item->room_number != room_num) {
        Item_NewRoom(g_Lara.item_number, room_num);
    }

    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    item->floor = Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // LaraGun() expects request_gun_type to be set only when it
    // really is needed, not at all times.
    // https://github.com/LostArtefacts/TR1X/issues/36
    g_Lara.request_gun_type = LGT_UNARMED;
}

bool Game_Demo_ProcessInput(void)
{
    if (m_DemoPtr >= &g_DemoData[DEMO_COUNT_MAX] || (int)*m_DemoPtr == -1) {
        return false;
    }

    // Translate demo inputs (that use TombATI key values) to TR1X inputs.
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
        .menu_confirm = (bool)(*m_DemoPtr & (1 << 20)),
        .menu_back = (bool)(*m_DemoPtr & (1 << 21)),
        .save = (bool)(*m_DemoPtr & (1 << 22)),
        .load = (bool)(*m_DemoPtr & (1 << 23)),
    };

    m_DemoPtr++;
    return true;
}

GAMEFLOW_OPTION Game_Demo(void)
{
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

    s = &g_GameInfo.current[m_DemoLevel];
    start = *s;
    s->flags.available = 1;
    s->flags.got_pistols = 1;
    s->pistol_ammo = 1000;
    s->gun_status = LGS_ARMLESS;
    s->gun_type = LGT_PISTOLS;
    s->lara_hitpoints = LARA_HITPOINTS;

    Random_SeedDraw(0xD371F947);
    Random_SeedControl(0xD371F947);

    // changing the controls affects negatively the original game demo data,
    // so temporarily turn off all the TR1X enhancements
    const bool old_enhanced_look = g_Config.enable_enhanced_look;
    const bool old_tr2_jumping = g_Config.enable_tr2_jumping;
    const TARGET_LOCK_MODE old_target_mode = g_Config.target_mode;
    g_Config.enable_enhanced_look = false;
    g_Config.enable_tr2_jumping = false;
    g_Config.target_mode = TLM_FULL;

    TEXTSTRING *txt =
        Text_Create(0, -16, g_GameFlow.strings[GS_MISC_DEMO_MODE]);
    Text_Flash(txt, 1, 20);
    Text_AlignBottom(txt, 1);
    Text_CentreH(txt, 1);

    if (!Level_Initialise(m_DemoLevel)) {
        goto end;
    }

    Game_Demo_Initialise();
    g_GameInfo.current_level_type = GFL_DEMO;
    Phase_Set(PHASE_GAME, NULL);
    Phase_Run();

    Text_Remove(txt);

    *s = start;

end:
    g_Config.target_mode = old_target_mode;
    g_Config.enable_enhanced_look = old_enhanced_look;
    g_Config.enable_tr2_jumping = old_tr2_jumping;
    return GF_EXIT_TO_TITLE;
}
