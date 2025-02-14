#include "game/game_string.h"
#include "game/input.h"
#include "game/inventory_ring.h"
#include "game/music.h"
#include "game/option/option.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/utils.h>

#include <stdio.h>

static TEXTSTRING *m_SoundText[4];

static void M_InitText(void);
static void M_ShutdownText(void);

static void M_InitText(void)
{
    CLAMPG(g_Config.audio.music_volume, 10);
    CLAMPG(g_Config.audio.sound_volume, 10);

    char text[32];
    sprintf(text, "\\{icon music} %2d", g_Config.audio.music_volume);
    m_SoundText[0] = Text_Create(0, 0, text);
    Text_AddBackground(m_SoundText[0], 128, 0, 0, 0, TS_REQUESTED);
    Text_AddOutline(m_SoundText[0], TS_REQUESTED);

    sprintf(text, "\\{icon sound} %2d", g_Config.audio.sound_volume);
    m_SoundText[1] = Text_Create(0, 25, text);

    m_SoundText[2] = Text_Create(0, -32, " ");
    Text_AddBackground(m_SoundText[2], 140, 85, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_SoundText[2], TS_BACKGROUND);

    m_SoundText[3] = Text_Create(0, -30, GS(SOUND_SET_VOLUMES));
    Text_AddBackground(m_SoundText[3], 136, 0, 0, 0, TS_HEADING);
    Text_AddOutline(m_SoundText[3], TS_HEADING);

    for (int32_t i = 0; i < 4; i++) {
        Text_CentreH(m_SoundText[i], true);
        Text_CentreV(m_SoundText[i], true);
    }
}

static void M_ShutdownText(void)
{
    for (int32_t i = 0; i < 4; i++) {
        Text_Remove(m_SoundText[i]);
        m_SoundText[i] = nullptr;
    }
}

void Option_Sound_Shutdown(void)
{
    M_ShutdownText();
}

void Option_Sound_Control(INVENTORY_ITEM *const item, const bool is_busy)
{
    if (is_busy) {
        return;
    }

    char text[32];

    if (m_SoundText[0] == nullptr) {
        M_InitText();
    }

    if (g_InputDB.menu_up && g_SoundOptionLine > 0) {
        Text_RemoveOutline(m_SoundText[g_SoundOptionLine]);
        Text_RemoveBackground(m_SoundText[g_SoundOptionLine]);
        g_SoundOptionLine--;
        Text_AddBackground(
            m_SoundText[g_SoundOptionLine], 128, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(m_SoundText[g_SoundOptionLine], TS_REQUESTED);
    }

    if (g_InputDB.menu_down && g_SoundOptionLine < 1) {
        Text_RemoveOutline(m_SoundText[g_SoundOptionLine]);
        Text_RemoveBackground(m_SoundText[g_SoundOptionLine]);
        g_SoundOptionLine++;
        Text_AddBackground(
            m_SoundText[g_SoundOptionLine], 128, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(m_SoundText[g_SoundOptionLine], TS_REQUESTED);
    }

    if (g_SoundOptionLine) {
        bool changed = false;
        if (g_InputDB.menu_left && g_Config.audio.sound_volume > 0) {
            g_Config.audio.sound_volume--;
            changed = true;
        } else if (g_InputDB.menu_right && g_Config.audio.sound_volume < 10) {
            g_Config.audio.sound_volume++;
            changed = true;
        }

        if (changed) {
            sprintf(text, "\\{icon sound} %2d", g_Config.audio.sound_volume);
            Text_ChangeText(m_SoundText[1], text);
            Sound_SetMasterVolume(g_Config.audio.sound_volume);
            Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
        }
    } else {
        bool changed = false;
        if (g_InputDB.menu_left && g_Config.audio.music_volume > 0) {
            g_Config.audio.music_volume--;
            changed = true;
        } else if (g_InputDB.menu_right && g_Config.audio.music_volume < 10) {
            g_Config.audio.music_volume++;
            changed = true;
        }

        if (changed) {
            sprintf(text, "\\{icon music} %2d", g_Config.audio.music_volume);
            Text_ChangeText(m_SoundText[0], text);
            Music_SetVolume(g_Config.audio.music_volume);
            Sound_Effect(SFX_MENU_PASSPORT, nullptr, SPM_ALWAYS);
        }
    }

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        Option_Sound_Shutdown();
    }
}

void Option_Sound_Draw(INVENTORY_ITEM *const item)
{
}
