#include "game/ui2/dialogs/graphic_settings.h"

#include <libtrx/config.h>
#include <libtrx/debug.h>
#include <libtrx/game/game_string.h>
#include <libtrx/game/input.h>
#include <libtrx/game/ui2/elements/anchor.h>
#include <libtrx/game/ui2/elements/hide.h>
#include <libtrx/game/ui2/elements/label.h>
#include <libtrx/game/ui2/elements/modal.h>
#include <libtrx/game/ui2/elements/requester.h>
#include <libtrx/game/ui2/elements/resize.h>
#include <libtrx/game/ui2/elements/spacer.h>
#include <libtrx/game/ui2/elements/stack.h>
#include <libtrx/memory.h>
#include <libtrx/strings.h>
#include <libtrx/utils.h>

typedef struct {
    CONFIG_OPTION_TYPE option_type;
    GAME_STRING_ID label_id;
    void *target;
    int32_t min_value;
    int32_t max_value;
    int32_t delta_slow;
    int32_t delta_fast;
    int32_t misc;
} M_OPTION;

static M_OPTION m_Options[] = {
    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOG_START),
        .target = &g_Config.visuals.fog_start,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOG_END),
        .target = &g_Config.visuals.fog_end,
        .min_value = 1,
        .max_value = 100,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_R),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = 0,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_G),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = 1,
    },

    {
        .option_type = COT_RGB888,
        .label_id = GS_ID(DETAIL_WATER_COLOR_B),
        .target = &g_Config.visuals.water_color,
        .min_value = 0,
        .max_value = 255,
        .delta_slow = 1,
        .delta_fast = 10,
        .misc = 2,
    },

    {
        .option_type = COT_INT32,
        .label_id = GS_ID(DETAIL_FOV),
        .target = &g_Config.visuals.fov,
        .min_value = 30,
        .max_value = 150,
        .delta_slow = 1,
        .delta_fast = 10,
    },

    {
        .option_type = COT_BOOL,
        .label_id = GS_ID(DETAIL_USE_PSX_FOV),
        .target = &g_Config.visuals.use_psx_fov,
    },

    {
        .target = nullptr,
    },
};

static uint8_t *M_GetColorComponent(const M_OPTION *option);
static char *M_FormatRowValue(int32_t row_idx);
static bool M_CanChangeValue(int32_t row_idx, int32_t dir);
static bool M_RequestChangeValue(int32_t row_idx, int32_t dir);

static uint8_t *M_GetColorComponent(const M_OPTION *const option)
{
    RGB_888 *const color = option->target;
    switch (option->misc) {
    case 0:
        return &color->r;
    case 1:
        return &color->g;
    case 2:
        return &color->b;
    }
    ASSERT_FAIL();
    return nullptr;
}

static char *M_FormatRowValue(const int32_t row_idx)
{
    const M_OPTION *const option = &m_Options[row_idx];
    switch (option->option_type) {
    case COT_BOOL:
        return String_Format(
            "%s", *(bool *)option->target ? GS(MISC_ON) : GS(MISC_OFF));
    case COT_INT32:
        return String_Format(
            GS(DETAIL_INTEGER_FMT), *(int32_t *)option->target);
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        return String_Format("%d", *component);
    }
    default:
        break;
    }
    return nullptr;
}

static bool M_CanChangeValue(const int32_t row_idx, const int32_t dir)
{
    const M_OPTION *const option = &m_Options[row_idx];
    switch (option->option_type) {
    case COT_BOOL:
        return true;
    case COT_INT32:
        if (dir < 0) {
            return *(int32_t *)option->target > option->min_value;
        } else if (dir > 0) {
            return *(int32_t *)option->target < option->max_value;
        }
        break;
    case COT_RGB888: {
        const uint8_t *const component = M_GetColorComponent(option);
        if (dir < 0) {
            return *component > option->min_value;
        } else if (dir > 0) {
            return *component < option->max_value;
        }
        break;
    }
    default:
        break;
    }
    return false;
}

static bool M_RequestChangeValue(const int32_t row_idx, const int32_t dir)
{
    if (!M_CanChangeValue(row_idx, dir)) {
        return false;
    }

    const M_OPTION *const option = &m_Options[row_idx];
    int32_t delta = g_Input.slow ? option->delta_slow : option->delta_fast;
    if (delta == 0) {
        delta = 1;
    }
    delta *= dir;

    switch (option->option_type) {
    case COT_BOOL:
        *(bool *)option->target = !*(bool *)option->target;
        break;
    case COT_INT32:
        *(int32_t *)option->target += delta;
        break;
    case COT_RGB888: {
        uint8_t *const component = M_GetColorComponent(option);
        int32_t component_i = *component;
        component_i += delta;
        CLAMP(component_i, 0, 255);
        *component = component_i;
        break;
    }
    default:
        return false;
    }
    Config_Write();
    return true;
}

void UI2_GraphicSettings_Init(UI2_GRAPHIC_SETTINGS_STATE *const s)
{
    int32_t row_count = 0;
    for (int32_t i = 0; m_Options[i].target != nullptr; i++) {
        row_count++;
    }
    UI2_Requester_Init(&s->req, row_count, row_count, true);
    s->req.row_pad = 2.0f;
    s->req.row_spacing = 0.0f;
}

void UI2_GraphicSettings_Free(UI2_GRAPHIC_SETTINGS_STATE *const s)
{
    UI2_Requester_Free(&s->req);
}

bool UI2_GraphicSettings_Control(UI2_GRAPHIC_SETTINGS_STATE *const s)
{
    const int32_t choice = UI2_Requester_Control(&s->req);
    if (choice == UI2_REQUESTER_CANCEL) {
        return true;
    }
    const int32_t sel_row = UI2_Requester_GetCurrentRow(&s->req);
    if (g_InputDB.menu_left && sel_row >= 0) {
        M_RequestChangeValue(sel_row, -1);
    } else if (g_InputDB.menu_right && sel_row >= 0) {
        M_RequestChangeValue(sel_row, +1);
    }
    return false;
}

void UI2_GraphicSettings(UI2_GRAPHIC_SETTINGS_STATE *const s)
{
    const int32_t sel_row = UI2_Requester_GetCurrentRow(&s->req);
    UI2_BeginModal(0.5f, 0.6f);
    UI2_BeginRequester(&s->req, GS(DETAIL_TITLE));
    for (int32_t i = UI2_Requester_GetFirstRow(&s->req);
         i < UI2_Requester_GetLastRow(&s->req); i++) {
        UI2_BeginRequesterRow(&s->req, i);
        UI2_BeginStack(UI2_STACK_HORIZONTAL);
        UI2_Label(GameString_Get(m_Options[i].label_id));
        UI2_Spacer(20.0f, 0.0f);

        UI2_BeginResize(70.0f, -1.0f);
        UI2_BeginStackEx((UI2_STACK_SETTINGS) {
            .orientation = UI2_STACK_HORIZONTAL,
            .align = { .h = UI2_STACK_H_ALIGN_CENTER },
            .spacing = { .h = 5.0f },
        });
        UI2_BeginHide(i != sel_row || !M_CanChangeValue(i, -1));
        UI2_Label("\\{button left}");
        UI2_EndHide();
        UI2_Label(M_FormatRowValue(i));
        UI2_BeginHide(i != sel_row || !M_CanChangeValue(i, +1));
        UI2_Label("\\{button right}");
        UI2_EndHide();
        UI2_EndStack();
        UI2_EndResize();

        UI2_EndStack();
        UI2_EndRequesterRow(&s->req, i);
    }
    UI2_EndRequester(&s->req);
    UI2_EndModal();
}
