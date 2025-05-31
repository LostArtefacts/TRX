// UI dialog for adjusting music and sound volumes
#pragma once

#include "../common.h"
#include "../dialogs/settings.h"

typedef UI_SETTINGS_STATE UI_SOUND_SETTINGS_STATE;

// Initialize the sound settings dialog state.
UI_SOUND_SETTINGS_STATE *UI_SoundSettings_Init(void);

// Free resources used by the sound settings dialog.
void UI_SoundSettings_Free(UI_SOUND_SETTINGS_STATE *s);

// Handle input/control for the sound settings dialog.
// Returns true if the dialog should be closed.
bool UI_SoundSettings_Control(UI_SOUND_SETTINGS_STATE *s);

// Render the sound settings dialog.
void UI_SoundSettings(UI_SOUND_SETTINGS_STATE *s);
