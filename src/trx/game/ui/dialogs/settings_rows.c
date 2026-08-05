#include <trx/game/ui/dialogs/settings_rows.h>

#include <trx/config/registry.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/ui/dialogs/settings_handlers.h>

#define X_UI_ROW(KEY_) QUOTE(KEY_),

static const char *const m_GameplayGeneralKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/gameplay_general.def>
    nullptr,
};

static const char *const m_GameplayControlsKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/gameplay_controls.def>
    nullptr,
};

static const char *const m_GameplayModsKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/gameplay_mods.def>
    nullptr,
};

static const char *const m_GameplayFixesKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/gameplay_fixes.def>
    nullptr,
};

static const char *const m_GraphicVisualsKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_visuals.def>
    nullptr,
};

static const char *const m_GraphicUIKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_ui.def>
    nullptr,
};

static const char *const m_GraphicUIStatsKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_ui_stats.def>
    nullptr,
};

static const char *const m_GraphicUIBarsKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_ui_bars.def>
    nullptr,
};

static const char *const m_GraphicRenderingKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/graphic_rendering.def>
    nullptr,
};

static const char *const m_SoundVolumeKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/sound_volume.def>
    nullptr,
};

static const char *const m_SoundMiscKeys[] = {
#include <trx/game/ui/dialogs/setting_tabs/sound_misc.def>
    nullptr,
};

static const char *const *const m_TabKeys[CONFIG_TAB_COUNT] = {
    [CONFIG_TAB_GAMEPLAY_GENERAL] = m_GameplayGeneralKeys,
    [CONFIG_TAB_GAMEPLAY_CONTROLS] = m_GameplayControlsKeys,
    [CONFIG_TAB_GAMEPLAY_MODS] = m_GameplayModsKeys,
    [CONFIG_TAB_GAMEPLAY_FIXES] = m_GameplayFixesKeys,
    [CONFIG_TAB_GRAPHIC_VISUALS] = m_GraphicVisualsKeys,
    [CONFIG_TAB_GRAPHIC_UI] = m_GraphicUIKeys,
    [CONFIG_TAB_GRAPHIC_UI_STATS] = m_GraphicUIStatsKeys,
    [CONFIG_TAB_GRAPHIC_UI_BARS] = m_GraphicUIBarsKeys,
    [CONFIG_TAB_GRAPHIC_RENDERING] = m_GraphicRenderingKeys,
    [CONFIG_TAB_SOUND_VOLUME] = m_SoundVolumeKeys,
    [CONFIG_TAB_SOUND_MISC] = m_SoundMiscKeys,
};

static VECTOR *m_Rows[CONFIG_TAB_COUNT] = {};
// What the rows were arranged against: the options, which are dropped and made
// again when the game changes, and the handlers, which a row keeps one of.
static int32_t m_ConfigGeneration = -1;
static int32_t m_HandlerGeneration = -1;

static void M_Arrange(void)
{
    m_ConfigGeneration = Config_GetGeneration();
    m_HandlerGeneration = UI_Settings_GetHandlerGeneration();
    for (int32_t tab = 0; tab < CONFIG_TAB_COUNT; tab++) {
        if (m_Rows[tab] == nullptr) {
            m_Rows[tab] = Vector_Create(sizeof(UI_SETTINGS_ROW));
        }
        Vector_Clear(m_Rows[tab]);
        const char *const *const keys = m_TabKeys[tab];
        ASSERT(keys != nullptr);
        for (int32_t i = 0; keys[i] != nullptr; i++) {
            // A tab naming a setting this game does not have is not an error:
            // one tab list serves every game, as one handler registration does.
            CONFIG_OPTION *const option = Config_FindOption(keys[i]);
            if (option != nullptr) {
                Vector_Add(
                    m_Rows[tab],
                    &(UI_SETTINGS_ROW) { option,
                                         UI_Settings_GetHandler(option) });
            }
        }
    }
}

static void M_Ensure(void)
{
    if (m_ConfigGeneration != Config_GetGeneration()
        || m_HandlerGeneration != UI_Settings_GetHandlerGeneration()) {
        M_Arrange();
    }
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    for (int32_t tab = 0; tab < CONFIG_TAB_COUNT; tab++) {
        if (m_Rows[tab] != nullptr) {
            Vector_Free(m_Rows[tab]);
            m_Rows[tab] = nullptr;
        }
    }
}

int32_t UI_Settings_GetRowCount(const CONFIG_TAB tab)
{
    ASSERT(tab >= 0 && tab < CONFIG_TAB_COUNT);
    M_Ensure();
    return m_Rows[tab]->count;
}

const UI_SETTINGS_ROW *UI_Settings_GetRow(
    const CONFIG_TAB tab, const int32_t index)
{
    ASSERT(tab >= 0 && tab < CONFIG_TAB_COUNT);
    M_Ensure();
    if (index < 0 || index >= m_Rows[tab]->count) {
        return nullptr;
    }
    return Vector_Get(m_Rows[tab], index);
}
