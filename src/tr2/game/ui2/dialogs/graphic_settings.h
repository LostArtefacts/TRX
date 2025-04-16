#pragma once

#include <libtrx/game/ui2/common.h>
#include <libtrx/game/ui2/elements/requester.h>

typedef struct {
    UI2_REQUESTER_STATE req;
} UI2_GRAPHIC_SETTINGS_STATE;

void UI2_GraphicSettings_Init(UI2_GRAPHIC_SETTINGS_STATE *s);
void UI2_GraphicSettings_Free(UI2_GRAPHIC_SETTINGS_STATE *s);
bool UI2_GraphicSettings_Control(UI2_GRAPHIC_SETTINGS_STATE *s);

void UI2_GraphicSettings(UI2_GRAPHIC_SETTINGS_STATE *s);
