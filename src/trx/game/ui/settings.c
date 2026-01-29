#include <trx/game/ui/settings.h>

#include <trx/config.h>
#include <trx/game/shell.h>
#include <trx/json_file.h>
#include <trx/memory.h>
#include <trx/strings.h>
#include <trx/version.h>

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
    struct {
        M_THEME_GROUP tr1, tr2, tr3, ps1;
    } bars;
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

static const M_THEME_GROUP *M_GetCurrentBarGroup(void)
{
    switch (g_Config.ui.bar_look) {
    case BAR_LOOK_TR1:
        return &m_Settings.bars.tr1;
    case BAR_LOOK_TR2_PC:
        return &m_Settings.bars.tr2;
    case BAR_LOOK_TR3_PC:
        return &m_Settings.bars.tr3;
    case BAR_LOOK_TR23_PS1:
        return &m_Settings.bars.ps1;
    default:
        return nullptr;
    }
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
    if (obj == nullptr) {
        Shell_ExitSystemFmt("missing '%s' in %s", section, path);
    }
    const char *const style =
        JSON_ObjectGetString(obj, "style", JSON_INVALID_STRING);
    if (style == JSON_INVALID_STRING || !String_Equivalent(style, "pc")) {
        Shell_ExitSystemFmt("invalid '%s.style' in %s", section, path);
    }
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
    if (obj == nullptr) {
        Shell_ExitSystemFmt("missing '%s' in %s", section, path);
    }
    const char *const style =
        JSON_ObjectGetString(obj, "style", JSON_INVALID_STRING);
    if (style == JSON_INVALID_STRING || !String_Equivalent(style, "ps1")) {
        Shell_ExitSystemFmt("invalid '%s.style' in %s", section, path);
    }
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

void UI_Settings_LoadFromFile(const char *const path)
{
    JSON_VALUE *const root =
        JSONFile_ReadEx(path, (JSON_FILE_OPTIONS) { .exit_on_error = true });
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        Shell_ExitSystemFmt("invalid ui settings file: %s", path);
    }
    M_LoadThemesPC(
        JSON_ObjectGetObject(root_obj, "tr1"), &m_Settings.bars.tr1, path,
        "tr1");
    M_LoadThemesPC(
        JSON_ObjectGetObject(root_obj, "tr2"), &m_Settings.bars.tr2, path,
        "tr2");
    M_LoadThemesPC(
        JSON_ObjectGetObject(root_obj, "tr3"), &m_Settings.bars.tr3, path,
        "tr3");
    M_LoadThemesPS1(
        JSON_ObjectGetObject(root_obj, "ps1"), &m_Settings.bars.ps1, path,
        "ps1");
    JSON_ValueFree(root);
}

static const char *M_GetBarColorName(const UI_BAR_TYPE type)
{
    if (type < 0 || type >= UI_BAR_NUMBER_OF) {
        return "gold";
    }

    const bool use_ps1 = g_Config.ui.bar_look == BAR_LOOK_TR23_PS1;
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

bool UI_Settings_CanChangeBarColor(const char *const current, const int32_t dir)
{
    const M_THEME_GROUP *const group = M_GetCurrentBarGroup();
    if (group == nullptr || group->color_count == 0 || dir == 0) {
        return false;
    }
    if (current == nullptr) {
        return true;
    }
    for (int32_t i = 0; i < group->color_count; i++) {
        if (String_Equivalent(group->colors[i].name, current)) {
            const int32_t next = i + dir;
            return next >= 0 && next < group->color_count;
        }
    }
    return true;
}

const char *UI_Settings_GetNextBarColorName(
    const char *const current, const int32_t dir)
{
    const M_THEME_GROUP *const group = M_GetCurrentBarGroup();
    if (group == nullptr || group->color_count == 0 || dir == 0) {
        return nullptr;
    }
    if (current == nullptr) {
        return group->colors[0].name;
    }
    for (int32_t i = 0; i < group->color_count; i++) {
        if (String_Equivalent(group->colors[i].name, current)) {
            const int32_t next = i + dir;
            if (next < 0 || next >= group->color_count) {
                return nullptr;
            }
            return group->colors[next].name;
        }
    }
    return group->colors[0].name;
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
