#include "game/option/option_graphics.h"

#include "game/game_string.h"
#include "game/input.h"
#include "game/output.h"
#include "game/screen.h"
#include "game/text.h"
#include "global/const.h"

#include <libtrx/config.h>
#include <libtrx/gfx/context.h>
#include <libtrx/utils.h>

#include <stdint.h>
#include <stdio.h>

#define TOP_Y (-85)
#define BORDER 8
#define COL_OFFSET_0 (-142)
#define COL_OFFSET_1 10
#define ROW_HEIGHT 17
#define ROW_WIDTH 300
#define OPTION_LENGTH 256
#define LEFT_ARROW_OFFSET (-20)
#define RIGHT_ARROW_OFFSET_MIN 35
#define RIGHT_ARROW_OFFSET_MAX 85

typedef enum {
    TEXT_TITLE,
    TEXT_TITLE_BORDER,
    TEXT_LEFT_ARROW,
    TEXT_RIGHT_ARROW,
    TEXT_UP_ARROW,
    TEXT_DOWN_ARROW,
    TEXT_ROW_SELECT,
    TEXT_NUMBER_OF,
} GRAPHICS_TEXT;

typedef enum {
    OPTION_FPS,
    OPTION_TEXTURE_FILTER,
    OPTION_FBO_FILTER,
    OPTION_VSYNC,
    OPTION_BRIGHTNESS,
    OPTION_UI_TEXT_SCALE,
    OPTION_UI_BAR_SCALE,
    OPTION_RENDER_MODE,
    OPTION_RESOLUTION,
    OPTION_PERSPECTIVE,
    OPTION_PRETTY_PIXELS,
    OPTION_REFLECTIONS,
    OPTION_NUMBER_OF,
    OPTION_MIN = OPTION_FPS,
    OPTION_MAX = OPTION_REFLECTIONS,
} GRAPHICS_OPTION_NAME;

typedef struct {
    GRAPHICS_OPTION_NAME option_name;
    GAME_STRING_ID option_string;
    GAME_STRING_ID value_string;
} GRAPHICS_OPTION_ROW;

typedef struct {
    const GRAPHICS_OPTION_ROW *first_option;
    const GRAPHICS_OPTION_ROW *last_option;
    const GRAPHICS_OPTION_ROW *cur_option;
    int32_t row_num;
    int32_t num_vis_options;
    const GRAPHICS_OPTION_ROW *first_visible;
    const GRAPHICS_OPTION_ROW *last_visible;
    TEXTSTRING *option_texts[OPTION_NUMBER_OF];
    TEXTSTRING *value_texts[OPTION_NUMBER_OF];
} GRAPHICS_MENU;

static const GRAPHICS_OPTION_ROW m_GfxOptionRows[] = {
    { OPTION_FPS, GS_ID(DETAIL_FPS), GS_ID(DETAIL_DECIMAL_FMT) },
    { OPTION_TEXTURE_FILTER, GS_ID(DETAIL_TEXTURE_FILTER), GS_ID(MISC_OFF) },
    { OPTION_FBO_FILTER, GS_ID(DETAIL_FBO_FILTER), GS_ID(MISC_OFF) },
    { OPTION_VSYNC, GS_ID(DETAIL_VSYNC), GS_ID(MISC_ON) },
    { OPTION_BRIGHTNESS, GS_ID(DETAIL_BRIGHTNESS), GS_ID(DETAIL_FLOAT_FMT) },
    { OPTION_UI_TEXT_SCALE, GS_ID(DETAIL_UI_TEXT_SCALE),
      GS_ID(DETAIL_FLOAT_FMT) },
    { OPTION_UI_BAR_SCALE, GS_ID(DETAIL_UI_BAR_SCALE),
      GS_ID(DETAIL_FLOAT_FMT) },
    { OPTION_RENDER_MODE, GS_ID(DETAIL_RENDER_MODE), GS_ID(DETAIL_STRING_FMT) },
    { OPTION_RESOLUTION, GS_ID(DETAIL_RESOLUTION),
      GS_ID(DETAIL_RESOLUTION_FMT) },
    { OPTION_PERSPECTIVE, GS_ID(DETAIL_PERSPECTIVE), GS_ID(MISC_ON) },
    { OPTION_PRETTY_PIXELS, GS_ID(DETAIL_PRETTY_PIXELS), GS_ID(MISC_ON) },
    { OPTION_REFLECTIONS, GS_ID(DETAIL_REFLECTIONS), GS_ID(MISC_ON) },
    // end
    { OPTION_NUMBER_OF, 0, 0 },
};

static GRAPHICS_MENU m_GraphicsMenu = {};

static bool m_IsTextInit = false;
static bool m_HideArrowLeft = false;
static bool m_HideArrowRight = false;
static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = {};

static void M_InitMenu(void);
static void M_UpdateMenuVisible(void);
static void M_Reinitialize(GRAPHICS_OPTION_NAME starting_option);
static void M_MenuUp(void);
static void M_MenuDown(void);
static void M_InitText(void);
static void M_UpdateText(void);
static void M_UpdateArrows(
    GRAPHICS_OPTION_NAME option_name, TEXTSTRING value_text, bool more_up,
    bool more_down);
static void M_ChangeTextOption(
    const GRAPHICS_OPTION_ROW *row, TEXTSTRING *option_text,
    TEXTSTRING *value_text);
static int16_t M_PlaceColumns(bool create);

static void M_InitMenu(void)
{
    m_GraphicsMenu.first_option = &m_GfxOptionRows[OPTION_MIN];
    m_GraphicsMenu.last_option = &m_GfxOptionRows[OPTION_MAX];
    m_GraphicsMenu.cur_option = &m_GfxOptionRows[OPTION_MIN];
    m_GraphicsMenu.row_num = 0;
    m_GraphicsMenu.num_vis_options = 0;
    m_GraphicsMenu.first_visible = &m_GfxOptionRows[OPTION_MIN];
    m_GraphicsMenu.last_visible = &m_GfxOptionRows[OPTION_MIN];
}

static void M_UpdateMenuVisible(void)
{
    int32_t visible_lines = 0;
    if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 260) {
        visible_lines = 8;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 384) {
        visible_lines = 9;
    } else if (Screen_GetResHeightDownscaled(RSR_TEXT) <= 480) {
        visible_lines = 12;
    } else {
        visible_lines = 16;
    }
    m_GraphicsMenu.num_vis_options = MIN(OPTION_NUMBER_OF, visible_lines);
    m_GraphicsMenu.first_visible = &m_GfxOptionRows[OPTION_MIN];
    m_GraphicsMenu.last_visible =
        &m_GfxOptionRows[OPTION_MIN] + m_GraphicsMenu.num_vis_options - 1;
}

static void M_Reinitialize(GRAPHICS_OPTION_NAME starting_option)
{
    Option_Graphics_Shutdown();
    M_InitText();
    m_IsTextInit = true;
    for (const GRAPHICS_OPTION_ROW *row = m_GraphicsMenu.first_option;
         row->option_name != starting_option; row++) {
        M_MenuDown();
    }
}

static void M_MenuUp(void)
{
    if (m_GraphicsMenu.cur_option != m_GraphicsMenu.first_option) {
        if (m_GraphicsMenu.cur_option == m_GraphicsMenu.first_visible) {
            m_GraphicsMenu.first_visible--;
            m_GraphicsMenu.last_visible--;
        } else {
            m_GraphicsMenu.row_num--;
        }
        m_GraphicsMenu.cur_option--;
        M_UpdateText();
    }
}

static void M_MenuDown(void)
{
    if (m_GraphicsMenu.cur_option != m_GraphicsMenu.last_option) {
        if (m_GraphicsMenu.cur_option == m_GraphicsMenu.last_visible) {
            m_GraphicsMenu.first_visible++;
            m_GraphicsMenu.last_visible++;
        } else {
            m_GraphicsMenu.row_num++;
        }
        m_GraphicsMenu.cur_option++;
        M_UpdateText();
    }
}

static void M_InitText(void)
{
    M_InitMenu();
    M_UpdateMenuVisible();

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, TOP_Y - 2, " ");
    Text_CentreH(m_Text[TEXT_TITLE_BORDER], 1);
    Text_CentreV(m_Text[TEXT_TITLE_BORDER], 1);

    m_Text[TEXT_TITLE] = Text_Create(0, TOP_Y, GS(DETAIL_SELECT_DETAIL));
    Text_CentreH(m_Text[TEXT_TITLE], 1);
    Text_CentreV(m_Text[TEXT_TITLE], 1);
    Text_AddBackground(m_Text[TEXT_TITLE], ROW_WIDTH - 4, 0, 0, 0, TS_HEADING);
    Text_AddOutline(m_Text[TEXT_TITLE], TS_HEADING);

    int16_t max_y = M_PlaceColumns(true);

    int16_t width = ROW_WIDTH;
    int16_t height = max_y + BORDER * 2 - TOP_Y;
    Text_AddBackground(
        m_Text[TEXT_TITLE_BORDER], width, height, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], TS_BACKGROUND);

    m_Text[TEXT_LEFT_ARROW] = Text_Create(0, 0, "\\{button left}");
    Text_CentreV(m_Text[TEXT_LEFT_ARROW], 1);
    Text_SetPos(
        m_Text[TEXT_LEFT_ARROW], m_GraphicsMenu.value_texts[0]->pos.x - 20,
        m_GraphicsMenu.value_texts[0]->pos.y);
    m_HideArrowLeft =
        g_Config.rendering.enable_perspective_filter ? false : true;

    m_Text[TEXT_RIGHT_ARROW] = Text_Create(0, 0, "\\{button right}");
    Text_CentreV(m_Text[TEXT_RIGHT_ARROW], 1);
    Text_SetPos(
        m_Text[TEXT_RIGHT_ARROW], m_GraphicsMenu.value_texts[0]->pos.x + 40,
        m_GraphicsMenu.value_texts[0]->pos.y);
    m_HideArrowRight =
        g_Config.rendering.enable_perspective_filter ? true : false;

    m_Text[TEXT_UP_ARROW] = Text_Create(0, TOP_Y + BORDER * 2, "\\{arrow up}");
    Text_SetScale(
        m_Text[TEXT_UP_ARROW], TEXT_BASE_SCALE * 2 / 3,
        TEXT_BASE_SCALE * 2 / 3);
    Text_CentreH(m_Text[TEXT_UP_ARROW], true);
    Text_CentreV(m_Text[TEXT_UP_ARROW], true);

    m_Text[TEXT_DOWN_ARROW] = Text_Create(0, max_y, "\\{arrow down}");
    Text_SetScale(
        m_Text[TEXT_DOWN_ARROW], TEXT_BASE_SCALE * 2 / 3,
        TEXT_BASE_SCALE * 2 / 3);
    Text_CentreH(m_Text[TEXT_DOWN_ARROW], true);
    Text_CentreV(m_Text[TEXT_DOWN_ARROW], true);

    const int16_t centre = Screen_GetResWidthDownscaled(RSR_TEXT) / 2;
    m_Text[TEXT_ROW_SELECT] =
        Text_Create(centre - 3, TOP_Y + ROW_HEIGHT + BORDER * 2, " ");
    Text_CentreV(m_Text[TEXT_ROW_SELECT], 1);
    // Text_AddBackground(
    //     m_Text[TEXT_ROW_SELECT], ROW_WIDTH - 7, 0, 0, 0, TS_REQUESTED);
    // Text_AddOutline(m_Text[TEXT_ROW_SELECT], TS_REQUESTED);

    M_UpdateText();
}

static void M_UpdateText(void)
{
    int i;
    const GRAPHICS_OPTION_ROW *row;
    for (i = 0, row = m_GraphicsMenu.first_visible;
         row <= m_GraphicsMenu.last_visible; i++, row++) {
        M_ChangeTextOption(
            row, m_GraphicsMenu.option_texts[i], m_GraphicsMenu.value_texts[i]);
    }
}

static void M_UpdateArrows(
    GRAPHICS_OPTION_NAME option_name, TEXTSTRING value_text, bool more_up,
    bool more_down)
{
    int16_t local_right_arrow_offset = 0;

    switch (option_name) {
    case OPTION_FPS:
        m_HideArrowLeft = g_Config.rendering.fps == 30;
        m_HideArrowRight = g_Config.rendering.fps == 60;
        break;
    case OPTION_TEXTURE_FILTER:
        m_HideArrowLeft = g_Config.rendering.texture_filter == GFX_TF_FIRST;
        m_HideArrowRight = g_Config.rendering.texture_filter == GFX_TF_LAST;
        break;
    case OPTION_FBO_FILTER:
        m_HideArrowLeft = g_Config.rendering.fbo_filter == GFX_TF_FIRST;
        m_HideArrowRight = g_Config.rendering.fbo_filter == GFX_TF_LAST;
        break;
    case OPTION_VSYNC:
        m_HideArrowLeft = !g_Config.rendering.enable_vsync;
        m_HideArrowRight = g_Config.rendering.enable_vsync;
        break;
    case OPTION_BRIGHTNESS:
        m_HideArrowLeft = g_Config.visuals.brightness <= CONFIG_MIN_BRIGHTNESS;
        m_HideArrowRight = g_Config.visuals.brightness >= CONFIG_MAX_BRIGHTNESS;
        break;
    case OPTION_UI_TEXT_SCALE:
        m_HideArrowLeft = g_Config.ui.text_scale <= CONFIG_MIN_TEXT_SCALE;
        m_HideArrowRight = g_Config.ui.text_scale >= CONFIG_MAX_TEXT_SCALE;
        break;
    case OPTION_UI_BAR_SCALE:
        m_HideArrowLeft = g_Config.ui.bar_scale <= CONFIG_MIN_BAR_SCALE;
        m_HideArrowRight = g_Config.ui.bar_scale >= CONFIG_MAX_BAR_SCALE;
        break;
    case OPTION_RENDER_MODE:
        local_right_arrow_offset = RIGHT_ARROW_OFFSET_MAX;
        m_HideArrowLeft = false;
        m_HideArrowRight = false;
        break;
    case OPTION_RESOLUTION:
        local_right_arrow_offset = RIGHT_ARROW_OFFSET_MAX;
        m_HideArrowLeft = !Screen_CanSetPrevRes();
        m_HideArrowRight = !Screen_CanSetNextRes();
        break;
    case OPTION_PERSPECTIVE:
        m_HideArrowLeft = !g_Config.rendering.enable_perspective_filter;
        m_HideArrowRight = g_Config.rendering.enable_perspective_filter;
        break;
    case OPTION_PRETTY_PIXELS:
        m_HideArrowLeft = !g_Config.rendering.pretty_pixels;
        m_HideArrowRight = g_Config.rendering.pretty_pixels;
        break;
    case OPTION_REFLECTIONS:
        m_HideArrowLeft = !g_Config.visuals.enable_reflections;
        m_HideArrowRight = g_Config.visuals.enable_reflections;
        break;

    case OPTION_NUMBER_OF:
    default:
        break;
    }

    Text_SetPos(
        m_Text[TEXT_LEFT_ARROW], value_text.pos.x + LEFT_ARROW_OFFSET,
        value_text.pos.y);
    Text_SetPos(
        m_Text[TEXT_RIGHT_ARROW],
        value_text.pos.x + RIGHT_ARROW_OFFSET_MIN + local_right_arrow_offset,
        value_text.pos.y);

    Text_Hide(m_Text[TEXT_LEFT_ARROW], m_HideArrowLeft);
    Text_Hide(m_Text[TEXT_RIGHT_ARROW], m_HideArrowRight);

    if (more_up) {
        Text_Hide(m_Text[TEXT_UP_ARROW], false);
    } else {
        Text_Hide(m_Text[TEXT_UP_ARROW], true);
    }

    if (more_down) {
        Text_Hide(m_Text[TEXT_DOWN_ARROW], false);
    } else {
        Text_Hide(m_Text[TEXT_DOWN_ARROW], true);
    }
}

static int16_t M_PlaceColumns(bool create)
{
    const int16_t centre = Screen_GetResWidthDownscaled(RSR_TEXT) / 2;
    const GRAPHICS_OPTION_ROW *row = m_GraphicsMenu.first_visible;

    int16_t name_x = centre + COL_OFFSET_0;
    int16_t name_y = TOP_Y + ROW_HEIGHT + BORDER * 2;
    int16_t value_x = centre + COL_OFFSET_1;
    int16_t value_y = TOP_Y + ROW_HEIGHT + BORDER * 2;

    for (int i = 0; i < m_GraphicsMenu.num_vis_options
         && row->option_name != OPTION_NUMBER_OF;
         i++, row++) {
        if (create) {
            m_GraphicsMenu.option_texts[i] =
                Text_Create(name_x, name_y, GameString_Get(row->option_string));

            m_GraphicsMenu.value_texts[i] = Text_Create(
                value_x, value_y, GameString_Get(row->value_string));
        } else {
            Text_SetPos(m_GraphicsMenu.option_texts[i], name_x, name_y);
            Text_SetPos(m_GraphicsMenu.value_texts[i], value_x, value_y);
        }

        Text_CentreV(m_GraphicsMenu.option_texts[i], true);
        Text_CentreV(m_GraphicsMenu.value_texts[i], true);

        name_y += ROW_HEIGHT;
        value_y += ROW_HEIGHT;
    }

    return MAX(name_y, value_y);
}

static void M_ChangeTextOption(
    const GRAPHICS_OPTION_ROW *row, TEXTSTRING *option_text,
    TEXTSTRING *value_text)
{
    char buf[OPTION_LENGTH];

    Text_ChangeText(option_text, GameString_Get(row->option_string));

    switch (row->option_name) {
    case OPTION_FPS:
        sprintf(buf, GS(DETAIL_DECIMAL_FMT), g_Config.rendering.fps);
        Text_ChangeText(value_text, buf);
        break;

    case OPTION_TEXTURE_FILTER: {
        bool is_enabled = g_Config.rendering.texture_filter == GFX_TF_BILINEAR;
        Text_ChangeText(
            value_text, is_enabled ? GS(DETAIL_BILINEAR) : GS(MISC_OFF));
        break;
    }

    case OPTION_FBO_FILTER: {
        bool is_enabled = g_Config.rendering.fbo_filter == GFX_TF_BILINEAR;
        Text_ChangeText(
            value_text, is_enabled ? GS(DETAIL_BILINEAR) : GS(MISC_OFF));
        break;
    }

    case OPTION_VSYNC: {
        bool is_enabled = g_Config.rendering.enable_vsync;
        Text_ChangeText(value_text, is_enabled ? GS(MISC_ON) : GS(MISC_OFF));
        break;
    }

    case OPTION_BRIGHTNESS:
        sprintf(buf, GS(DETAIL_FLOAT_FMT), g_Config.visuals.brightness);
        Text_ChangeText(value_text, buf);
        break;

    case OPTION_UI_TEXT_SCALE:
        sprintf(buf, GS(DETAIL_FLOAT_FMT), g_Config.ui.text_scale);
        Text_ChangeText(value_text, buf);
        // Text_SetPos(
        //     m_Text[TEXT_ROW_SELECT],
        //     Screen_GetResWidthDownscaled(RSR_TEXT) / 2 - 3,
        //     TOP_Y + ROW_HEIGHT + BORDER * 2);
        break;

    case OPTION_UI_BAR_SCALE:
        sprintf(buf, GS(DETAIL_FLOAT_FMT), g_Config.ui.bar_scale);
        Text_ChangeText(value_text, buf);
        break;

    case OPTION_RENDER_MODE:
        sprintf(
            buf, GS(DETAIL_STRING_FMT),
            (g_Config.rendering.render_mode == GFX_RM_FRAMEBUFFER
                 ? GS(DETAIL_RENDER_MODE_FBO)
                 : GS(DETAIL_RENDER_MODE_LEGACY)));
        Text_ChangeText(value_text, buf);
        break;

    case OPTION_RESOLUTION:
        sprintf(
            buf, GS(DETAIL_RESOLUTION_FMT), Screen_GetResWidth(),
            Screen_GetResHeight());
        Text_ChangeText(value_text, buf);
        break;

    case OPTION_PERSPECTIVE: {
        bool is_enabled = g_Config.rendering.enable_perspective_filter;
        Text_ChangeText(value_text, is_enabled ? GS(MISC_ON) : GS(MISC_OFF));
        break;
    }

    case OPTION_PRETTY_PIXELS: {
        bool is_enabled = g_Config.rendering.pretty_pixels;
        Text_ChangeText(value_text, is_enabled ? GS(MISC_ON) : GS(MISC_OFF));
        break;
    }

    case OPTION_REFLECTIONS: {
        bool is_enabled = g_Config.visuals.enable_reflections;
        Text_ChangeText(value_text, is_enabled ? GS(MISC_ON) : GS(MISC_OFF));
        break;
    }

    case OPTION_NUMBER_OF:
    default:
        break;
    }
}

void Option_Graphics_Control(INVENTORY_ITEM *inv_item, const bool is_busy)
{
    if (is_busy) {
        return;
    }

    if (!m_IsTextInit) {
        M_InitText();
        m_IsTextInit = true;
    }

    if (g_InputDB.menu_up) {
        M_MenuUp();
    }

    if (g_InputDB.menu_down) {
        M_MenuDown();
    }

    Text_AddBackground(
        m_Text[TEXT_ROW_SELECT], ROW_WIDTH - 7, 0, 0,
        ROW_HEIGHT * (m_GraphicsMenu.cur_option - m_GraphicsMenu.first_visible),
        TS_REQUESTED);
    Text_AddOutline(m_Text[TEXT_ROW_SELECT], TS_REQUESTED);

    int32_t reset = -1;

    if (g_InputDB.menu_right) {
        switch (m_GraphicsMenu.cur_option->option_name) {
        case OPTION_FPS:
            g_Config.rendering.fps = 60;
            reset = OPTION_FPS;
            break;

        case OPTION_TEXTURE_FILTER:
            if (g_Config.rendering.texture_filter != GFX_TF_LAST) {
                g_Config.rendering.texture_filter++;
                reset = OPTION_TEXTURE_FILTER;
            }
            break;

        case OPTION_FBO_FILTER:
            if (g_Config.rendering.fbo_filter != GFX_TF_LAST) {
                g_Config.rendering.fbo_filter++;
                reset = OPTION_FBO_FILTER;
            }
            break;

        case OPTION_VSYNC:
            if (!g_Config.rendering.enable_vsync) {
                g_Config.rendering.enable_vsync = true;
                GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
                reset = OPTION_VSYNC;
            }
            break;

        case OPTION_BRIGHTNESS:
            g_Config.visuals.brightness += 0.1f;
            CLAMP(
                g_Config.visuals.brightness, CONFIG_MIN_BRIGHTNESS,
                CONFIG_MAX_BRIGHTNESS);
            reset = OPTION_BRIGHTNESS;
            break;

        case OPTION_UI_TEXT_SCALE:
            g_Config.ui.text_scale += 0.1;
            CLAMP(
                g_Config.ui.text_scale, CONFIG_MIN_TEXT_SCALE,
                CONFIG_MAX_TEXT_SCALE);
            M_Reinitialize(OPTION_UI_TEXT_SCALE);
            reset = OPTION_UI_TEXT_SCALE;
            break;

        case OPTION_UI_BAR_SCALE:
            g_Config.ui.bar_scale += 0.1;
            CLAMP(
                g_Config.ui.bar_scale, CONFIG_MIN_BAR_SCALE,
                CONFIG_MAX_BAR_SCALE);
            reset = OPTION_UI_BAR_SCALE;
            break;

        case OPTION_RENDER_MODE:
            if (g_Config.rendering.render_mode == GFX_RM_LEGACY) {
                g_Config.rendering.render_mode = GFX_RM_FRAMEBUFFER;
            } else {
                g_Config.rendering.render_mode = GFX_RM_LEGACY;
            }
            reset = OPTION_RENDER_MODE;
            break;

        case OPTION_RESOLUTION:
            if (Screen_SetNextRes()) {
                g_Config.rendering.resolution_width = Screen_GetResWidth();
                g_Config.rendering.resolution_height = Screen_GetResHeight();
                M_Reinitialize(OPTION_RESOLUTION);
                reset = OPTION_RESOLUTION;
            }
            break;

        case OPTION_PERSPECTIVE:
            if (!g_Config.rendering.enable_perspective_filter) {
                g_Config.rendering.enable_perspective_filter = true;
                reset = OPTION_PERSPECTIVE;
            }
            break;

        case OPTION_PRETTY_PIXELS:
            if (!g_Config.rendering.pretty_pixels) {
                g_Config.rendering.pretty_pixels = true;
                reset = OPTION_PRETTY_PIXELS;
            }
            break;

        case OPTION_REFLECTIONS:
            if (!g_Config.visuals.enable_reflections) {
                g_Config.visuals.enable_reflections = true;
                reset = OPTION_REFLECTIONS;
            }
            break;

        case OPTION_NUMBER_OF:
        default:
            break;
        }
    }

    if (g_InputDB.menu_left) {
        switch (m_GraphicsMenu.cur_option->option_name) {
        case OPTION_FPS:
            g_Config.rendering.fps = 30;
            reset = OPTION_FPS;
            break;

        case OPTION_TEXTURE_FILTER:
            if (g_Config.rendering.texture_filter != GFX_TF_FIRST) {
                g_Config.rendering.texture_filter--;
                reset = OPTION_TEXTURE_FILTER;
            }
            break;

        case OPTION_FBO_FILTER:
            if (g_Config.rendering.fbo_filter != GFX_TF_FIRST) {
                g_Config.rendering.fbo_filter--;
                reset = OPTION_FBO_FILTER;
            }
            break;

        case OPTION_VSYNC:
            if (g_Config.rendering.enable_vsync) {
                g_Config.rendering.enable_vsync = false;
                GFX_Context_SetVSync(g_Config.rendering.enable_vsync);
                reset = OPTION_VSYNC;
            }
            break;

        case OPTION_BRIGHTNESS:
            g_Config.visuals.brightness -= 0.1f;
            CLAMP(
                g_Config.visuals.brightness, CONFIG_MIN_BRIGHTNESS,
                CONFIG_MAX_BRIGHTNESS);
            reset = OPTION_BRIGHTNESS;
            break;

        case OPTION_UI_TEXT_SCALE:
            g_Config.ui.text_scale -= 0.1;
            CLAMP(
                g_Config.ui.text_scale, CONFIG_MIN_TEXT_SCALE,
                CONFIG_MAX_TEXT_SCALE);
            M_Reinitialize(OPTION_UI_TEXT_SCALE);
            reset = OPTION_UI_TEXT_SCALE;
            break;

        case OPTION_UI_BAR_SCALE:
            g_Config.ui.bar_scale -= 0.1;
            CLAMP(
                g_Config.ui.bar_scale, CONFIG_MIN_BAR_SCALE,
                CONFIG_MAX_BAR_SCALE);
            reset = OPTION_UI_BAR_SCALE;
            break;

        case OPTION_RENDER_MODE:
            if (g_Config.rendering.render_mode == GFX_RM_LEGACY) {
                g_Config.rendering.render_mode = GFX_RM_FRAMEBUFFER;
            } else {
                g_Config.rendering.render_mode = GFX_RM_LEGACY;
            }
            reset = OPTION_RENDER_MODE;
            break;

        case OPTION_RESOLUTION:
            if (Screen_SetPrevRes()) {
                reset = OPTION_RESOLUTION;
                g_Config.rendering.resolution_width = Screen_GetResWidth();
                g_Config.rendering.resolution_height = Screen_GetResHeight();
                M_Reinitialize(OPTION_RESOLUTION);
            }
            break;

        case OPTION_PERSPECTIVE:
            if (g_Config.rendering.enable_perspective_filter) {
                g_Config.rendering.enable_perspective_filter = false;
                reset = OPTION_PERSPECTIVE;
            }
            break;

        case OPTION_PRETTY_PIXELS:
            if (g_Config.rendering.pretty_pixels) {
                g_Config.rendering.pretty_pixels = false;
                reset = OPTION_PRETTY_PIXELS;
            }
            break;

        case OPTION_REFLECTIONS:
            if (g_Config.visuals.enable_reflections) {
                g_Config.visuals.enable_reflections = false;
                reset = OPTION_REFLECTIONS;
            }
            break;

        case OPTION_NUMBER_OF:
        default:
            break;
        }
    }

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        reset = OPTION_NUMBER_OF;
    }

    if (reset > -1) {
        M_ChangeTextOption(
            m_GraphicsMenu.cur_option,
            m_GraphicsMenu.option_texts[m_GraphicsMenu.row_num],
            m_GraphicsMenu.value_texts[m_GraphicsMenu.row_num]);
        Config_Write();
    }

    M_UpdateArrows(
        m_GraphicsMenu.cur_option->option_name,
        *m_GraphicsMenu.value_texts[m_GraphicsMenu.row_num],
        m_GraphicsMenu.first_visible != m_GraphicsMenu.first_option,
        m_GraphicsMenu.last_visible != m_GraphicsMenu.last_option);

    if (reset == OPTION_NUMBER_OF) {
        Option_Graphics_Shutdown();
        Config_Write();
    }
}

void Option_Graphics_Shutdown(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = nullptr;
    }
    for (int i = 0; i < OPTION_NUMBER_OF; i++) {
        Text_Remove(m_GraphicsMenu.option_texts[i]);
        m_GraphicsMenu.option_texts[i] = nullptr;
        Text_Remove(m_GraphicsMenu.value_texts[i]);
        m_GraphicsMenu.value_texts[i] = nullptr;
    }
    m_IsTextInit = false;
    m_HideArrowLeft = false;
    m_HideArrowRight = false;
    M_InitMenu();
}
