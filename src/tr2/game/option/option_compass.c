#include "game/game.h"
#include "game/input.h"
#include "game/option/option.h"
#include "game/requester.h"
#include "game/savegame.h"
#include "game/sound.h"
#include "global/vars.h"

#include <libtrx/game/ui2.h>

#include <stdio.h>

typedef struct {
    bool ui_active;
    UI2_STATS_DIALOG_STATE ui_state;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *p);
static void M_Shutdown(M_PRIV *p);

static void M_Init(M_PRIV *const p)
{
    p->ui_active = true;
    UI2_StatsDialog_Init(
        &p->ui_state,
        (UI2_STATS_DIALOG_ARGS) {
            .mode = Game_IsInGym() ? UI2_STATS_DIALOG_MODE_ASSAULT_COURSE
                                   : UI2_STATS_DIALOG_MODE_LEVEL,
            .level_num = Game_GetCurrentLevel()->num,
            .style = UI2_STATS_DIALOG_STYLE_BORDERED,
        });
}

static void M_Shutdown(M_PRIV *const p)
{
    if (p->ui_active) {
        p->ui_active = false;
        UI2_StatsDialog_Free(&p->ui_state);
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
    UI2_StatsDialog_Control(&p->ui_state);

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        M_Shutdown(p);
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    }

    Sound_Effect(SFX_MENU_STOPWATCH, 0, SPM_ALWAYS);
}

void Option_Compass_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui_active) {
        UI2_StatsDialog(&p->ui_state);
    }
}

void Option_Compass_Shutdown(void)
{
    M_PRIV *const p = &m_Priv;
    M_Shutdown(p);
}
