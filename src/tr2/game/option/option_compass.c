#include "game/game.h"
#include "game/option/option.h"
#include "game/savegame.h"
#include "global/vars.h"

#include <libtrx/game/input.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/ui.h>

#include <stdio.h>

typedef struct {
    bool ui_active;
    UI_STATS_DIALOG_STATE ui_state;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *const p)
{
    p->ui_active = true;
    UI_StatsDialog_Init(
        &p->ui_state,
        (UI_STATS_DIALOG_ARGS) {
            .mode = Game_IsInGym() ? UI_STATS_DIALOG_MODE_ASSAULT_COURSE
                                   : UI_STATS_DIALOG_MODE_LEVEL,
            .level_num = Game_GetCurrentLevel()->num,
            .style = UI_STATS_DIALOG_STYLE_BORDERED,
        });
}

static void M_Close(M_PRIV *const p)
{
    if (p->ui_active) {
        p->ui_active = false;
        UI_StatsDialog_Free(&p->ui_state);
    }
}

void Option_Compass_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }

    if (!p->ui_active) {
        M_Init(p);
    }
    UI_StatsDialog_Control(&p->ui_state);

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        M_Close(p);
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
        Sound_StopEffect(SFX_MENU_STOPWATCH);
    } else {
        Sound_Effect(SFX_MENU_STOPWATCH, 0, SPM_ALWAYS);
    }
}

void Option_Compass_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui_active) {
        UI_StatsDialog(&p->ui_state);
    }
}

void Option_Compass_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_Close(p);
}
