#include <trx/core/enum_map.h>
#include <trx/core/json/util/file.h>
#include <trx/core/log.h>
#include <trx/core/result.h>
#include <trx/core/subsystem.h>
#include <trx/game/catalog/manager.h>
#include <trx/game/const.h>
#include <trx/game/gun/common.h>
#include <trx/game/gun/registry.h>
#include <trx/game/objects/names.h>
#include <trx/game/paths.h>

#include <string.h>

static RESULT M_ReadAngles(
    JSON_OBJECT *const obj, const char *const name, const char *const path,
    const char *const key, WEAPON_AIM_LIMITS *const limits)
{
    JSON_ARRAY *const arr = JSON_ObjectGetArray(obj, key);
    if (arr == nullptr) {
        return OK;
    }
    FAIL_IF(
        arr->length != 4, "%s: '%s' for '%s' must have four angles", path, key,
        name);
    int16_t *const angles[] = {
        &limits->min_yaw,
        &limits->max_yaw,
        &limits->min_pitch,
        &limits->max_pitch,
    };
    for (size_t i = 0; i < 4; i++) {
        *angles[i] = JSON_ArrayGetInt(arr, i, *angles[i]) * DEG_1;
    }
    return OK;
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

    WEAPON_AMMO_INFO *const ammo = &Gun_Registry_Get(type)->ammo;
    ammo->initial_shots = M_ReadAmmoValue(
        ammo_obj, "initial_shots", "initial_qty", ammo->initial_shots);
    ammo->box_shots =
        M_ReadAmmoValue(ammo_obj, "box_shots", "pickup_qty", ammo->box_shots);
    ammo->box_label_qty = M_ReadAmmoValue(
        ammo_obj, "box_label_qty", "inventory_qty", ammo->box_label_qty);
    ammo->rounds_per_shot = M_ReadAmmoValue(
        ammo_obj, "rounds_per_shot", "rounds_per_shot", ammo->rounds_per_shot);
    ammo->infinite = JSON_ObjectGetBool(ammo_obj, "infinite", ammo->infinite);
}

static SAMPLE_TRX_ID M_ReadSample(
    JSON_OBJECT *const obj, const char *const key, const char *const name,
    const char *const path)
{
    const char *const sample =
        JSON_ObjectGetString(obj, key, JSON_INVALID_STRING);
    if (sample != JSON_INVALID_STRING && sample[0] != '\0') {
        CATALOG_ID sample_id;
        if (!Catalog_NameToEnum(CATALOG_SAMPLES, sample, &sample_id)) {
            LOG_WARNING(
                "unknown sample '%s' for '%s' in %s", sample, name, path);
        } else {
            return sample_id;
        }
    }
    return SFX_TRX_INVALID;
}

static RESULT M_ReadWeapons(JSON_OBJECT *const root_obj, const char *const path)
{
#define L_READ_ANGLE(name, target)                                             \
    target = JSON_ObjectGetInt(obj, name, target) * DEG_1;
#define L_READ_DIST(name, target)                                              \
    target = JSON_ObjectGetDouble(obj, name, target / (float)WALL_L) * WALL_L;
#define L_READ_INT(name, target) target = JSON_ObjectGetInt(obj, name, target)
#define L_READ_OBJECT(name, target)                                            \
    do {                                                                       \
        const char *const key =                                                \
            JSON_ObjectGetString(obj, name, JSON_INVALID_STRING);              \
        if (key != JSON_INVALID_STRING && key[0] != '\0') {                    \
            const OBJECT_ID obj_id = Object_IdFromKey(key);                    \
            FAIL_IF(                                                           \
                obj_id == NO_OBJECT, "%s: unknown object '%s'", path, key);    \
            target = obj_id;                                                   \
        }                                                                      \
    } while (0)

    for (JSON_OBJECT_ELEMENT *elem = root_obj->start; elem != nullptr;
         elem = elem->next) {
        const char *const name = elem->name->string;
        const int32_t type = ENUM_MAP_GET(LARA_GUN_TYPE, name, -1);
        FAIL_IF(
            !Gun_Registry_IsValidType(type), "%s: unknown weapon '%s'", path,
            name);

        JSON_OBJECT *const obj = JSON_ValueAsObject(elem->value);

        // weapon type
        const char *const weapon_type =
            JSON_ObjectGetString(obj, "type", JSON_INVALID_STRING);
        if (weapon_type != JSON_INVALID_STRING && weapon_type[0] != '\0') {
            const int32_t weapon_type_val =
                ENUM_MAP_GET(WEAPON_TYPE, weapon_type, -1);
            FAIL_IF(
                weapon_type_val < 0 || weapon_type_val >= NUM_WEAPON_TYPES,
                "%s: unknown type '%s' for '%s'", path, weapon_type, name);
            Gun_Registry_Get(type)->type = weapon_type_val;
        }

        // angles
        MUST(M_ReadAngles(
            obj, name, path, "lock_angles", &Gun_Registry_Get(type)->lock));
        MUST(M_ReadAngles(
            obj, name, path, "left_angles", &Gun_Registry_Get(type)->left_arm));
        MUST(M_ReadAngles(
            obj, name, path, "right_angles",
            &Gun_Registry_Get(type)->right_arm));

        // scalar properties
        L_READ_ANGLE("aim_speed", Gun_Registry_Get(type)->aim_speed);
        L_READ_ANGLE("shot_accuracy", Gun_Registry_Get(type)->shot_accuracy);
        L_READ_INT("gun_height", Gun_Registry_Get(type)->gun_height);
        L_READ_INT("damage", Gun_Registry_Get(type)->damage);
        L_READ_DIST("target_dist", Gun_Registry_Get(type)->target_dist);
        L_READ_INT(
            "equip_anim_idx", Gun_Registry_Get(type)->anim.equip_anim_idx);
        L_READ_INT("draw_frame", Gun_Registry_Get(type)->anim.draw_frame);
        L_READ_INT("undraw_frame", Gun_Registry_Get(type)->anim.undraw_frame);
        L_READ_INT("recoil_frame", Gun_Registry_Get(type)->anim.recoil_frame);
        L_READ_INT("shell_frame", Gun_Registry_Get(type)->anim.shell_frame);
        L_READ_INT("flash_time", Gun_Registry_Get(type)->flash.time);
        L_READ_INT("flash_shade", Gun_Registry_Get(type)->flash.shade);
        L_READ_INT("smoke_count", Gun_Registry_Get(type)->smoke_count);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "flash_pos"),
            &Gun_Registry_Get(type)->flash.pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "flash_pos_alt"),
            &Gun_Registry_Get(type)->flash.pos.left);
        M_ReadRGB_F(
            JSON_ObjectGetValue(obj, "flash_color"),
            &Gun_Registry_Get(type)->flash.color);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "glow_pos"),
            &Gun_Registry_Get(type)->glow.pos);
        M_ReadRGB_F(
            JSON_ObjectGetValue(obj, "glow_color"),
            &Gun_Registry_Get(type)->glow.color);
        Gun_Registry_Get(type)->glow.scale = JSON_ObjectGetDouble(
            obj, "glow_scale", Gun_Registry_Get(type)->glow.scale);
        Gun_Registry_Get(type)->glow.flicker = JSON_ObjectGetBool(
            obj, "glow_flicker", Gun_Registry_Get(type)->glow.flicker);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "muzzle_pos"),
            &Gun_Registry_Get(type)->muzzle_pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "muzzle_pos_alt"),
            &Gun_Registry_Get(type)->muzzle_pos.left);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "shell_pos"),
            &Gun_Registry_Get(type)->shell_pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "shell_pos_alt"),
            &Gun_Registry_Get(type)->shell_pos.left);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "smoke_pos"),
            &Gun_Registry_Get(type)->smoke_pos.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "smoke_pos_alt"),
            &Gun_Registry_Get(type)->smoke_pos.left);

        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "smoke_tip"),
            &Gun_Registry_Get(type)->smoke_tip.right);
        M_ReadXYZ32(
            JSON_ObjectGetValue(obj, "smoke_tip_alt"),
            &Gun_Registry_Get(type)->smoke_tip.left);

        L_READ_OBJECT("gun_object", Gun_Registry_Get(type)->gun_object_id);
        L_READ_OBJECT("ammo_object", Gun_Registry_Get(type)->ammo_object_id);
        L_READ_OBJECT("anim_object", Gun_Registry_Get(type)->anim_object_id);
        L_READ_OBJECT("shell_object", Gun_Registry_Get(type)->shell_object_id);

        const char *const stow_place =
            JSON_ObjectGetString(obj, "stow_place", JSON_INVALID_STRING);
        if (stow_place != JSON_INVALID_STRING && stow_place[0] != '\0') {
            const int32_t place = ENUM_MAP_GET(STOW_PLACE, stow_place, -1);
            FAIL_IF(
                place < 0, "%s: unknown stow place '%s' for '%s'", path,
                stow_place, name);
            Gun_Registry_Get(type)->stow_place = place;
        }
        Gun_Registry_Get(type)->stow_order = JSON_ObjectGetInt(
            obj, "stow_order", Gun_Registry_Get(type)->stow_order);

        const char *const equip_role =
            JSON_ObjectGetString(obj, "equip_input_role", JSON_INVALID_STRING);
        if (equip_role != JSON_INVALID_STRING && equip_role[0] != '\0') {
            const int32_t role = ENUM_MAP_GET(INPUT_ROLE, equip_role, -1);
            FAIL_IF(
                role < 0, "%s: unknown input role '%s' for '%s'", path,
                equip_role, name);
            Gun_Registry_Get(type)->equip_input_role = role;
        }

        Gun_Registry_Get(type)->unaims_on_release = JSON_ObjectGetBool(
            obj, "unaims_on_release",
            Gun_Registry_Get(type)->unaims_on_release);
        Gun_Registry_Get(type)->flash_lights_room = JSON_ObjectGetBool(
            obj, "flash_lights_room",
            Gun_Registry_Get(type)->flash_lights_room);
        Gun_Registry_Get(type)->flash_is_optional = JSON_ObjectGetBool(
            obj, "flash_is_optional",
            Gun_Registry_Get(type)->flash_is_optional);

        Gun_Registry_Get(type)->shell_throws_forward = JSON_ObjectGetBool(
            obj, "shell_throws_forward",
            Gun_Registry_Get(type)->shell_throws_forward);
        Gun_Registry_Get(type)->shell_angle = JSON_ObjectGetInt(
            obj, "shell_angle", Gun_Registry_Get(type)->shell_angle);
        Gun_Registry_Get(type)->shell_min_speed = JSON_ObjectGetInt(
            obj, "shell_min_speed", Gun_Registry_Get(type)->shell_min_speed);

        M_ReadAmmoInfo(obj, type);

        Gun_Registry_Get(type)->sample_num =
            M_ReadSample(obj, "sample_num", name, path);
        Gun_Registry_Get(type)->sample_overlay_num =
            M_ReadSample(obj, "sample_overlay_num", name, path);
        Gun_Registry_Get(type)->sample_overlay_pitch = JSON_ObjectGetInt(
            obj, "sample_overlay_pitch",
            Gun_Registry_Get(type)->sample_overlay_pitch);

        Gun_Registry_Get(type)->is_available =
            JSON_ObjectGetBool(obj, "is_available", true);
    }

    return OK;
#undef L_READ_ANGLE
#undef L_READ_DIST
#undef L_READ_INT
#undef L_READ_OBJECT
}

static RESULT M_LoadFrom(const char *const path)
{
    JSON_VALUE *root = nullptr;
    MUST(JSONFile_ReadRequired(path, &root));
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(root);
    const RESULT result = root_obj == nullptr
        ? FAIL("%s: the file must hold a dictionary", path)
        : M_ReadWeapons(root_obj, path);
    JSON_ValueFree(root);
    return result;
}

static void M_Init(void)
{
    Gun_Registry_Seed();
}

static RESULT M_Load(void)
{
    const char *path = nullptr;
    RESULT result = GamePath_Resolve(
        GAME_DYNAMIC_PATH_COMMON_CONFIG, "weapons.json5", &path);
    if (IS_OK(result)) {
        result = M_LoadFrom(path);
    }
    return result;
}

REGISTER_SUBSYSTEM(.init = M_Init, .load = M_Load)
