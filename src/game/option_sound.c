#include "game/option.h"

#include "config.h"
#include "game/gameflow.h"
#include "game/input.h"
#include "game/music.h"
#include "game/settings.h"
#include "game/sound.h"
#include "game/text.h"
#include "global/vars.h"

#include <stdio.h>

#define MIN_VOLUME 0
#define MAX_VOLUME 10

typedef enum SOUND_TEXT {
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
static RGBA8888 m_CenterColor = { 70, 30, 107, 255 };
static RGBA8888 m_EdgeColor = { 26, 10, 20, 155 };

static void Option_SoundInitText(void);

static void Option_SoundInitText(void)
{
    char buf[20];

    m_Text[TEXT_LEFT_ARROW] = Text_Create(-45, 0, "\200");
    m_Text[TEXT_RIGHT_ARROW] = Text_Create(40, 0, "\201");

    m_Text[TEXT_TITLE] =
        Text_Create(0, -30, g_GameFlow.strings[GS_SOUND_SET_VOLUMES]);
    m_Text[TEXT_TITLE_BORDER] = Text_Create(0, -32, " ");

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

    Text_AddBackground(m_Text[g_OptionSelected], 128, 0, 0, 0);
    Text_AddOutline(m_Text[g_OptionSelected], 1);
    Text_CentreVGradient(m_Text[g_OptionSelected], m_CenterColor, m_EdgeColor);
    Text_AddBackground(m_Text[TEXT_TITLE], 136, 0, 0, 0);
    Text_AddOutline(m_Text[TEXT_TITLE], 1);
    Text_AddBackground(m_Text[TEXT_TITLE_BORDER], 140, 85, 0, 0);
    Text_AddOutline(m_Text[TEXT_TITLE_BORDER], 1);

    for (int i = 0; i < TEXT_NUMBER_OF; i++) {
        Text_CentreH(m_Text[i], 1);
        Text_CentreV(m_Text[i], 1);
    }
}

void Option_Sound(INVENTORY_ITEM *inv_item)
{
    char buf[20];

    if (!m_Text[TEXT_MUSIC_VOLUME]) {
        Option_SoundInitText();
    }

    if (g_InputDB.forward && g_OptionSelected > TEXT_OPTION_MIN) {
        Text_RemoveOutline(m_Text[g_OptionSelected]);
        Text_RemoveBackground(m_Text[g_OptionSelected]);
        Text_AddBackground(m_Text[--g_OptionSelected], 128, 0, 0, 0);
        Text_AddOutline(m_Text[g_OptionSelected], 1);
        Text_CentreVGradient(
            m_Text[g_OptionSelected], m_CenterColor, m_EdgeColor);
        Text_SetPos(m_Text[TEXT_LEFT_ARROW], -45, 0);
        Text_SetPos(m_Text[TEXT_RIGHT_ARROW], 40, 0);
    }

    if (g_InputDB.back && g_OptionSelected < TEXT_OPTION_MAX) {
        Text_RemoveOutline(m_Text[g_OptionSelected]);
        Text_RemoveBackground(m_Text[g_OptionSelected]);
        Text_AddBackground(m_Text[++g_OptionSelected], 128, 0, 0, 0);
        Text_AddOutline(m_Text[g_OptionSelected], 1);
        Text_CentreVGradient(
            m_Text[g_OptionSelected], m_CenterColor, m_EdgeColor);
        Text_SetPos(m_Text[TEXT_LEFT_ARROW], -45, 25);
        Text_SetPos(m_Text[TEXT_RIGHT_ARROW], 40, 25);
    }

    switch (g_OptionSelected) {
    case TEXT_MUSIC_VOLUME:
        if (g_Input.left && g_Config.music_volume > MIN_VOLUME) {
            g_Config.music_volume--;
            g_IDelay = true;
            g_IDCount = 10;
            Music_SetVolume(g_Config.music_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "| %2d", g_Config.music_volume);
            Text_ChangeText(m_Text[TEXT_MUSIC_VOLUME], buf);
            Settings_Write();
        } else if (g_Input.right && g_Config.music_volume < MAX_VOLUME) {
            g_Config.music_volume++;
            g_IDelay = true;
            g_IDCount = 10;
            Music_SetVolume(g_Config.music_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "| %2d", g_Config.music_volume);
            Text_ChangeText(m_Text[TEXT_MUSIC_VOLUME], buf);
            Settings_Write();
        }

        if (g_Config.music_volume == MIN_VOLUME) {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        } else if (g_Config.music_volume == MAX_VOLUME) {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        } else {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
        }

        break;

    case TEXT_SOUND_VOLUME:
        if (g_Input.left && g_Config.sound_volume > MIN_VOLUME) {
            g_Config.sound_volume--;
            g_IDelay = true;
            g_IDCount = 10;
            Sound_SetMasterVolume(g_Config.sound_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "} %2d", g_Config.sound_volume);
            Text_ChangeText(m_Text[TEXT_SOUND_VOLUME], buf);
            Settings_Write();
        } else if (g_Input.right && g_Config.sound_volume < MAX_VOLUME) {
            g_Config.sound_volume++;
            g_IDelay = true;
            g_IDCount = 10;
            Sound_SetMasterVolume(g_Config.sound_volume);
            Sound_Effect(SFX_MENU_PASSPORT, NULL, SPM_ALWAYS);
            sprintf(buf, "} %2d", g_Config.sound_volume);
            Text_ChangeText(m_Text[TEXT_SOUND_VOLUME], buf);
            Settings_Write();
        }

        if (g_Config.sound_volume == MIN_VOLUME) {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], true);
        } else if (g_Config.sound_volume == MAX_VOLUME) {
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], true);
        } else {
            Text_Hide(m_Text[TEXT_LEFT_ARROW], false);
            Text_Hide(m_Text[TEXT_RIGHT_ARROW], false);
        }

        break;
    }

    if (g_InputDB.deselect || g_InputDB.select) {
        for (int i = 0; i < TEXT_NUMBER_OF; i++) {
            Text_Remove(m_Text[i]);
            m_Text[i] = NULL;
        }
    }
}
