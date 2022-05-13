#include "game/demo.h"

#include "config.h"
#include "game/game.h"
#include "game/gameflow.h"
#include "game/level.h"
#include "game/random.h"
#include "game/text.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stddef.h>

static int32_t m_DemoLevel = -1;

int32_t Game_Demo(void)
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

    s = &g_GameInfo.current[m_DemoLevel];
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

    if (Level_Initialise(m_DemoLevel)) {
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

        Game_Loop(GFL_DEMO);

        Text_Remove(txt);

        *s = start;
    }

    g_Config.enable_enhanced_look = old_enhanced_look;

    return GF_EXIT_TO_TITLE;
}
