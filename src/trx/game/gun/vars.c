#include <trx/game/gun/vars.h>

#include <trx/enum_map.h>
#include <trx/game/catalog.h>
#include <trx/game/const.h>
#include <trx/game/shell.h>
#include <trx/json_file.h>
#include <trx/log.h>

WEAPON_INFO g_Weapons[NUM_WEAPONS] = {};

static void M_ReadAngles(
    JSON_OBJECT *const obj, const char *const name, const char *const path,
    const char *const key, int16_t *const angles)
{
    JSON_ARRAY *const arr = JSON_ObjectGetArray(obj, key);
    if (arr == nullptr) {
        return;
    }
    if (arr->length != 4) {
        Shell_ExitSystemFmt("invalid '%s' for '%s' in %s", key, name, path);
    }
    for (size_t i = 0; i < 4; i++) {
        angles[i] = JSON_ArrayGetInt(arr, i, angles[i]) * DEG_1;
    }
}

void Gun_LoadVars(const char *const path)
{
#define L_READ_ANGLE(name, target)                                             \
    target = JSON_ObjectGetInt(obj, name, target) * DEG_1;
#define L_READ_DIST(name, target)                                              \
    target = JSON_ObjectGetDouble(obj, name, target / (float)WALL_L) * WALL_L;
#define L_READ_INT(name, target) target = JSON_ObjectGetInt(obj, name, target)

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

        // angles
        M_ReadAngles(
            obj, name, path, "lock_angles", g_Weapons[type].lock_angles);
        M_ReadAngles(
            obj, name, path, "left_angles", g_Weapons[type].left_angles);
        M_ReadAngles(
            obj, name, path, "right_angles", g_Weapons[type].right_angles);

        // scalar properties
        L_READ_ANGLE("aim_speed", g_Weapons[type].aim_speed);
        L_READ_ANGLE("shot_accuracy", g_Weapons[type].shot_accuracy);
        L_READ_INT("gun_height", g_Weapons[type].gun_height);
        L_READ_INT("damage", g_Weapons[type].damage);
        L_READ_DIST("target_dist", g_Weapons[type].target_dist);
        L_READ_INT("recoil_frame", g_Weapons[type].recoil_frame);
        L_READ_INT("flash_time", g_Weapons[type].flash_time);

        // sample_num
        const char *const sample =
            JSON_ObjectGetString(obj, "sample_num", JSON_INVALID_STRING);
        if (sample != JSON_INVALID_STRING && sample[0] != '\0') {
            CATALOG_ID sample_id;
            if (!Catalog_NameToEnum(CATALOG_SAMPLES, sample, &sample_id)) {
                LOG_WARNING(
                    "unknown sample '%s' for '%s' in %s", sample, name, path);
            } else {
                g_Weapons[type].sample_num = sample_id;
            }
        }
    }

    JSON_ValueFree(root);
#undef L_READ_ANGLE
#undef L_READ_DIST
#undef L_READ_INT
}
