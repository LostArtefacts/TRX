#include <trx/game/gun/vars.h>

#include <trx/enum_map.h>
#include <trx/game/catalog.h>
#include <trx/game/const.h>
#include <trx/game/shell.h>
#include <trx/json_file.h>
#include <trx/log.h>

WEAPON_INFO g_Weapons[NUM_WEAPONS] = {};

void Gun_LoadVars(const char *const path)
{
    JSON_VALUE *const root =
        JSONFile_ReadEx(path, (JSON_FILE_OPTIONS) { .exit_on_error = true });
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        Shell_ExitSystemFmt("invalid weapons vars file: %s", path);
    }

    for (JSON_OBJECT_ELEMENT *elem = root_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_GUN_TYPE, name, -1);
        if (type < 0 || type >= NUM_WEAPONS) {
            Shell_ExitSystemFmt("unknown weapon type '%s' in %s", name, path);
        }

        JSON_OBJECT *const obj = JSON_ValueAsObject(elem->value);

        // lock_angles
        JSON_ARRAY *arr = JSON_ObjectGetArray(obj, "lock_angles");
        if (arr != nullptr) {
            if (arr->length != 4) {
                Shell_ExitSystemFmt(
                    "invalid 'lock_angles' for '%s' in %s", name, path);
            }
            for (size_t i = 0; i < 4; i++) {
                g_Weapons[type].lock_angles[i] =
                    JSON_ArrayGetInt(arr, i, g_Weapons[type].lock_angles[i])
                    * DEG_1;
            }
        }

        arr = JSON_ObjectGetArray(obj, "left_angles");
        if (arr != nullptr) {
            if (arr->length != 4) {
                Shell_ExitSystemFmt(
                    "invalid 'left_angles' for '%s' in %s", name, path);
            }
            for (size_t i = 0; i < 4; i++) {
                g_Weapons[type].left_angles[i] =
                    JSON_ArrayGetInt(arr, i, g_Weapons[type].left_angles[i])
                    * DEG_1;
            }
        }

        // right_angles
        arr = JSON_ObjectGetArray(obj, "right_angles");
        if (arr != nullptr) {
            if (arr->length != 4) {
                Shell_ExitSystemFmt(
                    "invalid 'right_angles' for '%s' in %s", name, path);
            }
            for (size_t i = 0; i < 4; i++) {
                g_Weapons[type].right_angles[i] =
                    JSON_ArrayGetInt(arr, i, g_Weapons[type].right_angles[i])
                    * DEG_1;
            }
        }

        // scalar properties
        g_Weapons[type].aim_speed =
            JSON_ObjectGetInt(obj, "aim_speed", g_Weapons[type].aim_speed)
            * DEG_1;
        g_Weapons[type].shot_accuracy =
            JSON_ObjectGetInt(
                obj, "shot_accuracy", g_Weapons[type].shot_accuracy)
            * DEG_1;
        g_Weapons[type].gun_height =
            JSON_ObjectGetInt(obj, "gun_height", g_Weapons[type].gun_height);
        g_Weapons[type].damage =
            JSON_ObjectGetInt(obj, "damage", g_Weapons[type].damage);
        float dist = JSON_ObjectGetDouble(
            obj, "target_dist", (float)g_Weapons[type].target_dist / WALL_L);
        g_Weapons[type].target_dist = (int32_t)(dist * WALL_L);
        g_Weapons[type].recoil_frame = JSON_ObjectGetInt(
            obj, "recoil_frame", g_Weapons[type].recoil_frame);
        g_Weapons[type].flash_time =
            JSON_ObjectGetInt(obj, "flash_time", g_Weapons[type].flash_time);
        // sample_num
        const char *const sample =
            JSON_ObjectGetString(obj, "sample_num", JSON_INVALID_STRING);
        CATALOG_ID sample_id;
        if (!Catalog_NameToEnum(CATALOG_SAMPLES, sample, &sample_id)) {
            LOG_WARNING(
                "unknown sample '%s' for '%s' in %s", sample, name, path);
        } else {
            g_Weapons[type].sample_num = sample_id;
        }
    }

    JSON_ValueFree(root);
}
