#include "game/ui/dialogs/sound_settings.h"

#include "config.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/music.h"
#include "game/sound.h"
#include "game/ui/elements/anchor.h"
#include "game/ui/elements/hide.h"
#include "game/ui/elements/label.h"
#include "game/ui/elements/modal.h"
#include "game/ui/elements/requester.h"
#include "game/ui/elements/resize.h"
#include "game/ui/elements/spacer.h"
#include "game/ui/elements/stack.h"
#include "memory.h"
#include "strings.h"
#include "utils.h"

typedef struct UI_SOUND_SETTINGS_STATE {
    UI_REQUESTER_STATE req;
} UI_SOUND_SETTINGS_STATE;

typedef enum {
    M_ROW_MUSIC = 0,
    M_ROW_SOUND = 1,
    M_ROW_COUNT = 2,
} M_ROW;

static const GAME_STRING_ID m_Labels[M_ROW_COUNT] = {
    GS_ID(SOUND_DIALOG_SOUND),
    GS_ID(SOUND_DIALOG_MUSIC),
};

static char *M_FormatRowValue(int32_t row);
static bool M_CanChange(int32_t row, int32_t dir);
static bool M_RequestChange(int32_t row, int32_t dir);

static char *M_FormatRowValue(const int32_t row)
{
    switch (row) {
    case M_ROW_MUSIC:
        return String_Format("%2d", g_Config.audio.music_volume);
    case M_ROW_SOUND:
        return String_Format("%2d", g_Config.audio.sound_volume);
    default:
        return nullptr;
    }
}

static bool M_CanChange(const int32_t row, const int32_t dir)
{
    switch (row) {
    case M_ROW_MUSIC:
        if (dir < 0) {
            return g_Config.audio.music_volume > Music_GetMinVolume();
        } else if (dir > 0) {
            return g_Config.audio.music_volume < Music_GetMaxVolume();
        }
        break;
    case M_ROW_SOUND:
        if (dir < 0) {
            return g_Config.audio.sound_volume > Sound_GetMinVolume();
        } else if (dir > 0) {
            return g_Config.audio.sound_volume < Sound_GetMaxVolume();
        }
        break;
    }
    return false;
}

static bool M_RequestChange(const int32_t row, const int32_t dir)
{
    if (!M_CanChange(row, dir)) {
        return false;
    }
    switch (row) {
    case M_ROW_MUSIC:
        g_Config.audio.music_volume += dir;
        Music_SetVolume(g_Config.audio.music_volume);
        break;
    case M_ROW_SOUND:
        g_Config.audio.sound_volume += dir;
        Sound_SetMasterVolume(g_Config.audio.sound_volume);
        break;
    default:
        return false;
    }
    Config_Write();
    Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
    return true;
}

UI_SOUND_SETTINGS_STATE *UI_SoundSettings_Init(void)
{
    UI_SOUND_SETTINGS_STATE *s = Memory_Alloc(sizeof(UI_SOUND_SETTINGS_STATE));
    UI_Requester_Init(&s->req, M_ROW_COUNT, M_ROW_COUNT, true);
    s->req.row_pad = 2.0f;
    return s;
}

void UI_SoundSettings_Free(UI_SOUND_SETTINGS_STATE *const s)
{
    UI_Requester_Free(&s->req);
    Memory_Free(s);
}

bool UI_SoundSettings_Control(UI_SOUND_SETTINGS_STATE *const s)
{
    const int32_t choice = UI_Requester_Control(&s->req);
    if (choice == UI_REQUESTER_CANCEL) {
        return true;
    }
    const int32_t sel = UI_Requester_GetCurrentRow(&s->req);
    if (g_InputDB.menu_left && sel >= 0) {
        M_RequestChange(sel, -1);
    } else if (g_InputDB.menu_right && sel >= 0) {
        M_RequestChange(sel, +1);
    }
    return false;
}

void UI_SoundSettings(UI_SOUND_SETTINGS_STATE *const s)
{
    const int32_t sel = UI_Requester_GetCurrentRow(&s->req);
    UI_BeginModal(0.5f, 0.6f);
    UI_BeginRequester(&s->req, GS(SOUND_DIALOG_TITLE));

    // Measure the maximum width of the value label to prevent the entire
    // dialog from changing its size as the player changes the sound levels.
    float value_w = -1.0f;
    UI_Label_Measure("10", &value_w, nullptr);

    for (int32_t i = 0; i < s->req.scroll.max_items; ++i) {
        if (!UI_Requester_IsRowVisible(&s->req, i)) {
            UI_BeginResize(-1.0f, 0.0f);
        } else {
            UI_BeginResize(-1.0f, -1.0f);
        }
        UI_BeginRequesterRow(&s->req, i);

        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
        });
        UI_Label(GameString_Get(m_Labels[i]));
        UI_Spacer(20.0f, 0.0f);

        UI_BeginStackEx((UI_STACK_SETTINGS) {
            .orientation = UI_STACK_HORIZONTAL,
            .align = { .h = UI_STACK_H_ALIGN_DISTRIBUTE },
            .spacing = { .h = 5.0f },
        });
        UI_BeginHide(i != sel || !M_CanChange(i, -1));
        UI_Label("\\{button left}");
        UI_EndHide();

        UI_BeginResize(value_w, -1.0f);
        UI_BeginAnchor(0.5f, 0.5f);
        UI_Label(M_FormatRowValue(i));
        UI_EndAnchor();
        UI_EndResize();

        UI_BeginHide(i != sel || !M_CanChange(i, +1));
        UI_Label("\\{button right}");
        UI_EndHide();
        UI_EndStack();
        UI_EndStack();

        UI_EndRequesterRow(&s->req, i);
        UI_EndResize();
    }

    UI_EndRequester(&s->req);
    UI_EndModal();
}
