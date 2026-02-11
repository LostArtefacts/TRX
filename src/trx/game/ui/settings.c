#include <trx/game/ui/settings.h>

#include <trx/config.h>
#include <trx/game/game_string.h>
#include <trx/game/shell.h>
#include <trx/json/util/file.h>
#include <trx/memory.h>
#include <trx/strings.h>

#include <uthash.h>

typedef struct {
    char *name;
    UI_BAR_THEME theme;
} M_THEME_ENTRY;

typedef struct M_THEME_LOOKUP {
    char *name;
    int32_t index;
    UT_hash_handle hh;
} M_THEME_LOOKUP;

typedef struct {
    int32_t color_count;
    M_THEME_ENTRY *colors;
    struct M_THEME_LOOKUP *lookup;
} M_THEME_GROUP;

typedef struct {
    char *name;
    char *name_gs;
    UI_BAR_THEME_KIND kind;
    M_THEME_GROUP group;
} M_BAR_THEME_ENTRY;

typedef struct M_BAR_THEME_LOOKUP {
    char *name;
    int32_t index;
    UT_hash_handle hh;
} M_BAR_THEME_LOOKUP;

typedef struct {
    int32_t bar_theme_count;
    M_BAR_THEME_ENTRY *bar_themes;
    struct M_BAR_THEME_LOOKUP *bar_lookup;
} M_SETTINGS;

typedef struct {
    char *const *const pc_color;
    char *const *const ps1_color;
} M_BAR_COLOR_SELECT;

static const M_BAR_COLOR_SELECT m_BarColorSelect[UI_BAR_NUMBER_OF] = {
    [UI_BAR_LARA_HP] = {
        .pc_color = &g_Config.ui.lara_health_bar.color,
        .ps1_color = &g_Config.ui.lara_health_bar.color_ps1,
    },
    [UI_BAR_LARA_HP_POISON] = {
        .pc_color = &g_Config.ui.lara_health_bar.poison_color,
        .ps1_color = &g_Config.ui.lara_health_bar.poison_color_ps1,
    },
    [UI_BAR_LARA_AIR] = {
        .pc_color = &g_Config.ui.lara_air_bar.color,
        .ps1_color = &g_Config.ui.lara_air_bar.color_ps1,
    },
    [UI_BAR_LARA_STAMINA] = {
        .pc_color = &g_Config.ui.lara_sprint_bar.color,
        .ps1_color = &g_Config.ui.lara_sprint_bar.color_ps1,
    },
    [UI_BAR_LARA_EXPOSURE] = {
        .pc_color = &g_Config.ui.lara_exposure_bar.color,
        .ps1_color = &g_Config.ui.lara_exposure_bar.color_ps1,
    },
    [UI_BAR_ENEMY_HP] = {
        .pc_color = &g_Config.ui.enemy_health_bar.color,
        .ps1_color = &g_Config.ui.enemy_health_bar.color_ps1,
    },
    [UI_BAR_ALLY_HP] = {
        .pc_color = &g_Config.ui.enemy_health_bar.color_allies,
        .ps1_color = &g_Config.ui.enemy_health_bar.color_allies_ps1,
    },
};

static M_SETTINGS m_Settings;

static void M_FreeThemeGroup(M_THEME_GROUP *const group)
{
    M_THEME_LOOKUP *entry = nullptr;
    M_THEME_LOOKUP *tmp = nullptr;
    HASH_ITER(hh, group->lookup, entry, tmp)
    {
        HASH_DEL(group->lookup, entry);
        Memory_FreePointer(&entry);
    }
    if (group->colors == nullptr) {
        return;
    }
    for (int32_t i = 0; i < group->color_count; i++) {
        Memory_FreePointer(&group->colors[i].name);
    }
    Memory_FreePointer(&group->colors);
    group->color_count = 0;
    group->lookup = nullptr;
}

static void M_ResetDynamicEnumValues(void)
{
    const CONFIG_OPTION *const bar_look_option =
        Config_GetOption(&g_Config.ui.bar_look);
    if (bar_look_option != nullptr) {
        Config_DynamicEnum_ResetValues(bar_look_option);
    }

    for (int32_t i = 0; i < UI_BAR_NUMBER_OF; i++) {
        const M_BAR_COLOR_SELECT *const select = &m_BarColorSelect[i];
        const CONFIG_OPTION *const pc_option =
            Config_GetOption(select->pc_color);
        if (pc_option != nullptr) {
            Config_DynamicEnum_ResetValues(pc_option);
        }
        const CONFIG_OPTION *const ps1_option =
            Config_GetOption(select->ps1_color);
        if (ps1_option != nullptr) {
            Config_DynamicEnum_ResetValues(ps1_option);
        }
    }
}

static bool M_IsBarColorNameEncountered(
    const UI_BAR_THEME_KIND kind, const char *const name, const int32_t stop_i,
    const int32_t stop_j)
{
    for (int32_t i = 0; i < m_Settings.bar_theme_count; i++) {
        M_BAR_THEME_ENTRY *const theme = &m_Settings.bar_themes[i];
        if (theme->kind != kind) {
            continue;
        }
        for (int32_t j = 0; j < theme->group.color_count; j++) {
            if (i == stop_i && j == stop_j) {
                return false;
            }
            if (String_Equivalent(theme->group.colors[j].name, name)) {
                return true;
            }
        }
    }
    return false;
}

static void M_SeedDynamicEnumBarColors(
    const CONFIG_OPTION *const option, const UI_BAR_THEME_KIND kind)
{
    Config_DynamicEnum_ResetValues(option);
    for (int32_t i = 0; i < m_Settings.bar_theme_count; i++) {
        const M_BAR_THEME_ENTRY *const theme = &m_Settings.bar_themes[i];
        if (theme->kind != kind) {
            continue;
        }
        for (int32_t j = 0; j < theme->group.color_count; j++) {
            const char *const name = theme->group.colors[j].name;
            if (M_IsBarColorNameEncountered(kind, name, i, j)) {
                continue;
            }
            Config_DynamicEnum_AddValue(option, name, nullptr);
        }
    }
}

static void M_SeedDynamicEnumValues(void)
{
    const CONFIG_OPTION *const bar_look_option =
        Config_GetOption(&g_Config.ui.bar_look);
    if (bar_look_option != nullptr) {
        Config_DynamicEnum_ResetValues(bar_look_option);
        for (int32_t i = 0; i < m_Settings.bar_theme_count; i++) {
            const M_BAR_THEME_ENTRY *const theme = &m_Settings.bar_themes[i];
            Config_DynamicEnum_AddValue(
                bar_look_option, theme->name, theme->name_gs);
        }
    }

    for (int32_t i = 0; i < UI_BAR_NUMBER_OF; i++) {
        const M_BAR_COLOR_SELECT *const select = &m_BarColorSelect[i];
        M_SeedDynamicEnumBarColors(
            Config_GetOption(select->pc_color), UI_BAR_THEME_PC_KIND);
        M_SeedDynamicEnumBarColors(
            Config_GetOption(select->ps1_color), UI_BAR_THEME_PS1_KIND);
    }
}

static void M_FreeBarThemes(void)
{
    M_ResetDynamicEnumValues();

    M_BAR_THEME_LOOKUP *entry = nullptr;
    M_BAR_THEME_LOOKUP *tmp = nullptr;
    HASH_ITER(hh, m_Settings.bar_lookup, entry, tmp)
    {
        HASH_DEL(m_Settings.bar_lookup, entry);
        Memory_FreePointer(&entry);
    }

    if (m_Settings.bar_themes == nullptr) {
        return;
    }

    for (int32_t i = 0; i < m_Settings.bar_theme_count; i++) {
        M_BAR_THEME_ENTRY *const theme = &m_Settings.bar_themes[i];
        Memory_FreePointer(&theme->name);
        Memory_FreePointer(&theme->name_gs);
        M_FreeThemeGroup(&theme->group);
    }

    Memory_FreePointer(&m_Settings.bar_themes);
    m_Settings.bar_theme_count = 0;
    m_Settings.bar_lookup = nullptr;
}

static bool M_ParseHexColor(const char *const value, RGBA_8888 *const out)
{
    RGB_888 color = {};
    if (!String_ParseRGB888(value, &color)) {
        return false;
    }
    out->r = color.r;
    out->g = color.g;
    out->b = color.b;
    out->a = 255;
    return true;
}

static void M_ReadColorArray(
    JSON_ARRAY *const arr, RGBA_8888 colors[UI_BAR_COLOR_STEPS],
    const char *path, const char *key)
{
    if (arr == nullptr || arr->length != UI_BAR_COLOR_STEPS) {
        Shell_ExitSystemFmt(
            "invalid '%s' array in %s (expected %d entries)", key, path,
            UI_BAR_COLOR_STEPS);
    }
    for (size_t i = 0; i < UI_BAR_COLOR_STEPS; i++) {
        const char *const value = JSON_ArrayGetString(arr, i, nullptr);
        if (value == nullptr || !M_ParseHexColor(value, &colors[i])) {
            Shell_ExitSystemFmt(
                "invalid '%s' color '%s' in %s", key,
                value != nullptr ? value : "(null)", path);
        }
    }
}

static void M_LoadThemesPC(
    JSON_OBJECT *const obj, M_THEME_GROUP *const group, const char *const path,
    const char *const section)
{
    const float basic_scale = (float)JSON_ObjectGetDouble(obj, "scale", 1.0f);
    RGBA_8888 border_light = {};
    RGBA_8888 border_dark = {};

    if (!M_ParseHexColor(
            JSON_ObjectGetString(obj, "border_light", JSON_INVALID_STRING),
            &border_light)) {
        Shell_ExitSystemFmt("missing '%s.border_light' in %s", section, path);
    }

    if (!M_ParseHexColor(
            JSON_ObjectGetString(obj, "border_dark", JSON_INVALID_STRING),
            &border_dark)) {
        Shell_ExitSystemFmt("missing '%s.border_dark' in %s", section, path);
    }

    JSON_OBJECT *const colors_obj = JSON_ObjectGetObject(obj, "colors");

    if (colors_obj == nullptr) {
        Shell_ExitSystemFmt("missing '%s.colors' in %s", section, path);
    }

    M_FreeThemeGroup(group);

    size_t count = 0;
    for (JSON_OBJECT_ELEMENT *elem = colors_obj->start; elem != nullptr;
         elem = elem->next) {
        count++;
    }
    if (count == 0) {
        Shell_ExitSystemFmt("empty '%s.colors' in %s", section, path);
    }

    group->colors = Memory_Alloc(sizeof(*group->colors) * count);
    group->color_count = (int32_t)count;
    group->lookup = nullptr;

    size_t idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = colors_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        JSON_ARRAY *const arr = JSON_ValueAsArray(elem->value);
        group->colors[idx].name = Memory_DupStr(name);
        M_THEME_LOOKUP *existing = nullptr;
        HASH_FIND_STR(group->lookup, group->colors[idx].name, existing);
        if (existing != nullptr) {
            Shell_ExitSystemFmt(
                "duplicate '%s.colors.%s' in %s", section, name, path);
        }
        M_THEME_LOOKUP *const entry = Memory_Alloc(sizeof(*entry));
        entry->name = group->colors[idx].name;
        entry->index = (int32_t)idx;
        HASH_ADD_KEYPTR(
            hh, group->lookup, entry->name, strlen(entry->name), entry);
        UI_BAR_THEME *const theme = &group->colors[idx].theme;
        *theme = (UI_BAR_THEME) {
            .kind = UI_BAR_THEME_PC_KIND,
            .basic_scale = basic_scale,
            .border_light = border_light,
            .border_dark = border_dark,
        };
        M_ReadColorArray(
            arr, theme->ramp, path,
            String_FormatStatic("%s.colors.%s", section, name));
        idx++;
    }
}

static void M_LoadThemesPS1(
    JSON_OBJECT *const obj, M_THEME_GROUP *const group, const char *const path,
    const char *const section)
{
    const float basic_scale = (float)JSON_ObjectGetDouble(obj, "scale", 1.0f);
    RGBA_8888 border_tl = {};
    RGBA_8888 border_tr = {};
    RGBA_8888 border_bl = {};
    RGBA_8888 border_br = {};

    if (!M_ParseHexColor(
            JSON_ObjectGetString(obj, "border_tl", JSON_INVALID_STRING),
            &border_tl)) {
        Shell_ExitSystemFmt("missing '%s.border_tl' in %s", section, path);
    }

    if (!M_ParseHexColor(
            JSON_ObjectGetString(obj, "border_tr", JSON_INVALID_STRING),
            &border_tr)) {
        Shell_ExitSystemFmt("missing '%s.border_tr' in %s", section, path);
    }

    if (!M_ParseHexColor(
            JSON_ObjectGetString(obj, "border_bl", JSON_INVALID_STRING),
            &border_bl)) {
        Shell_ExitSystemFmt("missing '%s.border_bl' in %s", section, path);
    }

    if (!M_ParseHexColor(
            JSON_ObjectGetString(obj, "border_br", JSON_INVALID_STRING),
            &border_br)) {
        Shell_ExitSystemFmt("missing '%s.border_br' in %s", section, path);
    }
    JSON_OBJECT *const colors_obj = JSON_ObjectGetObject(obj, "colors");
    if (colors_obj == nullptr) {
        Shell_ExitSystemFmt("missing '%s.colors' in %s", section, path);
    }

    M_FreeThemeGroup(group);

    size_t count = 0;
    for (JSON_OBJECT_ELEMENT *elem = colors_obj->start; elem != nullptr;
         elem = elem->next) {
        count++;
    }
    if (count == 0) {
        Shell_ExitSystemFmt("empty '%s.colors' in %s", section, path);
    }

    group->colors = Memory_Alloc(sizeof(*group->colors) * count);
    group->color_count = (int32_t)count;
    group->lookup = nullptr;

    size_t idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = colors_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        JSON_ARRAY *const arr = JSON_ValueAsArray(elem->value);
        if (arr == nullptr || arr->length != 2) {
            Shell_ExitSystemFmt(
                "invalid '%s.colors.%s' in %s (expected 2 arrays)", section,
                name, path);
        }
        JSON_ARRAY *const left_arr =
            JSON_ValueAsArray(JSON_ArrayGetValue(arr, 0));
        JSON_ARRAY *const right_arr =
            JSON_ValueAsArray(JSON_ArrayGetValue(arr, 1));
        group->colors[idx].name = Memory_DupStr(name);
        M_THEME_LOOKUP *existing = nullptr;
        HASH_FIND_STR(group->lookup, group->colors[idx].name, existing);
        if (existing != nullptr) {
            Shell_ExitSystemFmt(
                "duplicate '%s.colors.%s' in %s", section, name, path);
        }
        M_THEME_LOOKUP *const entry = Memory_Alloc(sizeof(*entry));
        entry->name = group->colors[idx].name;
        entry->index = (int32_t)idx;
        HASH_ADD_KEYPTR(
            hh, group->lookup, entry->name, strlen(entry->name), entry);
        UI_BAR_THEME *const theme = &group->colors[idx].theme;
        *theme = (UI_BAR_THEME) {
            .kind = UI_BAR_THEME_PS1_KIND,
            .basic_scale = basic_scale,
            .border_tl = border_tl,
            .border_tr = border_tr,
            .border_bl = border_bl,
            .border_br = border_br,
        };
        M_ReadColorArray(
            left_arr, theme->ramp_left, path,
            String_FormatStatic("%s.colors.%s[0]", section, name));
        M_ReadColorArray(
            right_arr, theme->ramp_right, path,
            String_FormatStatic("%s.colors.%s[1]", section, name));
        idx++;
    }
}

static M_BAR_THEME_ENTRY *M_FindBarThemeByName(const char *const name)
{
    if (name == nullptr) {
        return nullptr;
    }
    M_BAR_THEME_LOOKUP *entry = nullptr;
    HASH_FIND_STR(m_Settings.bar_lookup, name, entry);
    if (entry == nullptr) {
        return nullptr;
    }
    return &m_Settings.bar_themes[entry->index];
}

static M_BAR_THEME_ENTRY *M_GetCurrentBarTheme(void)
{
    M_BAR_THEME_ENTRY *theme = M_FindBarThemeByName(g_Config.ui.bar_look);
    if (theme != nullptr) {
        return theme;
    }
    if (m_Settings.bar_theme_count <= 0) {
        return nullptr;
    }
    return &m_Settings.bar_themes[0];
}

static const M_THEME_GROUP *M_GetCurrentBarGroup(void)
{
    M_BAR_THEME_ENTRY *const theme = M_GetCurrentBarTheme();
    if (theme == nullptr) {
        return nullptr;
    }
    return &theme->group;
}

void UI_Settings_LoadFromFile(const char *const path)
{
    JSON_VALUE *const root =
        JSONFile_ReadEx(path, (JSON_FILE_OPTIONS) { .exit_on_error = true });
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        Shell_ExitSystemFmt("invalid ui settings file: %s", path);
    }

    M_FreeBarThemes();

    size_t theme_count = 0;
    for (JSON_OBJECT_ELEMENT *elem = root_obj->start; elem != nullptr;
         elem = elem->next) {
        theme_count++;
    }
    if (theme_count == 0) {
        Shell_ExitSystemFmt("ui settings file has no bar themes: %s", path);
    }

    m_Settings.bar_themes =
        Memory_Alloc(sizeof(*m_Settings.bar_themes) * theme_count);
    m_Settings.bar_theme_count = (int32_t)theme_count;
    m_Settings.bar_lookup = nullptr;

    size_t idx = 0;
    for (JSON_OBJECT_ELEMENT *elem = root_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const theme_name = elem->name->string;
        JSON_OBJECT *const theme_obj = JSON_ValueAsObject(elem->value);
        if (theme_obj == nullptr) {
            Shell_ExitSystemFmt(
                "invalid '%s' in %s (expected object)", theme_name, path);
        }

        M_BAR_THEME_ENTRY *const theme = &m_Settings.bar_themes[idx];
        theme->name = Memory_DupStr(theme_name);
        theme->name_gs = nullptr;
        theme->kind = UI_BAR_THEME_PC_KIND;
        theme->group = (M_THEME_GROUP) {};

        M_BAR_THEME_LOOKUP *existing = nullptr;
        HASH_FIND_STR(m_Settings.bar_lookup, theme->name, existing);
        if (existing != nullptr) {
            Shell_ExitSystemFmt("duplicate '%s' in %s", theme_name, path);
        }

        const char *const name_gs =
            JSON_ObjectGetString(theme_obj, "name_gs", JSON_INVALID_STRING);
        if (name_gs == JSON_INVALID_STRING) {
            Shell_ExitSystemFmt(
                "invalid '%s.name_gs' in %s (unknown game string ID)",
                theme_name, path);
        }
        theme->name_gs = Memory_DupStr(name_gs);

        const char *const style =
            JSON_ObjectGetString(theme_obj, "style", JSON_INVALID_STRING);
        if (style == JSON_INVALID_STRING) {
            Shell_ExitSystemFmt("missing '%s.style' in %s", theme_name, path);
        }
        if (String_Equivalent(style, "pc")) {
            theme->kind = UI_BAR_THEME_PC_KIND;
            M_LoadThemesPC(theme_obj, &theme->group, path, theme_name);
        } else if (String_Equivalent(style, "ps1")) {
            theme->kind = UI_BAR_THEME_PS1_KIND;
            M_LoadThemesPS1(theme_obj, &theme->group, path, theme_name);
        } else {
            Shell_ExitSystemFmt("invalid '%s.style' in %s", theme_name, path);
        }

        M_BAR_THEME_LOOKUP *const entry = Memory_Alloc(sizeof(*entry));
        entry->name = theme->name;
        entry->index = (int32_t)idx;
        HASH_ADD_KEYPTR(
            hh, m_Settings.bar_lookup, entry->name, strlen(entry->name), entry);
        idx++;
    }

    M_SeedDynamicEnumValues();

    JSON_ValueFree(root);
}

static const char *M_GetBarColorName(const UI_BAR_TYPE type)
{
    if (type < 0 || type >= UI_BAR_NUMBER_OF) {
        return "gold";
    }

    const M_BAR_THEME_ENTRY *const theme = M_GetCurrentBarTheme();
    const bool use_ps1 =
        theme != nullptr && theme->kind == UI_BAR_THEME_PS1_KIND;
    const M_BAR_COLOR_SELECT *const select = &m_BarColorSelect[type];
    const char *value = nullptr;

    if (use_ps1 && select->ps1_color != nullptr) {
        value = *select->ps1_color;
    } else if (!use_ps1 && select->pc_color != nullptr) {
        value = *select->pc_color;
    }

    return value;
}

static const UI_BAR_THEME *M_FindThemeByName(
    const M_THEME_GROUP *const group, const char *const name)
{
    if (group == nullptr || group->colors == nullptr || group->color_count <= 0
        || name == nullptr) {
        return nullptr;
    }
    M_THEME_LOOKUP *entry = nullptr;
    HASH_FIND_STR(group->lookup, name, entry);
    if (entry != nullptr) {
        return &group->colors[entry->index].theme;
    }
    return nullptr;
}

bool UI_Settings_IsCurrentBarLookPS1(void)
{
    const M_BAR_THEME_ENTRY *const theme = M_GetCurrentBarTheme();
    return theme != nullptr && theme->kind == UI_BAR_THEME_PS1_KIND;
}

const UI_BAR_THEME *UI_Settings_GetBarTheme(const UI_BAR_TYPE type)
{
    if (type < 0 || type >= UI_BAR_NUMBER_OF) {
        return nullptr;
    }
    const M_THEME_GROUP *const group = M_GetCurrentBarGroup();
    if (group == nullptr || group->color_count <= 0) {
        return nullptr;
    }
    const char *const name = M_GetBarColorName(type);
    const UI_BAR_THEME *theme = M_FindThemeByName(group, name);
    if (theme != nullptr) {
        return theme;
    }
    return &group->colors[0].theme;
}
