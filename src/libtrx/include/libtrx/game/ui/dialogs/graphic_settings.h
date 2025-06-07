#pragma once

#include "../common.h"
#include "./settings.h"

typedef UI_SETTINGS_STATE UI_GRAPHIC_SETTINGS_STATE;

void UI_GraphicSettings_Init(UI_GRAPHIC_SETTINGS_STATE *s);
void UI_GraphicSettings_Free(UI_GRAPHIC_SETTINGS_STATE *s);
bool UI_GraphicSettings_Control(UI_GRAPHIC_SETTINGS_STATE *s);

void UI_GraphicSettings(UI_GRAPHIC_SETTINGS_STATE *s);
