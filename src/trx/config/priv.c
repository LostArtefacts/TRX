#include <trx/config/priv.h>

#include <trx/config/common.h>
#include <trx/config/file.h>
#include <trx/config/legacy.h>
#include <trx/config/registry.h>
#include <trx/config/section.h>
#include <trx/config/vars.h>

// What an option's bounds cannot say: a setting that takes some values out of a
// range rather than all of them.
static void M_SanitizeSpecial(void)
{
    if (g_Config.rendering.aspect_mode != ASPECT_MODE_ANY
        && g_Config.rendering.aspect_mode != ASPECT_MODE_16_9
        && g_Config.rendering.aspect_mode != ASPECT_MODE_16_10) {
        CONFIG_SET(g_Config.rendering.aspect_mode, ASPECT_MODE_4_3);
    }
    if (g_Config.rendering.fps != 30 && g_Config.rendering.fps != 60) {
        CONFIG_SET(g_Config.rendering.fps, 30);
    }
}

void Config_LoadFromJSON(JSON_OBJECT *root_obj)
{
    for (CONFIG_OPTION *const *option = Config_GetOptions(); *option != nullptr;
         option++) {
        ConfigFile_ApplyFileValueTo(*option);
    }
    for (const CONFIG_SECTION *const *s = Config_Section_GetAll();
         *s != nullptr; s++) {
        (*s)->load(JSON_ObjectGetObject(root_obj, (*s)->key));
    }
    ConfigLegacy_Load(root_obj);
    // Last, because a write on a held option lands on the hold rather than on
    // the player's own value: a migration running after this would carry an
    // older release's setting no further than the hold, and the file would be
    // written without it.
    for (CONFIG_OPTION *const *option = Config_GetOptions(); *option != nullptr;
         option++) {
        ConfigFile_ApplyEnforcedTo(*option);
    }
}

void Config_DumpToJSON(JSON_OBJECT *root_obj)
{
    ConfigFile_DumpOptions(root_obj);
    for (const CONFIG_SECTION *const *s = Config_Section_GetAll();
         *s != nullptr; s++) {
        JSON_OBJECT *const obj = JSON_ObjectNew();
        (*s)->save(obj);
        // A section with nothing to say leaves no key behind, so what another
        // game wrote under it is carried over rather than replaced by a husk.
        if (obj->length > 0) {
            JSON_ObjectAppendObject(root_obj, (*s)->key, obj);
        } else {
            JSON_ObjectFree(obj);
        }
    }
}

void Config_Sanitize(void)
{
    for (CONFIG_OPTION *const *option = Config_GetOptions(); *option != nullptr;
         option++) {
        Config_Option_Sanitize(*option);
    }
    M_SanitizeSpecial();
}
