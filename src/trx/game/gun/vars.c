#include <trx/game/gun/vars.h>

#include <trx/core/enum_map.h>
#include <trx/core/json/util/file.h>
#include <trx/core/log.h>
#include <trx/core/subsystem.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/const.h>
#include <trx/game/paths.h>
#include <trx/game/shell/common.h>

#include <string.h>

WEAPON_INFO g_Weapons[NUM_WEAPONS] = {};

static void M_ReadAngles(
    JSON_OBJECT *const obj, const char *const name, const char *const path,
    const char *const key, WEAPON_AIM_LIMITS *const limits)
{
    JSON_ARRAY *const arr = JSON_ObjectGetArray(obj, key);
    if (arr == nullptr) {
        return;
    }
    if (arr->length != 4) {
        Shell_ExitSystemFmt("invalid '%s' for '%s' in %s", key, name, path);
    }
    int16_t *const angles[] = {
        &limits->min_yaw,
        &limits->max_yaw,
        &limits->min_pitch,
        &limits->max_pitch,
    };
    for (size_t i = 0; i < 4; i++) {
        *angles[i] = JSON_ArrayGetInt(arr, i, *angles[i]) * DEG_1;
    }
}

static void M_ReadRGB_F(JSON_VALUE *const value, RGB_F *const target)
{
    JSON_OBJECT *const obj = JSON_ValueAsObject(value);
    if (obj != nullptr) {
        target->r = JSON_ObjectGetDouble(obj, "r", 0.0);
        target->g = JSON_ObjectGetDouble(obj, "g", 0.0);
        target->b = JSON_ObjectGetDouble(obj, "b", 0.0);
    }
    JSON_ARRAY *const arr = JSON_ValueAsArray(value);
    if (arr != nullptr && arr->length == 3) {
        target->r = JSON_ArrayGetDouble(arr, 0, 0.0);
        target->g = JSON_ArrayGetDouble(arr, 1, 0.0);
        target->b = JSON_ArrayGetDouble(arr, 2, 0.0);
    }
}

static void M_ReadXYZ32(JSON_VALUE *const value, XYZ_32 *const target)
{
    JSON_OBJECT *const obj = JSON_ValueAsObject(value);
    if (obj != nullptr) {
        target->x = JSON_ObjectGetInt(obj, "x", 0);
        target->y = JSON_ObjectGetInt(obj, "y", 0);
        target->z = JSON_ObjectGetInt(obj, "z", 0);
    }
    JSON_ARRAY *const arr = JSON_ValueAsArray(value);
    if (arr != nullptr && arr->length == 3) {
        target->x = JSON_ArrayGetInt(arr, 0, 0);
        target->y = JSON_ArrayGetInt(arr, 1, 0);
        target->z = JSON_ArrayGetInt(arr, 2, 0);
    }
}

// The ammunition keys were renamed to say what they count. The names they had
// are still read, so a weapons.json5 written for an earlier version goes on
// working.
// TODO: remove after 1.14
static int32_t M_ReadAmmoValue(
    JSON_OBJECT *const ammo_obj, const char *const key,
    const char *const legacy_key, const int32_t fallback)
{
    return JSON_ObjectGetInt(
        ammo_obj, key, JSON_ObjectGetInt(ammo_obj, legacy_key, fallback));
}

static void M_ReadAmmoInfo(JSON_OBJECT *const obj, const int32_t type)
{
    JSON_OBJECT *const ammo_obj = JSON_ObjectGetObject(obj, "ammo");
    if (ammo_obj == nullptr) {
        return;
    }

    WEAPON_AMMO_INFO *const ammo = &g_Weapons[type].ammo;
    ammo->initial_shots = M_ReadAmmoValue(
        ammo_obj, "initial_shots", "initial_qty", ammo->initial_shots);
    ammo->box_shots =
        M_ReadAmmoValue(ammo_obj, "box_shots", "pickup_qty", ammo->box_shots);
    ammo->box_label_qty = M_ReadAmmoValue(
        ammo_obj, "box_label_qty", "inventory_qty", ammo->box_label_qty);
    ammo->infinite = JSON_ObjectGetBool(ammo_obj, "infinite", ammo->infinite);
}

static void M_Load(void)
{
    const char *const path =
        GamePath_Resolve(GAME_DYNAMIC_PATH_COMMON_CONFIG, "weapons.json5");
#define L_READ_ANGLE(name, target)                                             \
    target = JSON_ObjectGetInt(obj, name, target) * DEG_1;
#define L_READ_DIST(name, target)                                              \
    target = JSON_ObjectGetDouble(obj, name, target / (float)WALL_L) * WALL_L;
#define L_READ_INT(name, target) target = JSON_ObjectGetInt(obj, name, target)

    JSON_VALUE *const root = JSONFile_ReadEx(path, true);
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    if (root_obj == nullptr) {
        Shell_ExitSystemFmt("invalid weapons vars file: %s", path);
    }

    // Every weapon starts from nothing, so that what the file leaves out is
    // absent rather than left over from the mod played before this one.
    memset(g_Weapons, 0, sizeof(g_Weapons));
    for (int32_t i = 0; i < NUM_WEAPONS; i++) {
        g_Weapons[i].glow.scale = 1.0f;
    }

    for (JSON_OBJECT_ELEMENT *elem = root_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_GUN_TYPE, name, -1);
        if (type < 0 || type >= NUM_WEAPONS) {
            Shell_ExitSystemFmt("unknown weapon type '%s' in %s", name, path);
        }

        JSON_OBJECT *const obj = JSON_ValueAsObject(elem->value);

        // weapon type
        const char *const weapon_type =
            JSON_ObjectGetString(obj, "type", JSON_INVALID_STRING);
        if (weapon_type != JSON_INVALID_STRING && weapon_type[0] != '\0') {
            const int32_t weapon_type_val =
                ENUM_MAP_GET(WEAPON_TYPE, weapon_type, -1);
            if (weapon_type_val < 0 || weapon_type_val >= NUM_WEAPON_TYPES) {
                Shell_ExitSystemFmt(
                    "unknown weapon type '%s' in %s", weapon_type, path);
            } else {
                g_Weapons[type].type = weapon_type_val;
            }
        }

        // angles
        M_ReadAngles(obj, name, path, "lock_angles", &g_Weapons[type].lock);
        M_ReadAngles(obj, name, path, "left_angles", &g_Weapons[type].left_arm);
        M_ReadAngles(
            obj, name, path, "right_angles", &g_Weapons[type].right_arm);

        // scalar properties
        L_READ_ANGLE("aim_speed", g_Weapons[type].aim_speed);
        L_READ_ANGLE("shot_accuracy", g_Weapons[type].shot_accuracy);
        L_READ_INT("gun_height", g_Weapons[type].gun_height);
        L_READ_INT("damage", g_Weapons[type].damage);
        L_READ_DIST("target_dist", g_Weapons[type].target_dist);
        L_READ_INT("equip_anim_idx", g_Weapons[type].anim.equip_anim_idx);
        L_READ_INT("draw_frame", g_Weapons[type].anim.draw_frame);
        L_READ_INT("undraw_frame", g_Weapons[type].anim.undraw_frame);
        L_READ_INT("recoil_frame", g_Weapons[type].anim.recoil_frame);
        L_READ_INT("flash_time", g_Weapons[type].flash.time);
        L_READ_INT("flash_shade", g_Weapons[type].flash.shade);
        L_READ_INT("smoke_count", g_Weapons[type].smoke_count);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "flash_pos"),
            &g_Weapons[type].flash.pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "flash_pos_alt"),
            &g_Weapons[type].flash.pos.left);
        M_ReadRGB_F(
            JSON_ObjectGetValue(obj, "flash_color"),
            &g_Weapons[type].flash.color);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "glow_pos"), &g_Weapons[type].glow.pos);
        M_ReadRGB_F(
            JSON_ObjectGetValue(obj, "glow_color"),
            &g_Weapons[type].glow.color);
        g_Weapons[type].glow.scale =
            JSON_ObjectGetDouble(obj, "glow_scale", g_Weapons[type].glow.scale);
        g_Weapons[type].glow.flicker = JSON_ObjectGetBool(
            obj, "glow_flicker", g_Weapons[type].glow.flicker);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "muzzle_pos"),
            &g_Weapons[type].muzzle_pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "muzzle_pos_alt"),
            &g_Weapons[type].muzzle_pos.left);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "shell_pos"),
            &g_Weapons[type].shell_pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "shell_pos_alt"),
            &g_Weapons[type].shell_pos.left);

        M_ReadAmmoInfo(obj, type);

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

        g_Weapons[type].is_available =
            JSON_ObjectGetBool(obj, "is_available", true);
    }

    JSON_ValueFree(root);
#undef L_READ_ANGLE
#undef L_READ_DIST
#undef L_READ_INT
}

REGISTER_SUBSYSTEM(.load = M_Load)
