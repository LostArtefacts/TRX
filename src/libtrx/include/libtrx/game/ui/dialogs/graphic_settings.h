#pragma once

#include "../common.h"
#include "./settings.h"

UI_SETTINGS_STATE *UI_GraphicSettings_Init(void);
void UI_GraphicSettings_Free(UI_SETTINGS_STATE *s);
bool UI_GraphicSettings_Control(UI_SETTINGS_STATE *s);

void UI_GraphicSettings(UI_SETTINGS_STATE *s);
