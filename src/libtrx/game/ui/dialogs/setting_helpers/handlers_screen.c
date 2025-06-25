#include "config.h"
#include "game/ui/dialogs/setting_helpers/handlers.h"
#include "strings.h"

#if TR_VERSION == 1
// TODO: tidy me once we decide what to do about screen.c
extern int32_t Screen_GetResWidth(void);
extern int32_t Screen_GetResHeight(void);
extern bool Screen_CanSetPrevRes(void);
extern bool Screen_CanSetNextRes(void);
extern bool Screen_SetPrevRes(void);
extern bool Screen_SetNextRes(void);

const char *UI_Settings_ScreenResolution_FormatValue(
    const UI_SETTINGS_OPTION *const option)
{
    return String_FormatStatic(
        "%dx%d", Screen_GetResWidth(), Screen_GetResHeight());
}

bool UI_Settings_ScreenResolution_CanChangeValue(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    return dir < 0 ? Screen_CanSetPrevRes() : Screen_CanSetNextRes();
}

bool UI_Settings_ScreenResolution_RequestChangeValue(
    const UI_SETTINGS_OPTION *const option, const int32_t dir)
{
    bool result = false;
    if (dir < 0) {
        result = Screen_SetPrevRes();
    } else {
        result = Screen_SetNextRes();
    }
    if (result) {
        g_Config.rendering.resolution_width = Screen_GetResWidth();
        g_Config.rendering.resolution_height = Screen_GetResHeight();
    }
    return true;
}
#endif
