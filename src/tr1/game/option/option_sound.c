#include "game/option/option_sound.h"

#include "config.h"
#include "game/game_string.h"
#include "game/input.h"
#include "game/music.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"

#include <stdbool.h>
#include <stdio.h>

typedef enum {
    TEXT_MUSIC_VOLUME = 0,
    TEXT_SOUND_VOLUME = 1,
    TEXT_TITLE = 2,
    TEXT_TITLE_BORDER = 3,
    TEXT_LEFT_ARROW = 4,
    TEXT_RIGHT_ARROW = 5,
    TEXT_NUMBER_OF = 6,
    TEXT_OPTION_MIN = TEXT_MUSIC_VOLUME,
    TEXT_OPTION_MAX = TEXT_SOUND_VOLUME,
} SOUND_TEXT;

static TEXTSTRING *m_Text[TEXT_NUMBER_OF] = { 0 };

static void M_InitText(void);

static void M_InitText(void)
{
    char buf[20];

    m_Text[TEXT_LEFT_ARROW] = Text_Create(-45, 0, "\200");
    m_Text[TEXT_RIGHT_ARROW] = Text_Create(40, 0, "\201");

    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, -32, " ");
    m_Text[TEXT_TITLE] = Text_Create(0, -30, GS(SOUND_SET_VOLUMES));

    if (g_Config.music_volume > 10) {
        g_Config.music_volume = 10;
    }
    sprintf(buf, "| %2d", g_Config.music_volume);
    m_Text[TEXT_MUSIC_VOLUME] = Text_Create(0, 0, buf);

    if (g_Config.sound_volume > 10) {
        g_Config.sound_volume = 10;
    }
    sprintf(buf, "} %2d", g_Config.sound_volume);
    m_Text[TEXT_SOUND_VOLUME] = Text_Create(0, 25, buf);

    Text_AddBackground(m_Text[g_OptionSelected], 128, 0, 0, 0, TS_REQUESTED);
    Text_AddOutline(m_Text[g_OptionSelected], true, TS_REQUESTED);
    Text_AddBackground(m_Text[TEXT_TITLE], 136, 0, 0, 0, TS_HEADING);
    Text_AddOutline(m_Text[TEXT_TITLE], true, TS_HEADING);
    Text_AddBackground(m_Text[TEXT_TITLE_BORDER], 140, 85, 0, 0, TS_BACKGROUND);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], true, TS_BACKGROUND);

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_CentreH(m_Text[i], 1);
        Text_CentreV(m_Text[i], 1);
    }
}

void Option_Sound(INVENTORY_ITEM *inv_item)
{
    char buf[20];

    if (!m_Text[TEXT_MUSIC_VOLUME]) {
        M_InitText();
    }

    if (g_InputDB.menu_up && g_OptionSelected > TEXT_OPTION_MIN) {
        Text_RemoveOutline(m_Text[g_OptionSelected]);
        Text_RemoveBackground(m_Text[g_OptionSelected]);
        --g_OptionSelected;
        Text_AddBackground(
            m_Text[g_OptionSelected], 128, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(m_Text[g_OptionSelected], true, TS_REQUESTED);
        Text_SetPos(m_Text[TEXT_LEFT_ARROW], -45, 0);
        Text_SetPos(m_Text[TEXT_RIGHT_ARROW], 40, 0);
    }

    if (g_InputDB.menu_down && g_OptionSelected < TEXT_OPTION_MAX) {
        Text_RemoveOutline(m_Text[g_OptionSelected]);
        Text_RemoveBackground(m_Text[g_OptionSelected]);
        ++g_OptionSelected;
        Text_AddBackground(
            m_Text[g_OptionSelected], 128, 0, 0, 0, TS_REQUESTED);
        Text_AddOutline(m_Text[g_OptionSelected], true, TS_REQUESTED);
        Text_SetPos(m_Text[TEXT_LEFT_ARROW], -45, 25);
        Text_SetPos(m_Text[TEXT_RIGHT_ARROW], 40, 25);
    }

    switch (g_OptionSelected) {
    case TEXT_MUSIC_VOLUME:
        if (g_Input.menu_left && g_Config.music_volume > Music_GetMinVolume()) {
            g_Config.music_volume--;
            Config_Write();
            g_IDelay = true;
            g_IDCount = 5;
            Music_SetVolume(g_Config.music_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "| %2d", g_Config.music_volume);
            Text_ChangeText(m_Text[TEXT_MUSIC_VOLUME], buf);
        } else if (
            g_Input.menu_right
            && g_Config.music_volume < Music_GetMaxVolume()) {
            g_Config.music_volume++;
            Config_Write();
            g_IDelay = true;
            g_IDCount = 5;
            Music_SetVolume(g_Config.music_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "| %2d", g_Config.music_volume);
            Text_ChangeText(m_Text[TEXT_MUSIC_VOLUME], buf);
        }

        if (g_Config.music_volume == Music_GetMinVolume()) {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        } else if (g_Config.music_volume == Music_GetMaxVolume()) {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        } else {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
        }

        break;

    case TEXT_SOUND_VOLUME:
        if (g_Input.menu_left && g_Config.sound_volume > Sound_GetMinVolume()) {
            g_Config.sound_volume--;
            Config_Write();
            g_IDelay = true;
            g_IDCount = 5;
            Sound_SetMasterVolume(g_Config.sound_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "} %2d", g_Config.sound_volume);
            Text_ChangeText(m_Text[TEXT_SOUND_VOLUME], buf);
        } else if (
            g_Input.menu_right
            && g_Config.sound_volume < Sound_GetMaxVolume()) {
            g_Config.sound_volume++;
            Config_Write();
            g_IDelay = true;
            g_IDCount = 5;
            Sound_SetMasterVolume(g_Config.sound_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "} %2d", g_Config.sound_volume);
            Text_ChangeText(m_Text[TEXT_SOUND_VOLUME], buf);
        }

        if (g_Config.sound_volume == Sound_GetMinVolume()) {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        } else if (g_Config.sound_volume == Sound_GetMaxVolume()) {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        } else {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
        }

        break;
    }

    if (g_InputDB.menu_confirm || g_InputDB.menu_back) {
        Option_Sound_Shutdown();
    }
}

void Option_Sound_Shutdown(void)
{
    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_Remove(m_Text[i]);
        m_Text[i] = NULL;
    }
}
