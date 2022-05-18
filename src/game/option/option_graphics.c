#include "game/option/option_graphics.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/screen.h"
#include "game/settings.h"
#include "game/text.h"
#include "gfx/context.h"
#include "global/vars.h"

#include <stdio.h>

#define TOP_Y -55
#define BORDER 4
#define ROW_HEIGHT 17
#define ROW_WIDTH 300
#define OPTION_LENGTH 256
#define LEFT_ARROW_OFFSET 20
#define RIGHT_ARROW_OFFSET 35

typedef enum GRAPHICS_TEXT {
    TEXT_PERSPECTIVE = 0,
    TEXT_BILINEAR = 1,
    TEXT_VSYNC = 2,
    TEXT_BRIGHTNESS = 3,
    TEXT_UI_TEXT_SCALE = 4,
    TEXT_UI_BAR_SCALE = 5,
    TEXT_RESOLUTION = 6,
    TEXT_TITLE = 7,
    TEXT_TITLE_BORDER = 8,
    TEXT_PERSPECTIVE_TOGGLE = 9,
    TEXT_BILINEAR_TOGGLE = 10,
    TEXT_VSYNC_TOGGLE = 11,
    TEXT_BRIGHTNESS_TOGGLE = 12,
    TEXT_UI_TEXT_SCALE_TOGGLE = 13,
    TEXT_UI_BAR_SCALE_TOGGLE = 14,
    TEXT_RESOLUTION_TOGGLE = 15,
    TEXT_LEFT_ARROW = 16,
    TEXT_RIGHT_ARROW = 17,
    TEXT_ROW_SELECT = 18,
    TEXT_NUMBER_OF = 19,
    TEXT_EMPTY_ENTRY = -1,
    TEXT_OPTION_MIN = TEXT_PERSPECTIVE,
    TEXT_OPTION_MAX = TEXT_RESOLUTION,
} GRAPHICS_TEXT;

typedef struct TEXT_COLUMN_PLACEMENT {
    GRAPHICS_TEXT option;
    int col_num;
    GAME_STRING_ID text;
} TEXT_COLUMN_PLACEMENT;

static const TEXT_COLUMN_PLACEMENT m_GfxTextPlacement[] = {
    // left column
    { TEXT_PERSPECTIVE, 0, GS_DETAIL_PERSPECTIVE },
    { TEXT_BILINEAR, 0, GS_DETAIL_BILINEAR },
    { TEXT_VSYNC, 0, GS_DETAIL_VSYNC },
    { TEXT_BRIGHTNESS, 0, GS_DETAIL_BRIGHTNESS },
    { TEXT_UI_TEXT_SCALE, 0, GS_DETAIL_UI_TEXT_SCALE },
    { TEXT_UI_BAR_SCALE, 0, GS_DETAIL_UI_BAR_SCALE },
    { TEXT_RESOLUTION, 0, GS_DETAIL_RESOLUTION },
    // right column
    { TEXT_PERSPECTIVE_TOGGLE, 1, GS_MISC_ON },
    { TEXT_BILINEAR_TOGGLE, 1, GS_MISC_ON },
    { TEXT_VSYNC_TOGGLE, 1, GS_MISC_ON },
    { TEXT_BRIGHTNESS_TOGGLE, 1, GS_DETAIL_FLOAT_FMT },
    { TEXT_UI_TEXT_SCALE_TOGGLE, 1, GS_DETAIL_FLOAT_FMT },
    { TEXT_UI_BAR_SCALE_TOGGLE, 1, GS_DETAIL_FLOAT_FMT },
    { TEXT_RESOLUTION_TOGGLE, 1, GS_DETAIL_RESOLUTION_FMT },
    // end
    { TEXT_EMPTY_ENTRY, -1, -1 },
};

static bool m_IsTextInit = false;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };
static bool m_HideArrowLeft = false;
static bool m_HideArrowRight = false;

static void Option_GraphicsInitText(void);
static void Option_GraphicsShutdownText(void);
static void Option_GraphicsUpdateArrows(void);
static void Option_GraphicsChangeTextOption(int32_t option_num);
static int16_t Option_GraphicsPlaceColumns(bool create);

static void Option_GraphicsInitText(void)
{
    char buf[OPTION_LENGTH];

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, TOP_Y - 2, " ");
    Text_CentreH(m_Text[TEXT_TITLE_BORDER], 1);
    Text_CentreV(m_Text[TEXT_TITLE_BORDER], 1);

    m_Text[TEXT_TITLE] =
        Text_Create(0, TOP_Y, g_GameFlow.strings[GS_DETAIL_SELECT_DETAIL]);
    Text_CentreH(m_Text[TEXT_TITLE], 1);
    Text_CentreV(m_Text[TEXT_TITLE], 1);
    Text_AddBackground(m_Text[TEXT_TITLE], ROW_WIDTH - 4, 0, 0, 0);
    Text_AddOutline(m_Text[TEXT_TITLE], 1);

    if (g_OptionSelected < TEXT_OPTION_MIN) {
        g_OptionSelected = TEXT_OPTION_MIN;
    }
    if (g_OptionSelected > TEXT_OPTION_MAX) {
        g_OptionSelected = TEXT_OPTION_MAX;
    }

    int16_t max_y = Option_GraphicsPlaceColumns(true);

    int16_t width = ROW_WIDTH;
    int16_t height = max_y + BORDER * 2 - TOP_Y;
    Text_AddBackground(m_Text[TEXT_TITLE_BORDER], width, height, 0, 0);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], 1);

    m_Text[TEXT_LEFT_ARROW] = Text_Create(0, 0, "\200");
    Text_CentreV(m_Text[TEXT_LEFT_ARROW], 1);
    Text_SetPos(
        m_Text[TEXT_LEFT_ARROW], m_Text[TEXT_PERSPECTIVE_TOGGLE]->pos.x - 20,
        m_Text[TEXT_PERSPECTIVE_TOGGLE]->pos.y);
    m_HideArrowLeft =
        g_Config.rendering.enable_perspective_filter ? false : true;

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(0, 0, "\201");
    Text_CentreV(m_Text[TEXT_RIGHT_ARROW], 1);
    Text_SetPos(
        m_Text[TEXT_RIGHT_ARROW], m_Text[TEXT_PERSPECTIVE_TOGGLE]->pos.x + 40,
        m_Text[TEXT_PERSPECTIVE_TOGGLE]->pos.y);
    m_HideArrowRight =
        g_Config.rendering.enable_perspective_filter ? true : false;

    const int16_t centre = Screen_GetResWidthDownscaled() / 2;
    m_Text[TEXT_ROW_SELECT] =
        Text_Create(centre - 3, TOP_Y + ROW_HEIGHT + BORDER * 2, " ");
    Text_CentreV(m_Text[TEXT_ROW_SELECT], 1);
    Text_AddBackground(m_Text[TEXT_ROW_SELECT], ROW_WIDTH - 7, 0, 0, 0);
    Text_AddOutline(m_Text[TEXT_ROW_SELECT], 1);

    Option_GraphicsChangeTextOption(TEXT_PERSPECTIVE);
    Option_GraphicsChangeTextOption(TEXT_BILINEAR);
    Option_GraphicsChangeTextOption(TEXT_VSYNC);
    Option_GraphicsChangeTextOption(TEXT_BRIGHTNESS);
    Option_GraphicsChangeTextOption(TEXT_UI_TEXT_SCALE);
    Option_GraphicsChangeTextOption(TEXT_UI_BAR_SCALE);
    Option_GraphicsChangeTextOption(TEXT_RESOLUTION);
}

static void Option_GraphicsShutdownText(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = NULL;
    }
}

static void Option_GraphicsUpdateArrows(void)
{
    int16_t resolution_offset = 0;

    switch (g_OptionSelected) {
    case TEXT_PERSPECTIVE:
        m_HideArrowLeft = !g_Config.rendering.enable_perspective_filter;
        m_HideArrowRight = g_Config.rendering.enable_perspective_filter;
        break;
    case TEXT_BILINEAR:
        m_HideArrowLeft = !g_Config.rendering.enable_bilinear_filter;
        m_HideArrowRight = g_Config.rendering.enable_bilinear_filter;
        break;
    case TEXT_VSYNC:
        m_HideArrowLeft = !g_Config.rendering.enable_vsync;
        m_HideArrowRight = g_Config.rendering.enable_vsync;
        break;
    case TEXT_BRIGHTNESS:
        m_HideArrowLeft = g_Config.brightness <= MIN_BRIGHTNESS;
        m_HideArrowRight = g_Config.brightness >= MAX_BRIGHTNESS;
        break;
    case TEXT_UI_TEXT_SCALE:
        m_HideArrowLeft = g_Config.ui.text_scale <= MIN_UI_SCALE;
        m_HideArrowRight = g_Config.ui.text_scale >= MAX_UI_SCALE;
        break;
    case TEXT_UI_BAR_SCALE:
        m_HideArrowLeft = g_Config.ui.bar_scale <= MIN_UI_SCALE;
        m_HideArrowRight = g_Config.ui.bar_scale >= MAX_UI_SCALE;
        break;
    case TEXT_RESOLUTION:
        resolution_offset = 70;
        m_HideArrowLeft = Screen_GetPendingResIdx() == 0;
        m_HideArrowRight = Screen_GetPendingResIdx() == (RESOLUTIONS_SIZE - 1);
        break;
    }

    Text_SetPos(
        m_Text[TEXT_LEFT_ARROW],
        m_Text[g_OptionSelected + TEXT_PERSPECTIVE_TOGGLE]->pos.x
            - LEFT_ARROW_OFFSET,
        m_Text[g_OptionSelected + TEXT_PERSPECTIVE_TOGGLE]->pos.y);
    Text_SetPos(
        m_Text[TEXT_RIGHT_ARROW],
        m_Text[g_OptionSelected + TEXT_PERSPECTIVE_TOGGLE]->pos.x
            + RIGHT_ARROW_OFFSET + resolution_offset,
        m_Text[g_OptionSelected + TEXT_PERSPECTIVE_TOGGLE]->pos.y);

    Text_Hide(m_Text[TEXT_LEFT_ARROW], m_HideArrowLeft);
    Text_Hide(m_Text[TEXT_RIGHT_ARROW], m_HideArrowRight);
}

static int16_t Option_GraphicsPlaceColumns(bool create)
{
    const int16_t centre = Screen_GetResWidthDownscaled() / 2;

    int16_t max_y = 0;
    int16_t xs[2] = { centre - 142, centre + 30 };
    int16_t ys[2] = { TOP_Y + ROW_HEIGHT + BORDER * 2,
                      TOP_Y + ROW_HEIGHT + BORDER * 2 };

    for (const TEXT_COLUMN_PLACEMENT *col = m_GfxTextPlacement;
         col->col_num >= 0 && col->col_num <= 1; col++) {
        int16_t x = xs[col->col_num];
        int16_t y = ys[col->col_num];

        if (col->option != TEXT_EMPTY_ENTRY) {
            if (create) {
                m_Text[col->option] =
                    Text_Create(x, y, g_GameFlow.strings[col->text]);
            } else {
                Text_SetPos(m_Text[col->option], x, y);
            }
            Text_CentreV(m_Text[col->option], 1);
        }

        ys[col->col_num] += ROW_HEIGHT;
        max_y = MAX(max_y, ys[col->col_num]);
    }
    return max_y;
}

static void Option_GraphicsChangeTextOption(int32_t option_num)
{
    char buf[OPTION_LENGTH];

    switch (option_num) {
    case TEXT_PERSPECTIVE:
        Text_ChangeText(
            m_Text[TEXT_PERSPECTIVE_TOGGLE],
            g_GameFlow.strings
                [g_Config.rendering.enable_perspective_filter ? GS_MISC_ON
                                                              : GS_MISC_OFF]);
        break;

    case TEXT_BILINEAR:
        Text_ChangeText(
            m_Text[TEXT_BILINEAR_TOGGLE],
            g_GameFlow.strings
                [g_Config.rendering.enable_bilinear_filter ? GS_MISC_ON
                                                           : GS_MISC_OFF]);
        break;

    case TEXT_VSYNC:
        Text_ChangeText(
            m_Text[TEXT_VSYNC_TOGGLE],
            g_GameFlow.strings
                [g_Config.rendering.enable_vsync ? GS_MISC_ON : GS_MISC_OFF]);
        break;

    case TEXT_BRIGHTNESS:
        sprintf(
            buf, g_GameFlow.strings[GS_DETAIL_FLOAT_FMT], g_Config.brightness);
        Text_ChangeText(m_Text[TEXT_BRIGHTNESS_TOGGLE], buf);
        break;

    case TEXT_UI_TEXT_SCALE:
        sprintf(
            buf, g_GameFlow.strings[GS_DETAIL_FLOAT_FMT],
            g_Config.ui.text_scale);
        Text_ChangeText(m_Text[TEXT_UI_TEXT_SCALE_TOGGLE], buf);
        Option_GraphicsPlaceColumns(false);
        Text_SetPos(
            m_Text[TEXT_ROW_SELECT], Screen_GetResWidthDownscaled() / 2 - 3,
            TOP_Y + ROW_HEIGHT + BORDER * 2);
        break;

    case TEXT_UI_BAR_SCALE:
        sprintf(
            buf, g_GameFlow.strings[GS_DETAIL_FLOAT_FMT],
            g_Config.ui.bar_scale);
        Text_ChangeText(m_Text[TEXT_UI_BAR_SCALE_TOGGLE], buf);
        break;

    case TEXT_RESOLUTION:
        sprintf(
            buf, g_GameFlow.strings[GS_DETAIL_RESOLUTION_FMT],
            Screen_GetPendingResWidth(), Screen_GetPendingResHeight());
        Text_ChangeText(m_Text[TEXT_RESOLUTION_TOGGLE], buf);
        break;
    }
}

void Option_Graphics(INVENTORY_ITEM *inv_item)
{
    if (!m_IsTextInit) {
        Option_GraphicsInitText();
        m_IsTextInit = true;
    }

    if (g_InputDB.forward && g_OptionSelected > TEXT_OPTION_MIN) {
        g_OptionSelected--;
        Text_AddBackground(
            m_Text[TEXT_ROW_SELECT], ROW_WIDTH - 7, 0, 0,
            ROW_HEIGHT * g_OptionSelected);
        Text_AddOutline(m_Text[TEXT_ROW_SELECT], 1);
    }

    if (g_InputDB.back && g_OptionSelected < TEXT_OPTION_MAX) {
        g_OptionSelected++;
        Text_AddBackground(
            m_Text[TEXT_ROW_SELECT], ROW_WIDTH - 7, 0, 0,
            ROW_HEIGHT * g_OptionSelected);
        Text_AddOutline(m_Text[TEXT_ROW_SELECT], 1);
    }

    int32_t reset = -1;

    if (g_InputDB.right) {
        switch (g_OptionSelected) {
        case TEXT_PERSPECTIVE:
            if (!g_Config.rendering.enable_perspective_filter) {
                g_Config.rendering.enable_perspective_filter = true;
                reset = TEXT_PERSPECTIVE;
            }
            break;

        case TEXT_BILINEAR:
            if (!g_Config.rendering.enable_bilinear_filter) {
                g_Config.rendering.enable_bilinear_filter = true;
                reset = TEXT_BILINEAR;
            }
            break;

        case TEXT_VSYNC:
            if (!g_Config.rendering.enable_vsync) {
                g_Config.rendering.enable_vsync = true;
                reset = TEXT_VSYNC;
                GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
            }
            break;

        case TEXT_BRIGHTNESS:
            if (g_Config.brightness < MAX_BRIGHTNESS) {
                g_Config.brightness += 0.1f;
                reset = TEXT_BRIGHTNESS;
            }
            break;

        case TEXT_UI_TEXT_SCALE:
            if (g_Config.ui.text_scale < MAX_UI_SCALE) {
                g_Config.ui.text_scale += 0.1;
                reset = TEXT_UI_TEXT_SCALE;
            }
            break;

        case TEXT_UI_BAR_SCALE:
            if (g_Config.ui.bar_scale < MAX_UI_SCALE) {
                g_Config.ui.bar_scale += 0.1;
                reset = TEXT_UI_BAR_SCALE;
            }
            break;

        case TEXT_RESOLUTION:
            if (Screen_SetNextRes()) {
                reset = TEXT_RESOLUTION;
            }
            break;
        }
    }

    if (g_InputDB.left) {
        switch (g_OptionSelected) {
        case TEXT_PERSPECTIVE:
            if (g_Config.rendering.enable_perspective_filter) {
                g_Config.rendering.enable_perspective_filter = false;
                reset = TEXT_PERSPECTIVE;
            }
            break;

        case TEXT_BILINEAR:
            if (g_Config.rendering.enable_bilinear_filter) {
                g_Config.rendering.enable_bilinear_filter = false;
                reset = TEXT_BILINEAR;
            }
            break;

        case TEXT_VSYNC:
            if (g_Config.rendering.enable_vsync) {
                g_Config.rendering.enable_vsync = false;
                reset = TEXT_VSYNC;
                GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
            }
            break;

        case TEXT_BRIGHTNESS:
            if (g_Config.brightness > MIN_BRIGHTNESS) {
                g_Config.brightness -= 0.1f;
                reset = TEXT_BRIGHTNESS;
            }
            break;

        case TEXT_UI_TEXT_SCALE:
            if (g_Config.ui.text_scale > MIN_UI_SCALE) {
                g_Config.ui.text_scale -= 0.1;
                reset = TEXT_UI_TEXT_SCALE;
            }
            break;

        case TEXT_UI_BAR_SCALE:
            if (g_Config.ui.bar_scale > MIN_UI_SCALE) {
                g_Config.ui.bar_scale -= 0.1;
                reset = TEXT_UI_BAR_SCALE;
            }
            break;

        case TEXT_RESOLUTION:
            if (Screen_SetPrevRes()) {
                reset = TEXT_RESOLUTION;
            }
            break;
        }
    }

    if (g_InputDB.deselect || g_InputDB.select) {
        reset = TEXT_NUMBER_OF;
    }

    if (reset > -1) {
        Option_GraphicsChangeTextOption(reset);
        Settings_Write();
    }

    Option_GraphicsUpdateArrows();

    if (reset == TEXT_NUMBER_OF) {
        Option_GraphicsShutdownText();
        m_IsTextInit = false;
        m_HideArrowLeft = false;
        m_HideArrowRight = false;
        Settings_Write();
    }
}
