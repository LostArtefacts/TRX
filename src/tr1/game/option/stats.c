#include "game/option/stats.h"

#include <libtrx/config.h>
#include <libtrx/game/game.h>
#include <libtrx/game/game_flow.h>
#include <libtrx/game/gym.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/sound.h>
#include <libtrx/game/ui.h>

typedef struct {
    bool is_ready;
    UI_STATS_DIALOG_STATE ui_state;
} M_PRIV;

static M_PRIV m_Priv = {};

static void M_Init(M_PRIV *const p, INVENTORY_ITEM *const inv_item)
{
    if (inv_item->object_id == O_COMPASS_OPTION
        && !g_Config.gameplay.enable_compass_stats) {
        return;
    }

    p->is_ready = true;
    UI_StatsDialog_Init(
        &p->ui_state,
        (UI_STATS_DIALOG_ARGS) {
            .mode = UI_STATS_DIALOG_MODE_LEVEL,
            .style = UI_STATS_DIALOG_STYLE_BORDERED,
            .level_num = Game_GetCurrentLevel()->num,
        });
}

static void M_Close(M_PRIV *const p)
{
    if (p->is_ready) {
        p->is_ready = false;
        UI_StatsDialog_Free(&p->ui_state);
    }
}

void Option_Stats_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }

    if (!p->is_ready) {
        M_Init(p, inv_item);
    }
    if (p->is_ready) {
        UI_StatsDialog_Control(&p->ui_state);
    }

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        M_Close(p);
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
        if (inv_item->object_id == O_STOPWATCH_OPTION) {
            Sound_StopEffect(SFX_MENU_STOPWATCH);
        }
    } else if (inv_item->object_id == O_STOPWATCH_OPTION) {
        Sound_Effect(SFX_MENU_STOPWATCH, 0, SPM_ALWAYS);
    }
}

void Option_Stats_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->is_ready) {
        UI_StatsDialog(&p->ui_state);
    }
}

void Option_Stats_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_Close(p);
}
