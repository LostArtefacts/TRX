#pragma once

#include "../common.h"
#include "./settings.h"

UI_SETTINGS_STATE *UI_GameplaySettings_Init(void);
void UI_GameplaySettings_Free(UI_SETTINGS_STATE *s);
bool UI_GameplaySettings_Control(UI_SETTINGS_STATE *s);

void UI_GameplaySettings(UI_SETTINGS_STATE *s);
