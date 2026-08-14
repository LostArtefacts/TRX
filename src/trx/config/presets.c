#include <trx/config/presets.h>

#include <trx/config/common.h>
#include <trx/config/registry.h>
#include <trx/core/filesystem.h>
#include <trx/core/json.h>
#include <trx/core/json/util/file.h>
#include <trx/core/log.h>
#include <trx/core/memory.h>
#include <trx/core/strings.h>
#include <trx/core/subsystem.h>
#include <trx/core/vector.h>
#include <trx/game/game_strings/entries.h>
#include <trx/game/game_strings/manager.h>
#include <trx/game/paths.h>

#include <stdlib.h>
#include <string.h>

static VECTOR *m_Presets = nullptr; // CONFIG_PRESET
static int32_t m_StringsListener = -1;

static void M_FreePreset(CONFIG_PRESET *const preset)
{
    Memory_FreePointer(&preset->name_gs);
    for (int32_t i = 0; i < preset->setting_count; i++) {
        Memory_FreePointer(&preset->keys[i]);
        Memory_FreePointer(&preset->values[i]);
    }
    Memory_FreePointer(&preset->keys);
    Memory_FreePointer(&preset->values);
}

static void M_FreeAllPresets(void)
{
    if (m_Presets != nullptr) {
        for (int32_t i = 0; i < m_Presets->count; i++) {
            M_FreePreset(Vector_Get(m_Presets, i));
        }
        Vector_Free(m_Presets);
        m_Presets = nullptr;
    }
}

static char *M_SerializeJSONValue(const JSON_VALUE *const val)
{
    if (val == nullptr) {
        return Memory_DupStr("");
    }
    const char *const str = JSON_ValueGetString(val, nullptr);
    if (str != nullptr) {
        return Memory_DupStr(str);
    }
    const int bool_val = JSON_ValueGetBool(val, JSON_INVALID_BOOL);
    if (bool_val != JSON_INVALID_BOOL) {
        return Memory_DupStr(bool_val ? "true" : "false");
    }
    const JSON_NUMBER *const num = JSON_ValueGetNumber(val);
    if (num != nullptr && num->number != nullptr) {
        return Memory_DupStr(num->number);
    }
    return Memory_DupStr("");
}

static bool M_LoadPreset(const char *const path)
{
    JSON_VALUE *const root = JSONFile_ReadEx(path, false);
    if (root == nullptr) {
        LOG_WARNING("Failed to parse preset: %s", path);
        return false;
    }

    const JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        LOG_WARNING("Preset root is not an object: %s", path);
        JSON_ValueFree(root);
        return false;
    }

    const char *const name_gs =
        JSON_ObjectGetString(root_obj, "name_gs", nullptr);
    if (name_gs == nullptr) {
        LOG_WARNING("Preset missing name_gs: %s", path);
        JSON_ValueFree(root);
        return false;
    }

    const JSON_OBJECT *const config_obj =
        JSON_ObjectGetObject(root_obj, "config");
    if (config_obj == nullptr) {
        LOG_WARNING("Preset missing config object: %s", path);
        JSON_ValueFree(root);
        return false;
    }

    // Count entries first
    int32_t count = 0;
    for (const JSON_OBJECT_ELEMENT *elem = config_obj->start; elem != nullptr;
         elem = elem->next) {
        count++;
    }

    CONFIG_PRESET preset = {
        .name_gs = Memory_DupStr(name_gs),
        .setting_count = count,
        .keys = count > 0 ? Memory_Alloc(sizeof(char *) * count) : nullptr,
        .values = count > 0 ? Memory_Alloc(sizeof(char *) * count) : nullptr,
    };

    int32_t i = 0;
    for (const JSON_OBJECT_ELEMENT *elem = config_obj->start; elem != nullptr;
         elem = elem->next, i++) {
        preset.keys[i] = Memory_DupStr(elem->name->string);
        char *const serialized = M_SerializeJSONValue(elem->value);
        const CONFIG_OPTION *const opt = Config_FindOption(preset.keys[i]);
        preset.values[i] =
            Config_Option_NormalizeValueString(opt, serialized, false);
        Memory_Free(serialized);
    }

    Vector_Add(m_Presets, &preset);
    JSON_ValueFree(root);
    return true;
}

static const char *M_GetTitle(const CONFIG_PRESET *const preset)
{
    const char *const title = GameString_Get(preset->name_gs);
    return title != nullptr ? title : preset->name_gs;
}

static int M_CompareByTitle(const void *const a, const void *const b)
{
    return strcmp(M_GetTitle(a), M_GetTitle(b));
}

static void M_HandleLanguageReload(const EVENT *const event, void *const data)
{
    Config_Presets_Sort();
}

static void M_Init(void)
{
    m_StringsListener =
        GameStringManager_SubscribeReload(M_HandleLanguageReload, nullptr);
}

static void M_Shutdown(void)
{
    if (m_StringsListener >= 0) {
        GameStringManager_UnsubscribeReload(m_StringsListener);
        m_StringsListener = -1;
    }
    M_FreeAllPresets();
}

void Config_Presets_ScanFiles(void)
{
    M_FreeAllPresets();

    m_Presets = Vector_Create(sizeof(CONFIG_PRESET));

    char *presets_dir = GamePath_ExpandVars("%mod_dir%/presets");
    if (presets_dir == nullptr || !FS_DirExists(presets_dir)) {
        Memory_FreePointer(&presets_dir);
        presets_dir = GamePath_ExpandVars("%base_mod_dir%/presets");
    }
    if (presets_dir == nullptr || !FS_DirExists(presets_dir)) {
        Memory_FreePointer(&presets_dir);
        presets_dir = GamePath_ExpandVars("%config_dir%/presets");
    }
    if (presets_dir == nullptr || !FS_DirExists(presets_dir)) {
        Memory_FreePointer(&presets_dir);
        return;
    }

    void *const dir = FS_OpenDirectory(presets_dir);
    if (dir == nullptr) {
        Memory_FreePointer(&presets_dir);
        return;
    }

    const char *entry_name;
    while ((entry_name = FS_ReadDirectory(dir)) != nullptr) {
        if (!String_EndsWith(entry_name, ".json5")) {
            continue;
        }
        const char *const full_path =
            String_FormatStatic("%s/%s", presets_dir, entry_name);
        M_LoadPreset(full_path);
    }
    FS_CloseDirectory(dir);
    Memory_FreePointer(&presets_dir);
    Config_Presets_Sort();

    LOG_INFO("Loaded %d config preset(s)", m_Presets->count);
}

void Config_Presets_Sort(void)
{
    if (m_Presets == nullptr || m_Presets->count < 2) {
        return;
    }
    qsort(
        Vector_GetData(m_Presets), m_Presets->count, m_Presets->item_size,
        M_CompareByTitle);
}

int32_t Config_Presets_GetCount(void)
{
    return m_Presets != nullptr ? m_Presets->count : 0;
}

const CONFIG_PRESET *Config_Presets_Get(const int32_t idx)
{
    if (m_Presets == nullptr || idx < 0 || idx >= m_Presets->count) {
        return nullptr;
    }
    return Vector_Get(m_Presets, idx);
}

void Config_Presets_Apply(const int32_t idx)
{
    const CONFIG_PRESET *const preset = Config_Presets_Get(idx);
    if (preset == nullptr) {
        return;
    }
    for (int32_t i = 0; i < preset->setting_count; i++) {
        CONFIG_OPTION *const opt = Config_FindOption(preset->keys[i]);
        if (opt == nullptr) {
            LOG_WARNING("Preset: unknown config key '%s'", preset->keys[i]);
            continue;
        }
        Config_Option_SetFromString(opt, preset->values[i], false);
    }
    Config_Update();
    Config_Write();
}

REGISTER_SUBSYSTEM(
        .init = M_Init, .load = Config_Presets_ScanFiles,
        .shutdown = M_Shutdown)
