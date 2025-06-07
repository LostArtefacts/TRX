#pragma once

#include "../common.h"
#include "./settings.h"

typedef UI_SETTINGS_STATE UI_GAMEPLAY_SETTINGS_STATE;

void UI_GameplaySettings_Init(UI_GAMEPLAY_SETTINGS_STATE *s);
void UI_GameplaySettings_Free(UI_GAMEPLAY_SETTINGS_STATE *s);
bool UI_GameplaySettings_Control(UI_GAMEPLAY_SETTINGS_STATE *s);

void UI_GameplaySettings(UI_GAMEPLAY_SETTINGS_STATE *s);
