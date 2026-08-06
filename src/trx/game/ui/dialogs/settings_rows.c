#include <trx/game/ui/dialogs/settings_rows.h>

#include <trx/config/common.h>
#include <trx/config/registry.h>
#include <trx/core/memory.h>
#include <trx/core/utils.h>
#include <trx/core/vector.h>
#include <trx/debug.h>
#include <trx/game/ui/dialogs/settings_handlers.h>

#include <string.h>

#define X_UI_ROW(KEY_) QUOTE(KEY_),

// How far apart the rows a .def names are numbered, which is how many rows can
// be declared between any two of them before the tab has to be numbered again.
#define M_ROW_SPACING 100

// A row before it has an option: where it sits, and which setting it is for.
// This is what a tab is made of - the rows themselves are worked out from it
// whenever the options or the handlers move underneath.
typedef struct {
    // Owned where the row was declared; the .def's own string otherwise.
    const char *key;
    int32_t place;
    bool declared;
} M_ROW_SPEC;

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

// What a tab is called from outside the engine, which is the name of the .def
// its rows are written in.
static const char *const m_TabNames[CONFIG_TAB_COUNT] = {
    [CONFIG_TAB_GAMEPLAY_GENERAL] = "gameplay_general",
    [CONFIG_TAB_GAMEPLAY_CONTROLS] = "gameplay_controls",
    [CONFIG_TAB_GAMEPLAY_MODS] = "gameplay_mods",
    [CONFIG_TAB_GAMEPLAY_FIXES] = "gameplay_fixes",
    [CONFIG_TAB_GRAPHIC_VISUALS] = "graphic_visuals",
    [CONFIG_TAB_GRAPHIC_UI] = "graphic_ui",
    [CONFIG_TAB_GRAPHIC_UI_STATS] = "graphic_ui_stats",
    [CONFIG_TAB_GRAPHIC_UI_BARS] = "graphic_ui_bars",
    [CONFIG_TAB_GRAPHIC_RENDERING] = "graphic_rendering",
    [CONFIG_TAB_SOUND_VOLUME] = "sound_volume",
    [CONFIG_TAB_SOUND_MISC] = "sound_misc",
};

// Every tab's rows, in the order they are shown. Kept sorted by place.
static VECTOR *m_Specs[CONFIG_TAB_COUNT] = {};
static VECTOR *m_Rows[CONFIG_TAB_COUNT] = {};
// What the rows were arranged against: the options, which are dropped and made
// again when the game changes, the handlers, which a row keeps one of, and the
// specs, which a script adds to.
static int32_t m_ConfigGeneration = -1;
static int32_t m_HandlerGeneration = -1;
static int32_t m_SpecGeneration = 0;
static int32_t m_ArrangedSpecGeneration = -1;

static M_ROW_SPEC *M_GetSpec(const CONFIG_TAB tab, const int32_t idx)
{
    return Vector_Get(m_Specs[tab], idx);
}

static void M_EnsureSpecs(const CONFIG_TAB tab)
{
    if (m_Specs[tab] != nullptr) {
        return;
    }
    m_Specs[tab] = Vector_Create(sizeof(M_ROW_SPEC));
    const char *const *const keys = m_TabKeys[tab];
    ASSERT(keys != nullptr);
    for (int32_t i = 0; keys[i] != nullptr; i++) {
        Vector_Add(
            m_Specs[tab],
            &(M_ROW_SPEC) { .key = keys[i], .place = i * M_ROW_SPACING });
    }
}

// The row for the option a name names, by the key the settings file knows the
// option by. A row's key and an anchor may spell the same option differently -
// one by the whole path, one by its last segment - and that segment is unique
// across the options, so it is what they are compared by.
static int32_t M_FindSpec(const CONFIG_TAB tab, const char *const key)
{
    if (key == nullptr) {
        return -1;
    }
    const char *const leaf = Config_ResolveOptionName(key);
    for (int32_t i = 0; i < m_Specs[tab]->count; i++) {
        if (strcmp(Config_ResolveOptionName(M_GetSpec(tab, i)->key), leaf)
            == 0) {
            return i;
        }
    }
    return -1;
}

// Numbers a tab's rows afresh, spaced as the .def's own are, for a tab that has
// no room left between two of its rows.
static void M_Renumber(const CONFIG_TAB tab)
{
    for (int32_t i = 0; i < m_Specs[tab]->count; i++) {
        M_GetSpec(tab, i)->place = i * M_ROW_SPACING;
    }
}

// The place a row takes to land between the two rows an index sits between.
// The same number as the row below it means the gap is closed.
static int32_t M_PlaceBetween(
    const CONFIG_TAB tab, const int32_t below_idx, const int32_t above_idx)
{
    const int32_t count = m_Specs[tab]->count;
    const int32_t below = below_idx >= 0
        ? M_GetSpec(tab, below_idx)->place
        : M_GetSpec(tab, above_idx)->place - 2 * M_ROW_SPACING;
    const int32_t above = above_idx < count
        ? M_GetSpec(tab, above_idx)->place
        : M_GetSpec(tab, below_idx)->place + 2 * M_ROW_SPACING;
    return below + (above - below) / 2;
}

static int32_t M_ResolvePlace(
    const CONFIG_TAB tab, const char *const before, const char *const after)
{
    const int32_t count = m_Specs[tab]->count;
    if (count == 0) {
        return 0;
    }

    const int32_t anchor_idx =
        before != nullptr ? M_FindSpec(tab, before) : M_FindSpec(tab, after);
    if (anchor_idx < 0) {
        return M_GetSpec(tab, count - 1)->place + M_ROW_SPACING;
    }

    int32_t below_idx = anchor_idx - 1;
    if (before == nullptr) {
        // A row already declared against this anchor sits between it and the
        // next row the .def names. A second one goes after that one rather than
        // between the two, so several rows declared against one anchor read in
        // the order they were declared in.
        below_idx = anchor_idx;
        while (below_idx + 1 < count
               && M_GetSpec(tab, below_idx + 1)->declared) {
            below_idx++;
        }
    }
    int32_t place = M_PlaceBetween(tab, below_idx, below_idx + 1);
    if (below_idx >= 0 && place == M_GetSpec(tab, below_idx)->place) {
        M_Renumber(tab);
        place = M_PlaceBetween(tab, below_idx, below_idx + 1);
    }
    return place;
}

static void M_Arrange(void)
{
    m_ConfigGeneration = Config_GetGeneration();
    m_HandlerGeneration = UI_Settings_GetHandlerGeneration();
    m_ArrangedSpecGeneration = m_SpecGeneration;
    for (int32_t tab = 0; tab < CONFIG_TAB_COUNT; tab++) {
        M_EnsureSpecs(tab);
        if (m_Rows[tab] == nullptr) {
            m_Rows[tab] = Vector_Create(sizeof(UI_SETTINGS_ROW));
        }
        Vector_Clear(m_Rows[tab]);
        for (int32_t i = 0; i < m_Specs[tab]->count; i++) {
            // A tab naming a setting this game does not have is not an error:
            // one tab list serves every game, as one handler registration does.
            CONFIG_OPTION *const option =
                Config_FindOption(M_GetSpec(tab, i)->key);
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
        || m_HandlerGeneration != UI_Settings_GetHandlerGeneration()
        || m_ArrangedSpecGeneration != m_SpecGeneration) {
        M_Arrange();
    }
}

__attribute__((destructor)) static void M_Shutdown(void)
{
    UI_Settings_DropDeclaredRows();
    for (int32_t tab = 0; tab < CONFIG_TAB_COUNT; tab++) {
        if (m_Rows[tab] != nullptr) {
            Vector_Free(m_Rows[tab]);
            m_Rows[tab] = nullptr;
        }
        if (m_Specs[tab] != nullptr) {
            Vector_Free(m_Specs[tab]);
            m_Specs[tab] = nullptr;
        }
    }
}

bool UI_Settings_FindTab(const char *const name, CONFIG_TAB *const out)
{
    ASSERT(out != nullptr);
    if (name == nullptr) {
        return false;
    }
    for (int32_t tab = 0; tab < CONFIG_TAB_COUNT; tab++) {
        if (strcmp(m_TabNames[tab], name) == 0) {
            *out = tab;
            return true;
        }
    }
    return false;
}

void UI_Settings_AddDeclaredRow(
    const CONFIG_TAB tab, const char *const key, const char *const before,
    const char *const after)
{
    ASSERT(tab >= 0 && tab < CONFIG_TAB_COUNT);
    ASSERT(key != nullptr);
    M_EnsureSpecs(tab);

    const int32_t place = M_ResolvePlace(tab, before, after);
    int32_t idx = m_Specs[tab]->count;
    while (idx > 0 && M_GetSpec(tab, idx - 1)->place > place) {
        idx--;
    }
    Vector_Insert(
        m_Specs[tab], idx,
        &(M_ROW_SPEC) {
            .key = Memory_DupStr(key), .place = place, .declared = true });
    m_SpecGeneration++;
}

void UI_Settings_DropDeclaredRows(void)
{
    for (int32_t tab = 0; tab < CONFIG_TAB_COUNT; tab++) {
        if (m_Specs[tab] == nullptr) {
            continue;
        }
        for (int32_t i = m_Specs[tab]->count - 1; i >= 0; i--) {
            M_ROW_SPEC *const spec = M_GetSpec(tab, i);
            if (!spec->declared) {
                continue;
            }
            Memory_Free((char *)spec->key);
            Vector_RemoveAt(m_Specs[tab], i);
        }
    }
    m_SpecGeneration++;
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
