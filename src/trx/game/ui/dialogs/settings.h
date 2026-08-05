#pragma once

#include <trx/config/option.h>
#include <trx/config/types.h>
#include <trx/core/utils.h>
#include <trx/game/ui/common.h>
#include <trx/game/ui/dialogs/settings_handlers.h>
#include <trx/game/ui/dialogs/settings_tabs.h>
#include <trx/game/ui/dialogs/text.h>
#include <trx/game/ui/elements/tab_switch.h>
#include <trx/game/ui/scrollable.h>

typedef struct UI_SETTINGS_DIALOG_STATE UI_SETTINGS_DIALOG_STATE;

UI_SETTINGS_DIALOG_STATE *UI_SettingsDialog_Init(
    GAME_STRING_ID title, int32_t tab_count, const UI_SETTINGS_TAB *tabs);
void UI_SettingsDialog_Free(UI_SETTINGS_DIALOG_STATE *s);
bool UI_SettingsDialog_Control(UI_SETTINGS_DIALOG_STATE *s);

void UI_SettingsDialog(UI_SETTINGS_DIALOG_STATE *s);
