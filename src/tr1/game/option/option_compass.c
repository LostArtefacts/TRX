#include "game/option/option_compass.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"

#include <libtrx/config.h>
#include <libtrx/game/input.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/ui.h>

#include <stdint.h>
#include <stdio.h>

typedef struct {
    struct {
        bool is_ready;
        UI_STATS_DIALOG_STATE state;
    } ui;
} M_PRIV;

static M_PRIV m_Priv = {};

static int16_t m_CompassNeedle = 0;
static int16_t m_CompassSpeed = 0;

static void M_Init(M_PRIV *const p)
{
    p->ui.is_ready = true;
    UI_StatsDialog_Init(
        &p->ui.state,
        (UI_STATS_DIALOG_ARGS) {
            .mode = UI_STATS_DIALOG_MODE_LEVEL,
            .style = UI_STATS_DIALOG_STYLE_BORDERED,
            .level_num = Game_GetCurrentLevel()->num,
        });
}

static void M_Close(M_PRIV *const p)
{
    if (p->ui.is_ready) {
        p->ui.is_ready = false;
        UI_StatsDialog_Free(&p->ui.state);
    }
}

void Option_Compass_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    M_PRIV *const p = &m_Priv;
    if (is_busy) {
        return;
    }

    if (!p->ui.is_ready && g_Config.gameplay.enable_compass_stats) {
        M_Init(p);
    }
    UI_StatsDialog_Control(&p->ui.state);

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        M_Close(p);
        inv_item->anim_direction = 1;
        inv_item->goal_frame = inv_item->frames_total - 1;
    }
}

void Option_Compass_Draw(void)
{
    M_PRIV *const p = &m_Priv;
    if (p->ui.is_ready) {
        UI_StatsDialog(&p->ui.state);
    }
}

void Option_Compass_Close(void)
{
    M_PRIV *const p = &m_Priv;
    M_Close(p);
}

void Option_Compass_UpdateNeedle(const INVENTORY_ITEM *const inv_item)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return;
    }
    int16_t delta = -inv_item->y_rot - lara_item->rot.y - m_CompassNeedle;
    m_CompassSpeed = m_CompassSpeed * 19 / 20 + delta / 50;
    m_CompassNeedle += m_CompassSpeed;
}

int16_t Option_Compass_GetNeedleAngle(void)
{
    return m_CompassNeedle;
}
