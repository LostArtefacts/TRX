#pragma once

#include "game/game_string.h"

typedef struct {
    int32_t value;
    GAME_STRING_ID name;
} UI_SETTINGS_ENUM_ENTRY;

#if TR_VERSION == 1
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_StatDetailModeEnumEntries[];
#endif
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_TargetModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_LookModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_QuickGunsModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_JumpLockModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_WallGlitchEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_StatDetailModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_TargetModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_WallGlitchEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_HealthBarShowModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_AirBarShowModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_SprintBarShowModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY
    UI_Settings_ExposureBarShowModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY
    UI_Settings_EnemyHealthBarShowModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_BarLocationEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_BarColorEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_BarLookEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_BackgroundStyleEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_CameraModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_TextureFilterEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_MenuStyleEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_LightingContrastEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_SpriteLockModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_AspectModeEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_ScreenshotFormatEnumEntries[];
extern const UI_SETTINGS_ENUM_ENTRY UI_Settings_MusicLoadConditionEnumEntries[];
