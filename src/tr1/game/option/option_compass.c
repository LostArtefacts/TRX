#include "game/option/option_compass.h"

#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/text.h"
#include "game/ui/widgets/stats_dialog.h"
#include "global/vars.h"

#include <libtrx/config.h>

#include <stdint.h>
#include <stdio.h>

static UI_WIDGET *m_Dialog = nullptr;
static int16_t m_CompassNeedle = 0;
static int16_t m_CompassSpeed = 0;

static void M_Init(void);
static void M_Shutdown(void);

static void M_Init(void)
{
    m_Dialog = UI_StatsDialog_Create((UI_STATS_DIALOG_ARGS) {
        .mode = UI_STATS_DIALOG_MODE_LEVEL,
        .style = UI_STATS_DIALOG_STYLE_BORDERED,
        .level_num = Game_GetCurrentLevel()->num,
    });
}

static void M_Shutdown(void)
{
    if (m_Dialog != nullptr) {
        m_Dialog->free(m_Dialog);
        m_Dialog = nullptr;
    }
}

void Option_Compass_Control(INVENTORY_ITEM *const inv_item, const bool is_busy)
{
    if (is_busy) {
        return;
    }

    if (g_Config.gameplay.enable_compass_stats) {
        char buf[100];
        char time_buf[100];

        if (m_Dialog == nullptr) {
            M_Init();
        }

        if (m_Dialog != nullptr) {
            m_Dialog->control(m_Dialog);
        }
    }

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        M_Shutdown();
        inv_item->goal_frame = inv_item->frames_total - 1;
        inv_item->anim_direction = 1;
    }
}

void Option_Compass_Draw(void)
{
    if (m_Dialog != nullptr) {
        m_Dialog->draw(m_Dialog);
    }
}

void Option_Compass_Shutdown(void)
{
    M_Shutdown();
}

void Option_Compass_UpdateNeedle(const INVENTORY_ITEM *const inv_item)
{
    if (g_LaraItem == nullptr) {
        return;
    }
    int16_t delta = -inv_item->y_rot - g_LaraItem->rot.y - m_CompassNeedle;
    m_CompassSpeed = m_CompassSpeed * 19 / 20 + delta / 50;
    m_CompassNeedle += m_CompassSpeed;
}

int16_t Option_Compass_GetNeedleAngle(void)
{
    return m_CompassNeedle;
}
