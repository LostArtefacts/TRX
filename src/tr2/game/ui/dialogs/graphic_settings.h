#pragma once

#include <libtrx/game/ui/common.h>
#include <libtrx/game/ui/elements/requester.h>

typedef struct {
    UI_REQUESTER_STATE req;
} UI_GRAPHIC_SETTINGS_STATE;

void UI_GraphicSettings_Init(UI_GRAPHIC_SETTINGS_STATE *s);
void UI_GraphicSettings_Free(UI_GRAPHIC_SETTINGS_STATE *s);
bool UI_GraphicSettings_Control(UI_GRAPHIC_SETTINGS_STATE *s);

void UI_GraphicSettings(UI_GRAPHIC_SETTINGS_STATE *s);
