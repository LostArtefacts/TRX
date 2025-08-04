#pragma once

#include "game/ui/dialogs/settings.h"

#if TR_VERSION == 1
bool UI_Settings_EnablePS1Crystals_IsAvailable(
    const UI_SETTINGS_OPTION *option);
#endif

bool UI_Settings_FixItemRots_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_FixStepGlitch_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_FixWadeWallHit_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_EnableExitFadeEffects_IsAvailable(
    const UI_SETTINGS_OPTION *option);
bool UI_Settings_PauseMusicInInventory_IsAvailable(
    const UI_SETTINGS_OPTION *option);

bool UI_Settings_EnableBreeze_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_Healthbar_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_Airbar_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_Sprint_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_Sprintbar_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_EnemyHealthbar_IsAvailable(const UI_SETTINGS_OPTION *option);
bool UI_Settings_AllyHealthbar_IsAvailable(const UI_SETTINGS_OPTION *option);

const char *UI_Settings_Language_FormatValue(const UI_SETTINGS_OPTION *option);
bool UI_Settings_Language_CanChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);
bool UI_Settings_Language_RequestChangeValue(
    const UI_SETTINGS_OPTION *option, int32_t dir);

bool UI_Settings_Volume_RequestChange(
    const UI_SETTINGS_OPTION *option, int32_t dir);
