#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/carrier.h>
#include <trx/game/effects.h>
#include <trx/game/game.h>
#include <trx/game/game_buf.h>
#include <trx/game/game_flow.h>
#include <trx/game/gun.h>
#include <trx/game/inventory.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/vars.h>
#include <trx/game/objects/vehicles/skidoo_common.h>
#include <trx/game/output.h>
#include <trx/game/pathing.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/stats.h>
#include <trx/game/weather_fx.h>
#include <trx/memory.h>
#include <trx/strings.h>
#include <trx/version.h>

#include <stdio.h>
#include <string.h>

#define M_NO_ROOM_LEGACY 255

#define M_MAX_STACK_SIZE 10
#define M_SHOULD(x) ((x) ? 1 : (LOG_WARNING("%s", ctx->error_msg), 0))
#define M_OPTIONAL(x) (void)(x);
#define M_MUST(x)                                                              \
    if (!(x)) {                                                                \
        goto fail;                                                             \
    }
#define M_FAIL() goto fail;
#define M_FINISH()                                                             \
    do {                                                                       \
    success:                                                                   \
        return true;                                                           \
    fail:                                                                      \
        return false;                                                          \
    } while (0);

typedef struct SAVEGAME_BSON_READ_CONTEXT {
    char path[256];
    int32_t path_index_stack[M_MAX_STACK_SIZE];
    int32_t path_top;
    char error_msg[256];
    JSON_VALUE *stack[M_MAX_STACK_SIZE];
    JSON_VALUE *current;
    size_t current_pos;
    uint16_t sg_version;
} SAVEGAME_BSON_READ_CONTEXT;

typedef SAVEGAME_BSON_READ_CONTEXT M_CONTEXT;

// =============================================================================
// Start of internal helpers
// =============================================================================

static void M_SetError(M_CONTEXT *const ctx, const char *fmt, ...)
{
    char body[256];
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(body, sizeof(body), fmt, ap);
    va_end(ap);
    if (ctx && ctx->path[0] != '\0') {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
        snprintf(
            ctx->error_msg, sizeof(ctx->error_msg), "%s: %s", ctx->path, body);
#pragma GCC diagnostic pop
    } else {
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), "%s", body);
    }
}

static bool M_PushPathKey(M_CONTEXT *const ctx, const char *const key)
{
    if (ctx->path_top + 1 >= M_MAX_STACK_SIZE) {
        return false;
    }
    const size_t pos = strlen(ctx->path);
    ctx->path_index_stack[ctx->path_top++] = pos;
    if (pos != 0) {
        strncat(ctx->path, ".", sizeof(ctx->path) - strlen(ctx->path) - 1);
    }
    strncat(ctx->path, key, sizeof(ctx->path) - strlen(ctx->path) - 1);
    return true;
}

static bool M_PushPathIndex(M_CONTEXT *const ctx, const size_t idx)
{
    if (ctx->path_top + 1 >= M_MAX_STACK_SIZE) {
        return false;
    }
    ctx->path_index_stack[ctx->path_top++] = strlen(ctx->path);
    char tmp[32];
    snprintf(tmp, sizeof(tmp), "[%zu]", idx);
    strncat(ctx->path, tmp, sizeof(ctx->path) - strlen(ctx->path) - 1);
    return true;
}

static void M_PopPath(M_CONTEXT *const ctx)
{
    if (ctx->path_top <= 0) {
        ctx->path[0] = '\0';
        ctx->path_top = 0;
        return;
    }
    int pos = ctx->path_index_stack[--ctx->path_top];
    ctx->path[pos] = '\0';
}

static bool M_PushValue(M_CONTEXT *const ctx, JSON_VALUE *const value)
{
    if (value == nullptr) {
        M_SetError(ctx, "pushing null value");
        return false;
    }
    if (ctx->current_pos + 1 >= M_MAX_STACK_SIZE) {
        M_SetError(ctx, "stack overflow");
        return false;
    }
    ctx->current_pos++;
    ctx->stack[ctx->current_pos] = value;
    ctx->current = ctx->stack[ctx->current_pos];
    return true;
}

static bool M_Pop(M_CONTEXT *const ctx)
{
    if (ctx->current_pos == 0) {
        M_SetError(ctx, "pop from empty stack");
        return false;
    }
    ctx->current_pos--;
    ctx->current = ctx->stack[ctx->current_pos];
    M_PopPath(ctx);
    return true;
}

static bool M_PushObject(M_CONTEXT *const ctx, const char *const key)
{
    JSON_OBJECT *const obj = JSON_ValueAsObject(ctx->current);
    if (obj == nullptr) {
        M_SetError(ctx, "not an object");
        return false;
    }
    if (!JSON_ObjectContainsKey(obj, key)) {
        M_SetError(ctx, "key does not exist: %s", key);
        return false;
    }
    if (!M_PushPathKey(ctx, key)) {
        M_SetError(ctx, "path depth overflow");
        return false;
    }
    if (!M_PushValue(ctx, JSON_ObjectGetValue(obj, key))) {
        M_PopPath(ctx);
        return false;
    }
    return true;
}

static bool M_PushArrayElem(M_CONTEXT *const ctx, const size_t i)
{
    JSON_ARRAY *const arr = JSON_ValueAsArray(ctx->current);
    if (arr == nullptr) {
        M_SetError(ctx, "not an array");
        return false;
    }
    if (i >= arr->length) {
        M_SetError(ctx, "invalid array index");
        return false;
    }
    if (!M_PushPathIndex(ctx, (int)i)) {
        M_SetError(ctx, "path depth overflow");
        return false;
    }
    if (!M_PushValue(ctx, JSON_ArrayGetValue(arr, i))) {
        M_PopPath(ctx);
        return false;
    }
    return true;
}

static int32_t M_HasKey(M_CONTEXT *const ctx, const char *const key)
{
    JSON_OBJECT *const obj = JSON_ValueAsObject(ctx->current);
    if (obj == nullptr) {
        return false;
    }
    return JSON_ObjectContainsKey(obj, key);
}

static int32_t M_GetArrayLength(M_CONTEXT *const ctx)
{
    JSON_ARRAY *const arr = JSON_ValueAsArray(ctx->current);
    if (arr == nullptr) {
        M_SetError(ctx, "not an array");
        return false;
    }
    return arr->length;
}

static bool M_ReadBoolDirect(M_CONTEXT *const ctx, bool *const target)
{
    if (JSON_ValueIsTrue(ctx->current)) {
        *target = true;
        return true;
    } else if (JSON_ValueIsFalse(ctx->current)) {
        *target = false;
        return true;
    } else {
        // TR1X <4.16, TR2X <1.6
        const int32_t int_val = JSON_ValueGetInt(ctx->current, -1);
        if (int_val == 1) {
            *target = true;
            return true;
        } else if (int_val == 0) {
            *target = false;
            return true;
        } else {
            M_SetError(ctx, "not a bool");
            return false;
        }
    }
}

static bool M_ReadBool(
    M_CONTEXT *const ctx, const char *key, bool *const target)
{
    M_MUST(M_PushObject(ctx, key));
    M_MUST(M_ReadBoolDirect(ctx, target));
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

#define L_DEFINE_M_READ_NUM_DIRECT(type_, name, minv, maxv)                    \
    static bool M_ReadNumDirect_##name(                                        \
        M_CONTEXT *const ctx, void *const target)                              \
    {                                                                          \
        if (ctx->current->type != JSON_TYPE_NUMBER) {                          \
            M_SetError(ctx, "not a number");                                   \
            return false;                                                      \
        }                                                                      \
        const long long val = JSON_ValueGetInt(ctx->current, 0);               \
        if (val < (long long)(minv) || val > (long long)(maxv)) {              \
            M_SetError(ctx, "value out of range: %lld", val);                  \
            return false;                                                      \
        }                                                                      \
        *(type_ *)target = (type_)val;                                         \
        return true;                                                           \
    }
L_DEFINE_M_READ_NUM_DIRECT(int8_t, S8, INT8_MIN, INT8_MAX)
L_DEFINE_M_READ_NUM_DIRECT(int16_t, S16, INT16_MIN, INT16_MAX)
L_DEFINE_M_READ_NUM_DIRECT(int32_t, S32, INT32_MIN, INT32_MAX)
L_DEFINE_M_READ_NUM_DIRECT(uint8_t, U8, 0, UINT8_MAX)
L_DEFINE_M_READ_NUM_DIRECT(uint16_t, U16, 0, UINT16_MAX)
L_DEFINE_M_READ_NUM_DIRECT(uint32_t, U32, 0, UINT32_MAX)
#undef L_DEFINE_M_READ_NUM_DIRECT

static bool M_ReadNumDirect_Double(M_CONTEXT *const ctx, double *const target)
{
    if (ctx->current->type != JSON_TYPE_NUMBER) {
        M_SetError(ctx, "not a number");
        return false;
    }
    const double val = JSON_ValueGetDouble(ctx->current, -1.0);
    *(double *)target = val;
    return true;
}

#define L_DEFINE_M_READ_NUM(type, name)                                        \
    static bool M_ReadNum_##name(                                              \
        M_CONTEXT *const ctx, const char *key, void *const target)             \
    {                                                                          \
        M_MUST(M_PushObject(ctx, key));                                        \
        M_MUST(M_ReadNumDirect_##name(ctx, target));                           \
        M_MUST(M_Pop(ctx));                                                    \
        M_FINISH();                                                            \
    }
L_DEFINE_M_READ_NUM(int8_t, S8)
L_DEFINE_M_READ_NUM(int16_t, S16)
L_DEFINE_M_READ_NUM(int32_t, S32)
L_DEFINE_M_READ_NUM(uint8_t, U8)
L_DEFINE_M_READ_NUM(uint16_t, U16)
L_DEFINE_M_READ_NUM(uint32_t, U32)
L_DEFINE_M_READ_NUM(double, Double)
#undef L_DEFINE_M_READ_NUM

#define M_ReadNumDirect(ctx, target_ptr)                                       \
    _Generic(                                                                  \
        *(target_ptr),                                                         \
        int8_t: M_ReadNumDirect_S8,                                            \
        uint8_t: M_ReadNumDirect_U8,                                           \
        int16_t: M_ReadNumDirect_S16,                                          \
        uint16_t: M_ReadNumDirect_U16,                                         \
        int32_t: M_ReadNumDirect_S32,                                          \
        uint32_t: M_ReadNumDirect_U32,                                         \
        double: M_ReadNumDirect_Double)(ctx, (void *)(target_ptr))

#define M_ReadNum(ctx, key, target_ptr)                                        \
    _Generic(                                                                  \
        *(target_ptr),                                                         \
        int8_t: M_ReadNum_S8,                                                  \
        uint8_t: M_ReadNum_U8,                                                 \
        int16_t: M_ReadNum_S16,                                                \
        uint16_t: M_ReadNum_U16,                                               \
        int32_t: M_ReadNum_S32,                                                \
        uint32_t: M_ReadNum_U32,                                               \
        double: M_ReadNum_Double)(ctx, key, (void *)(target_ptr))

// =============================================================================
// End of internal helpers
// =============================================================================

static bool M_ReadXYZ32(
    M_CONTEXT *const ctx, const char *const key, XYZ_32 *const target)
{
    ASSERT(target != nullptr);
    M_MUST(M_PushObject(ctx, key));
    M_MUST(M_ReadNum(ctx, "x", &target->x));
    M_MUST(M_ReadNum(ctx, "y", &target->y));
    M_MUST(M_ReadNum(ctx, "z", &target->z));
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

static bool M_ReadXYZ16(
    M_CONTEXT *const ctx, const char *const key, XYZ_16 *const target)
{
    ASSERT(target != nullptr);
    M_MUST(M_PushObject(ctx, key));
    M_MUST(M_ReadNum(ctx, "x", &target->x));
    M_MUST(M_ReadNum(ctx, "y", &target->y));
    M_MUST(M_ReadNum(ctx, "z", &target->z));
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

static bool M_ReadPos(M_CONTEXT *const ctx, XYZ_32 *const target)
{
    ASSERT(target != nullptr);
    if (M_HasKey(ctx, "x")) {
        // TR1X <4.16
        M_MUST(M_ReadNum(ctx, "x", &target->x));
        M_MUST(M_ReadNum(ctx, "y", &target->y));
        M_MUST(M_ReadNum(ctx, "z", &target->z));
    } else {
        M_MUST(M_ReadXYZ32(ctx, "pos", target));
    }
    M_FINISH();
}

static bool M_ReadRot(M_CONTEXT *const ctx, XYZ_16 *const target)
{
    ASSERT(target != nullptr);
    if (M_HasKey(ctx, "x_rot")) {
        // TR1X <v4.16
        M_MUST(M_ReadNum(ctx, "x_rot", &target->x));
        M_MUST(M_ReadNum(ctx, "y_rot", &target->y));
        M_MUST(M_ReadNum(ctx, "z_rot", &target->z));
    } else {
        M_MUST(M_ReadXYZ16(ctx, "rot", target));
    }
    M_FINISH();
}

static bool M_ReadObjectID(
    M_CONTEXT *const ctx, const char *const key, OBJECT_ID *const target)
{
    int32_t game_id = 0;
    M_MUST(M_ReadNum(ctx, key, &game_id));
    *target = Object_FromGameID(game_id);
    M_FINISH();
}

static bool M_ReadArm(
    M_CONTEXT *const ctx, const char *const key, LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    M_MUST(M_PushObject(ctx, key));
    // TR1X <4.16
    M_OPTIONAL(M_ReadNum(ctx, "anim_num", &arm->anim_num));
    M_MUST(M_ReadNum(ctx, "frame_num", &arm->frame_num));
    M_MUST(M_ReadNum(ctx, "lock", &arm->lock));
    M_MUST(M_ReadNum(ctx, "flash_gun", &arm->flash_gun));
    M_MUST(M_ReadRot(ctx, &arm->rot));
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

static bool M_ReadAmmo(
    M_CONTEXT *const ctx, const char *const key, AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    M_MUST(M_PushObject(ctx, key));
    M_MUST(M_ReadNum(ctx, "ammo", &ammo->ammo));
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

static bool M_ReadLara(M_CONTEXT *const ctx)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    M_MUST(M_ReadNum(ctx, "item_number", &lara->item_num));
    M_MUST(M_ReadNum(ctx, "gun_status", &lara->gun_status));
    M_MUST(M_ReadNum(ctx, "gun_type", &lara->gun_type));
    M_MUST(M_ReadNum(ctx, "request_gun_type", &lara->request_gun_type));

    // TRX <1.1
    if (g_TRVersion == 2 && ctx->sg_version < VERSION_14) {
        if (lara->gun_type == LGT_MAGNUMS) {
            lara->gun_type = LGT_AUTOS;
        }
        if (lara->request_gun_type == LGT_MAGNUMS) {
            lara->request_gun_type = LGT_AUTOS;
        }
    }

    // TR1X <4.12
    if (M_HasKey(ctx, "last_gun_type")) {
        M_MUST(M_ReadNum(ctx, "last_gun_type", &lara->last_gun_type));
    } else {
        lara->last_gun_type = lara->request_gun_type;
    }
    // Only TR1X >= 4.16, TR2X >=* have new back-gun support
    M_OPTIONAL(M_ReadObjectID(ctx, "back_gun_obj_id", &lara->back_gun_obj_id));
    M_MUST(M_ReadNum(ctx, "calc_fall_speed", &lara->calc_fall_speed));
    M_MUST(M_ReadNum(ctx, "water_status", &lara->water_status));
    // Only TR1X >= 4.15, TR2X >=* has ladder support
    M_OPTIONAL(M_ReadBool(ctx, "climb_status", &lara->climb_status));
    M_OPTIONAL(M_ReadBool(ctx, "is_crouched", &lara->is_crouched));
    M_OPTIONAL(M_ReadBool(ctx, "keep_crouched", &lara->keep_crouched));
    M_MUST(M_ReadNum(ctx, "pose_count", &lara->pose_count));
    M_MUST(M_ReadNum(ctx, "hit_frame", &lara->hit_frame));
    M_MUST(M_ReadNum(ctx, "hit_direction", &lara->hit_direction));
    M_MUST(M_ReadNum(ctx, "air", &lara->air));
    // Only TR1X >=4.14, TR2X >=1.4 have sprint function
    M_OPTIONAL(M_ReadNum(ctx, "sprint_timer", &lara->sprint_timer));
    // Only TR1X >=4.16, TR2X >=1.6 have exposure bar function
    M_OPTIONAL(M_ReadNum(ctx, "exposure_timer", &lara->exposure_timer));
    M_OPTIONAL(M_ReadNum(ctx, "poison_timer", &lara->poison_timer));
    M_MUST(M_ReadNum(ctx, "dive_count", &lara->dive_timer));
    M_MUST(M_ReadNum(ctx, "death_count", &lara->death_timer));
    M_MUST(M_ReadNum(ctx, "current_active", &lara->current.active));
    M_OPTIONAL(M_ReadNum(ctx, "current_vel_x", &lara->current.vel.x));
    M_OPTIONAL(M_ReadNum(ctx, "current_vel_z", &lara->current.vel.z));
    // Only TR1X >=4.12, TR2X >=* have burn flag
    M_OPTIONAL(M_ReadBool(ctx, "burn", &lara->burn));

    M_MUST(M_ReadNum(ctx, "mesh_effects", &lara->mesh_effects));
    M_OPTIONAL(M_ReadBool(ctx, "extra_anim", &lara->extra_anim));
    M_OPTIONAL(M_ReadNum(ctx, "water_surface_dist", &lara->water_surface_dist));

    // Only TR1X >=4.8, TR2X >=* stores hit effect count information
    M_OPTIONAL(M_ReadNum(ctx, "hit_effect_count", &lara->hit_effect_count));
    // Only TR1X >=4.8, TR2X >=- stores hit effect information
    int16_t hit_effect = NO_EFFECT;
    M_OPTIONAL(M_ReadNum(ctx, "hit_effect", &hit_effect));
    lara->hit_effect =
        hit_effect != NO_EFFECT && g_Config.gameplay.enable_enhanced_saves
        ? Effect_Get(hit_effect)
        : nullptr;

    const int16_t vehicle_idx = Lara_Vehicle_GetIndex();
    // Only TR1X >=4.16, TR2X >=* stores vehicle information
    M_OPTIONAL(M_ReadNum(ctx, "vehicle_item_number", &vehicle_idx));
    Lara_Vehicle_SetIndex(vehicle_idx);

    // Only TR1X >=4.16, TR2X >=* stores flares information
    M_OPTIONAL(M_ReadNum(ctx, "flare_age", &lara->flare.age));
    M_OPTIONAL(M_ReadNum(ctx, "flare_frame", &lara->flare.frame_num));
    M_OPTIONAL(M_ReadBool(ctx, "flare_control_left", &lara->flare.control));

    M_MUST(M_PushObject(ctx, "meshes"));
    const int32_t mesh_count = M_GetArrayLength(ctx);
    if (mesh_count != LM_NUMBER_OF) {
        M_SetError(
            ctx, "expected %d Lara meshes, got %d", LM_NUMBER_OF, mesh_count);
        M_FAIL();
    }
    for (int32_t i = 0; i < mesh_count; i++) {
        M_MUST(M_PushArrayElem(ctx, i));
        int32_t idx = Object_GetMeshOffset(lara->mesh_ptrs[i]);
        M_OPTIONAL(M_ReadNumDirect(ctx, &idx));
        OBJECT_MESH *const mesh = Object_FindMesh(idx);
        if (mesh != nullptr) {
            lara->mesh_ptrs[i] = mesh;
        } else {
            LOG_WARNING("can't find mesh %d", idx);
        }
        M_MUST(M_Pop(ctx));
    }
    M_MUST(M_Pop(ctx));

    lara->target = nullptr;
    M_MUST(M_ReadNum(ctx, "target_angle1", &lara->target_angles[0]));
    M_MUST(M_ReadNum(ctx, "target_angle2", &lara->target_angles[1]));
    M_MUST(M_ReadNum(ctx, "turn_rate", &lara->turn_rate));
    M_MUST(M_ReadNum(ctx, "move_angle", &lara->move_angle));
    if (M_HasKey(ctx, "head_rot.y")) {
        // TR1X <4.16
        M_MUST(M_ReadNum(ctx, "head_rot.y", &lara->head_rot.y));
        M_MUST(M_ReadNum(ctx, "head_rot.x", &lara->head_rot.x));
        M_MUST(M_ReadNum(ctx, "head_rot.z", &lara->head_rot.z));
        M_MUST(M_ReadNum(ctx, "torso_rot.y", &lara->torso_rot.y));
        M_MUST(M_ReadNum(ctx, "torso_rot.x", &lara->torso_rot.x));
        M_MUST(M_ReadNum(ctx, "torso_rot.z", &lara->torso_rot.z));
    } else {
        M_MUST(M_ReadXYZ16(ctx, "head_rot", &lara->head_rot));
        M_MUST(M_ReadXYZ16(ctx, "torso_rot", &lara->torso_rot));
    }

    if (g_TRVersion == 1) {
        if (ctx->sg_version >= VERSION_7) {
            // TR1X > 4.8
            M_MUST(M_ReadNum(ctx, "last_pos.x", &lara->last_pos.x));
            M_MUST(M_ReadNum(ctx, "last_pos.y", &lara->last_pos.y));
            M_MUST(M_ReadNum(ctx, "last_pos.z", &lara->last_pos.z));
        }
    } else {
        M_MUST(M_ReadXYZ32(ctx, "last_pos", &lara->last_pos));
    }

    M_MUST(M_ReadArm(ctx, "left_arm", &lara->left_arm));
    M_MUST(M_ReadArm(ctx, "right_arm", &lara->right_arm));
    M_MUST(M_ReadAmmo(ctx, "pistols", &lara->pistol_ammo));
    M_MUST(M_ReadAmmo(ctx, "magnums", &lara->magnum_ammo));
    M_MUST(M_ReadAmmo(ctx, "uzis", &lara->uzi_ammo));
    M_MUST(M_ReadAmmo(ctx, "shotgun", &lara->shotgun_ammo));
    // TR1X <4.16
    M_OPTIONAL(M_ReadAmmo(ctx, "harpoon", &lara->harpoon_ammo));
    M_OPTIONAL(M_ReadAmmo(ctx, "grenade", &lara->grenade_ammo));
    M_OPTIONAL(M_ReadAmmo(ctx, "m16", &lara->m16_ammo));

    // TRX <1.1
    M_OPTIONAL(M_ReadAmmo(ctx, "autos", &lara->autos_ammo));
    M_OPTIONAL(M_ReadAmmo(ctx, "desert_eagle", &lara->desert_eagle_ammo));
    M_OPTIONAL(M_ReadAmmo(ctx, "mp5", &lara->mp5_ammo));
    M_OPTIONAL(M_ReadAmmo(ctx, "rocket", &lara->rocket_ammo));

    if (g_TRVersion == 1 && ctx->sg_version < VERSION_13) {
        const bool has_rifle = Inv_RequestItem(O_SHOTGUN_ITEM) != 0;
        Gun_Rifle_LoadLegacy(has_rifle);
    } else if (M_HasKey(ctx, "weapon")) {
        M_MUST(M_PushObject(ctx, "weapon"));
        lara->gun_item_num = Item_Create();
        ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        M_MUST(M_ReadObjectID(ctx, "obj_id", &weapon_item->object_id));
        M_MUST(M_ReadNum(ctx, "anim_num", &weapon_item->anim_num));
        M_MUST(M_ReadNum(ctx, "frame_num", &weapon_item->frame_num));
        M_MUST(M_ReadNum(
            ctx, "current_anim_state", &weapon_item->current_anim_state));
        M_MUST(
            M_ReadNum(ctx, "goal_anim_state", &weapon_item->goal_anim_state));
        weapon_item->status = IS_ACTIVE;
        weapon_item->room_num = NO_ROOM;
        M_MUST(M_Pop(ctx));
    }

    if (M_HasKey(ctx, "interact_target.item_num")) {
        // TR1X >4.4, TR2X >1.2
        M_MUST(M_ReadNum(
            ctx, "interact_target.item_num", &lara->interact_target.item_num));
        M_MUST(M_ReadNum(
            ctx, "interact_target.move_count",
            &lara->interact_target.move_count));
        M_MUST(M_ReadBool(
            ctx, "interact_target.is_moving",
            &lara->interact_target.is_moving));
    }

    M_FINISH();
}

static bool M_IsValidItemObject(
    const OBJECT_ID saved_obj_id, const OBJECT_ID initial_obj_id)
{
    if (saved_obj_id == initial_obj_id) {
        return true;
    }
    if (Object_IsType(initial_obj_id, g_GunObjects)
        && Object_IsType(saved_obj_id, g_GunObjects)) {
        return true;
    }

    // clang-format off
    switch (saved_obj_id) {
        // used keyholes
        case O_PUZZLE_DONE_1: return initial_obj_id == O_PUZZLE_HOLE_1;
        case O_PUZZLE_DONE_2: return initial_obj_id == O_PUZZLE_HOLE_2;
        case O_PUZZLE_DONE_3: return initial_obj_id == O_PUZZLE_HOLE_3;
        case O_PUZZLE_DONE_4: return initial_obj_id == O_PUZZLE_HOLE_4;
        // pickups
        case O_PISTOL_AMMO_ITEM: return initial_obj_id == O_PISTOL_ITEM;
        case O_SHOTGUN_AMMO_ITEM: return initial_obj_id == O_SHOTGUN_ITEM;
        case O_MAGNUM_AMMO_ITEM: return initial_obj_id == O_MAGNUM_ITEM;
        case O_AUTOS_AMMO_ITEM: return initial_obj_id == O_AUTOS_ITEM;
        case O_DESERT_EAGLE_AMMO_ITEM: return initial_obj_id == O_DESERT_EAGLE_ITEM;
        case O_UZI_AMMO_ITEM: return initial_obj_id == O_UZI_ITEM;
        case O_HARPOON_AMMO_ITEM: return initial_obj_id == O_HARPOON_ITEM;
        case O_M16_AMMO_ITEM: return initial_obj_id == O_M16_ITEM;
        case O_MP5_AMMO_ITEM: return initial_obj_id == O_MP5_ITEM;
        case O_GRENADE_AMMO_ITEM: return initial_obj_id == O_GRENADE_GUN_ITEM;
        case O_ROCKET_AMMO_ITEM: return initial_obj_id == O_ROCKET_GUN_ITEM;
        // dual-state animals
        case O_ALLIGATOR: return initial_obj_id == O_CROCODILE;
        case O_CROCODILE: return initial_obj_id == O_ALLIGATOR;
        case O_RAT: return initial_obj_id == O_VOLE;
        case O_VOLE: return initial_obj_id == O_RAT;
        // skidoo swaps
        case O_SKIDOO_FAST: return initial_obj_id == O_SKIDOO_ARMED;
        // default
        default: return false;
    }
    // clang-format on
}

static bool M_ReadItem(
    SAVEGAME_BSON_READ_CONTEXT *const ctx, const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    int16_t game_object_id = -1;
    if (!M_ReadNum(ctx, "object_id", &game_object_id)) {
        // TR1X <4.16, TR2X <1.6
        M_MUST(M_ReadNum(ctx, "obj_num", &game_object_id));
    }
    const OBJECT_ID object_id = Object_FromGameID(game_object_id);
    if (object_id == NO_OBJECT) {
        item->object_id = O_DUMMY;
        LOG_ERROR("Unsupported object #%d", game_object_id);
        return true;
    }

    const OBJECT *const obj = Object_Get(object_id);
    item->object_id = object_id;
    if (!M_IsValidItemObject(object_id, item->object_id)) {
        M_SetError(
            ctx, "level has %d (%s), save has %d (%s)", item->object_id,
            Object_GetName(item->object_id), object_id,
            Object_GetName(object_id));
        M_FAIL();
    }

    // Not sure why some items do not have their their position saved,
    // despite OBJECT telling them to.
    if (obj->save_position && M_HasKey(ctx, "room_num")) {
        M_MUST(M_ReadPos(ctx, &item->pos));
        M_MUST(M_ReadRot(ctx, &item->rot));
        M_MUST(M_ReadNum(ctx, "speed", &item->speed));
        M_MUST(M_ReadNum(ctx, "fall_speed", &item->fall_speed));
        int16_t room_num = NO_ROOM;
        M_MUST(M_ReadNum(ctx, "room_num", &room_num));
        if (room_num != NO_ROOM) {
            Item_UpdateRoom(item_num, room_num);
        }
    }

    if (obj->save_anim) {
        // TRX >= 1.1 animated puzzle holes became animated
        M_SHOULD(M_ReadNum(ctx, "current_anim", &item->current_anim_state));
        M_SHOULD(M_ReadNum(ctx, "goal_anim", &item->goal_anim_state));
        M_SHOULD(M_ReadNum(ctx, "required_anim", &item->required_anim_state));
        M_SHOULD(M_ReadNum(ctx, "anim_num", &item->anim_num));
        M_SHOULD(M_ReadNum(ctx, "frame_num", &item->frame_num));
        // TRX >= 1.0 stores previous frame for interpolation checks
        M_OPTIONAL(M_ReadNum(ctx, "prev_frame_num", &item->prev_frame_num));

        // Prevent issues with pre-injection saves and Lara's enhanced
        // animation set.
        if (item->object_id == O_LARA
            && item->anim_num < LARA_ORIGINAL_ANIM_COUNT) {
            item->anim_num += obj->anim_idx;
        }
    }

    if (obj->save_hitpoints) {
        M_SHOULD(M_ReadNum(ctx, "hitpoints", &item->hit_points));
        // TR1X >= 4.16, TR2X >=1.6 store max hit points information
        M_OPTIONAL(M_ReadNum(ctx, "max_hitpoints", &item->max_hit_points));
    }

    if (obj->save_flags) {
        M_MUST(M_ReadNum(ctx, "flags", &item->flags));
        M_MUST(M_ReadNum(ctx, "timer", &item->timer));
        ITEM_STATUS saved_status = item->status;
        M_OPTIONAL(M_ReadNum(ctx, "status", &saved_status));

        if ((item->flags & IF_KILLED) != 0) {
            Item_Kill(item_num);
            item->status = saved_status;
        } else {
            bool is_active;
            M_MUST(M_ReadBool(ctx, "active", &is_active));
            if (is_active && !item->active) {
                Item_AddActive(item_num);
            }
            item->status = saved_status;
            M_MUST(M_ReadBool(ctx, "gravity", &item->gravity));
            M_OPTIONAL(M_ReadBool(ctx, "collidable", &item->collidable));
        }
        M_OPTIONAL(M_ReadNum(ctx, "ai_bits", &item->ai_bits));
        M_OPTIONAL(M_ReadNum(ctx, "ai_tag", &item->ai_tag));

        bool intelligent = obj->intelligent;
        M_OPTIONAL(M_ReadBool(ctx, "intelligent", &intelligent));
        if (intelligent) {
            LOT_EnableBaddieAI(item_num, true);
            CREATURE *const creature = item->data;
            if (creature != nullptr) {
                M_MUST(M_ReadNum(ctx, "head_rot", &creature->head_rotation));
                M_MUST(M_ReadNum(ctx, "neck_rot", &creature->neck_rotation));
                M_MUST(M_ReadNum(ctx, "max_turn", &creature->maximum_turn));
                M_MUST(M_ReadNum(ctx, "creature_flags", &creature->flags));
                M_MUST(M_ReadNum(ctx, "creature_mood", &creature->mood));
                if (M_SHOULD(M_PushObject(ctx, "creature"))) {
                    M_MUST(M_ReadBool(ctx, "alerted", &creature->alerted));
                    M_MUST(M_ReadBool(ctx, "head_left", &creature->head_left));
                    M_MUST(
                        M_ReadBool(ctx, "head_right", &creature->head_right));
                    M_MUST(M_ReadBool(
                        ctx, "reached_goal", &creature->reached_goal));
                    M_MUST(M_ReadBool(ctx, "patrol_2", &creature->patrol_2));
                    M_MUST(M_ReadBool(
                        ctx, "hurt_by_lara", &creature->hurt_by_lara));
                    M_SHOULD(M_ReadNum(
                        ctx, "damage_from_lara", &creature->damage_from_lara));
                    M_MUST(M_PushObject(ctx, "joint_rotations"));
                    for (int32_t i = 0; i < 4; i++) {
                        M_MUST(M_PushArrayElem(ctx, i));
                        M_OPTIONAL(
                            M_ReadNumDirect(ctx, &creature->joint_rotation[i]));
                        M_MUST(M_Pop(ctx));
                    }
                    M_MUST(M_Pop(ctx));
                    M_MUST(M_Pop(ctx));
                }
            }
        } else if (obj->intelligent) {
            item->data = nullptr;
            if (item->clear_body && item->hit_points <= 0
                && (item->flags & IF_KILLED) == 0) {
                item->next_active = Item_GetPrevActive();
                Item_SetPrevActive(item_num);
            }
        }
    }

    if (M_HasKey(ctx, "carried_items")) {
        M_MUST(M_PushObject(ctx, "carried_items"));
        CARRIED_ITEM *carried_item = item->carried_item;
        CARRIED_ITEM *prev_item = nullptr;
        for (int32_t j = 0;; j++) {
            if (!M_PushArrayElem(ctx, j)) {
                break;
            }
            if (carried_item == nullptr) {
                carried_item = GameBuf_Alloc(sizeof(CARRIED_ITEM), GBUF_ITEMS);
                carried_item->next_item = nullptr;
                carried_item->spawn_num = NO_ITEM;
                if (prev_item != nullptr) {
                    prev_item->next_item = carried_item;
                } else {
                    item->carried_item = carried_item;
                }
            }

            int16_t carried_game_object_id;
            M_MUST(M_ReadNum(ctx, "object_id", &carried_game_object_id));
            carried_item->object_id = Object_FromGameID(carried_game_object_id);

            M_MUST(M_ReadPos(ctx, &carried_item->pos));
            M_MUST(M_ReadNum(ctx, "y_rot", &carried_item->rot.y));
            M_MUST(M_ReadNum(ctx, "room_num", &carried_item->room_num));
            M_MUST(M_ReadNum(ctx, "fall_speed", &carried_item->fall_speed));
            M_OPTIONAL(M_ReadNum(ctx, "spawn_num", &carried_item->spawn_num));
            M_MUST(M_ReadNum(ctx, "status", &carried_item->status));

            if (g_TRVersion == 1 && ctx->sg_version < VERSION_10
                && carried_item->room_num == M_NO_ROOM_LEGACY) {
                carried_item->room_num = NO_ROOM;
            }

            if (carried_item->status == DS_CARRIED
                && carried_item->spawn_num != NO_ITEM) {
                ITEM *const pickup_item = Item_Get(carried_item->spawn_num);
                if (pickup_item != nullptr
                    && pickup_item->room_num != NO_ROOM) {
                    Item_UpdateRoom(carried_item->spawn_num, NO_ROOM);
                }
            }

            prev_item = carried_item;
            carried_item = carried_item->next_item;
            M_MUST(M_Pop(ctx));
        }
        Carrier_TestItemDrops(item_num);
        M_MUST(M_Pop(ctx));
    } else {
        if (g_TRVersion == 1 && ctx->sg_version < VERSION_4) {
            Carrier_TestLegacyDrops(item_num);
        }
    }

    switch (item->object_id) {
    case O_BACON_LARA: {
        if (g_TRVersion >= 2 || ctx->sg_version >= VERSION_5) {
            int32_t status;
            // TR1X <4.16, TR2X <1.6
            if (M_ReadNum(ctx, "bl_status", &status)) {
                item->priv = (void *)(intptr_t)status;
            } else if (M_PushObject(ctx, "data")) {
                M_MUST(M_ReadNum(ctx, "status", &status));
                item->priv = (void *)(intptr_t)status;
                M_MUST(M_Pop(ctx));
            }
        }
        break;
    }

    case O_FLAME_EMITTER:
    case O_FLAME_EMITTER_BIG:
    case O_FLAME_EMITTER_SMALL:
    case O_FLAME_EMITTER_JET:
    case O_FLAME_EMITTER_SIDE: {
        if ((g_TRVersion >= 2 || ctx->sg_version >= VERSION_3)
            && g_Config.gameplay.enable_enhanced_saves) {
            int32_t effect_num = NO_EFFECT;
            // TR1X <4.16, TR2X <1.6
            if (M_ReadNum(ctx, "fx_num", &effect_num)) {
                item->data = (void *)(intptr_t)(effect_num + 1);
            } else if (M_PushObject(ctx, "data")) {
                M_MUST(M_ReadNum(ctx, "fx_num", &effect_num));
                item->data = (void *)(intptr_t)(effect_num + 1);
                M_MUST(M_Pop(ctx));
            }
        }
        break;
    }

    case O_SKIDOO_FAST: {
        M_MUST(M_PushObject(ctx, "data"));
        SKIDOO_INFO *const data = item->data;
        M_MUST(M_ReadNum(ctx, "track_mesh", &data->track_mesh));
        M_MUST(M_ReadNum(ctx, "skidoo_turn", &data->skidoo_turn));
        M_MUST(M_ReadNum(ctx, "left_fallspeed", &data->left_fallspeed));
        M_MUST(M_ReadNum(ctx, "right_fallspeed", &data->right_fallspeed));
        M_MUST(M_ReadNum(ctx, "momentum_angle", &data->momentum_angle));
        M_MUST(M_ReadNum(ctx, "extra_rotation", &data->extra_rotation));
        M_MUST(M_ReadNum(ctx, "pitch", &data->pitch));
        M_MUST(M_Pop(ctx));
        break;
    }

    default:
        break;
    }

    if (obj->priv_size > 0 && obj->priv_load_func != nullptr) {
        if (M_SHOULD(M_PushObject(ctx, "priv"))
            || M_SHOULD(M_PushObject(ctx, "data"))) {
            JSON_OBJECT *const priv_root = JSON_ValueAsObject(ctx->current);
            M_MUST(priv_root != nullptr);
            obj->priv_load_func(item, priv_root);
            M_MUST(M_Pop(ctx));
        }
    }

    if (g_TRVersion >= 2) {
        // TODO: make this call in both engines consistently
        if (obj->handle_save_func != nullptr) {
            obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
        }
    }

    M_FINISH();
}

static bool M_ReadEffect(M_CONTEXT *const ctx)
{
    int32_t room_num = NO_ROOM;
    M_MUST(M_ReadNum(ctx, "room_number", &room_num));
    const int16_t effect_num = Effect_Create(room_num);
    if (effect_num == NO_EFFECT) {
        return true;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    M_MUST(M_ReadPos(ctx, &effect->pos));
    M_MUST(M_ReadRot(ctx, &effect->rot));
    M_MUST(M_ReadObjectID(ctx, "object_number", &effect->object_id));
    M_MUST(M_ReadNum(ctx, "speed", &effect->speed));
    M_MUST(M_ReadNum(ctx, "fall_speed", &effect->fall_speed));
    M_MUST(M_ReadNum(ctx, "frame_number", &effect->frame_num));
    M_MUST(M_ReadNum(ctx, "counter", &effect->counter));
    M_MUST(M_ReadNum(ctx, "shade", &effect->shade));
    M_FINISH();
}

static bool M_ReadFlare(M_CONTEXT *const ctx)
{
    const int16_t item_num = Item_Create();
    ITEM *const item = Item_Get(item_num);
    item->object_id = O_FLARE_ITEM;
    M_MUST(M_ReadPos(ctx, &item->pos));
    M_MUST(M_ReadRot(ctx, &item->rot));
    M_MUST(M_ReadNum(ctx, "room_num", &item->room_num));
    Item_Initialise(item_num);
    M_MUST(M_ReadNum(ctx, "speed", &item->speed));
    M_MUST(M_ReadNum(ctx, "fall_speed", &item->fall_speed));
    int32_t flare_age;
    M_MUST(M_ReadNum(ctx, "age", &flare_age));
    item->data = (void *)(intptr_t)flare_age;
    Item_AddActive(item_num);
    M_FINISH();
}

static MUSIC_ID M_ConvertMusicTrack(
    const MUSIC_ID track_id, uint16_t sg_version)
{
    if (g_TRVersion == 1) {
        return track_id;
    }
    // Added in TR2X 1.2 after removing OG music track shifting, so allowing
    // previous saves to still load. Remove after a suitable period.
    if (track_id == MX_INACTIVE || sg_version >= VERSION_11) {
        return track_id;
    }
    return Music_ConvertLegacyTrack(track_id);
}

static bool M_ReadMusicTracks(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    MUSIC_ID current_track = MX_INACTIVE;
    MUSIC_ID ambient_track = MX_INACTIVE;
    double timestamp;
    M_MUST(M_ReadNum(ctx, "current_track", &current_track));
    M_OPTIONAL(M_ReadNum(ctx, "current_ambient", &ambient_track));
    M_MUST(M_ReadNum(ctx, "timestamp", &timestamp));

    // TR1X <=4.11 / TR2X <=1.1 fallback behavior
    if (g_TRVersion == 1 && ctx->sg_version < VERSION_9) {
        bool legacy_ambient = false;
        // TR1X <=4.5 has no is_ambient
        M_OPTIONAL(M_ReadBool(ctx, "is_ambient", &legacy_ambient));
        if (legacy_ambient && current_track != MX_INACTIVE) {
            ambient_track = current_track;
        }
    }

    current_track = M_ConvertMusicTrack(current_track, ctx->sg_version);
    ambient_track = M_ConvertMusicTrack(ambient_track, ctx->sg_version);

    Music_Stop();
    if (ambient_track != MX_INACTIVE) {
        // Always restart the ambient as it may have changed based on the
        // current position in the level.
        Music_Play_Direct(ambient_track, MPM_LOOP);
    }

    if (g_Config.audio.music_load_condition == MUSIC_LOAD_NEVER) {
        return true;
    }

    const bool is_ambient =
        current_track != MX_INACTIVE && current_track == ambient_track;
    if (!is_ambient && current_track != MX_INACTIVE) {
        Music_Play_Direct(current_track, MPM_ONCE);
    }

    const bool load_timestamp =
        !is_ambient || g_Config.audio.music_load_condition == MUSIC_LOAD_ALWAYS;
    if (load_timestamp && !Music_SeekTimestamp(timestamp)) {
        LOG_WARNING(
            "Could not load current track %d at timestamp %lf.", current_track,
            timestamp);
    }

    M_FINISH();
}

static bool M_ReadMusicTrackFlags(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (!g_Config.audio.load_music_triggers) {
        return true;
    }

    const int32_t count = M_GetArrayLength(ctx);
    if (count > MAX_MUSIC_TRACKS) {
        M_SetError(
            ctx, "expected at most %d music track flags, got %d",
            MAX_MUSIC_TRACKS, count);
        M_FAIL();
    }

    for (int32_t i = 0; i < count; i++) {
        M_MUST(M_PushArrayElem(ctx, i));
        uint32_t flags;
        M_MUST(M_ReadNumDirect(ctx, &flags));
        Music_SetTrackFlags(i, flags);
        M_MUST(M_Pop(ctx));
    }

    M_FINISH();
}

static bool M_ReadResumeInfo(
    SAVEGAME_BSON_READ_CONTEXT *const ctx, RESUME_INFO *const resume)
{
    resume->lara_hitpoints = g_Config.gameplay.start_lara_hitpoints;
    M_OPTIONAL(M_ReadNum(ctx, "lara_hitpoints", &resume->lara_hitpoints));

    M_MUST(M_ReadNum(ctx, "gun_status", &resume->gun_status)); // LGS_ARMLESS
    M_MUST(
        M_ReadNum(ctx, "gun_type", &resume->equipped_gun_type)); // LGT_UNARMED

    // TR1X <4.2
    resume->holsters_gun_type = LGT_UNKNOWN;
    M_OPTIONAL(M_ReadNum(ctx, "holsters_gun_type", &resume->holsters_gun_type));

    // TRX <1.1
    if (g_TRVersion == 2 && ctx->sg_version < VERSION_14) {
        if (resume->equipped_gun_type == LGT_MAGNUMS) {
            resume->equipped_gun_type = LGT_AUTOS;
        }
        if (resume->holsters_gun_type == LGT_MAGNUMS) {
            resume->holsters_gun_type = LGT_AUTOS;
        }
    }

    // TR1X <4.2
    resume->back_gun_type = LGT_UNKNOWN;
    M_OPTIONAL(M_ReadNum(ctx, "back_gun_type", &resume->back_gun_type));

    // TR2X <1.6
    M_OPTIONAL(M_ReadBool(ctx, "costume", &resume->flags.costume));

    M_MUST(M_ReadNum(ctx, "pistol_ammo", &resume->pistol_ammo));
    M_MUST(M_ReadNum(ctx, "uzi_ammo", &resume->uzi_ammo));
    M_MUST(M_ReadNum(ctx, "shotgun_ammo", &resume->shotgun_ammo));

    // TRX <1.1
    if (ctx->sg_version < VERSION_14) {
        if (g_TRVersion == 1) {
            M_MUST(M_ReadNum(ctx, "magnum_ammo", &resume->magnum_ammo));
        } else {
            M_MUST(M_ReadNum(ctx, "magnum_ammo", &resume->autos_ammo));
        }
    } else {
        M_MUST(M_ReadNum(ctx, "magnum_ammo", &resume->magnum_ammo));
        M_MUST(M_ReadNum(ctx, "autos_ammo", &resume->autos_ammo));
        M_OPTIONAL(
            M_ReadNum(ctx, "desert_eagle_ammo", &resume->desert_eagle_ammo));
    }

    // TR1X <4.16
    M_OPTIONAL(M_ReadNum(ctx, "m16_ammo", &resume->m16_ammo));
    M_OPTIONAL(M_ReadNum(ctx, "grenade_ammo", &resume->grenade_ammo));
    M_OPTIONAL(M_ReadNum(ctx, "harpoon_ammo", &resume->harpoon_ammo));
    M_MUST(M_ReadNum(ctx, "num_medis", &resume->small_medipacks));
    M_MUST(M_ReadNum(ctx, "num_big_medis", &resume->large_medipacks));
    // TR1X <4.16
    M_OPTIONAL(M_ReadNum(ctx, "num_flares", &resume->flares));
    // TR2X <1.6
    M_OPTIONAL(M_ReadNum(ctx, "num_scions", &resume->num_scions));

    // TRX <1.2
    M_OPTIONAL(M_ReadNum(ctx, "num_quest_item_1", &resume->num_quest_item_1));
    M_OPTIONAL(M_ReadNum(ctx, "num_quest_item_2", &resume->num_quest_item_2));
    M_OPTIONAL(M_ReadNum(ctx, "num_quest_item_3", &resume->num_quest_item_3));
    M_OPTIONAL(M_ReadNum(ctx, "num_quest_item_4", &resume->num_quest_item_4));

    M_MUST(M_ReadBool(ctx, "available", &resume->flags.available));

    // TRX <1.2
    resume->level_completed = false;
    resume->prev_level = -1;
    resume->hurt_allies = false;
    M_OPTIONAL(M_ReadBool(ctx, "level_completed", &resume->level_completed));
    M_OPTIONAL(M_ReadNum(ctx, "prev_level", &resume->prev_level));
    M_OPTIONAL(M_ReadBool(ctx, "hurt_allies", &resume->hurt_allies));

    if (M_HasKey(ctx, "got_pistols")) {
        // TR1X <4.16
        M_MUST(M_ReadBool(ctx, "got_pistols", &resume->flags.has_pistols));
        M_MUST(M_ReadBool(ctx, "got_shotgun", &resume->flags.has_shotgun));
        if (g_TRVersion == 1) {
            M_MUST(M_ReadBool(ctx, "got_magnums", &resume->flags.has_magnums));
        } else {
            M_MUST(M_ReadBool(ctx, "got_magnums", &resume->flags.has_autos));
        }
        M_MUST(M_ReadBool(ctx, "got_uzis", &resume->flags.has_uzis));
    } else {
        M_MUST(M_ReadBool(ctx, "has_pistols", &resume->flags.has_pistols));
        M_MUST(M_ReadBool(ctx, "has_shotgun", &resume->flags.has_shotgun));
        M_MUST(M_ReadBool(ctx, "has_uzis", &resume->flags.has_uzis));
    }
    // TR1X <4.16
    M_OPTIONAL(M_ReadBool(ctx, "has_m16", &resume->flags.has_m16));
    M_OPTIONAL(M_ReadBool(ctx, "has_grenade", &resume->flags.has_grenade));
    M_OPTIONAL(M_ReadBool(ctx, "has_harpoon", &resume->flags.has_harpoon));

    // TRX <1.1
    if (ctx->sg_version < VERSION_14) {
        if (g_TRVersion == 1) {
            M_MUST(M_ReadBool(ctx, "has_magnums", &resume->flags.has_magnums));
        } else {
            M_MUST(M_ReadBool(ctx, "has_magnums", &resume->flags.has_autos));
        }
    } else {
        M_MUST(M_ReadBool(ctx, "has_magnums", &resume->flags.has_magnums));
        M_MUST(M_ReadBool(ctx, "has_autos", &resume->flags.has_autos));
        M_OPTIONAL(M_ReadBool(
            ctx, "has_desert_eagle", &resume->flags.has_desert_eagle));
        M_OPTIONAL(M_ReadBool(ctx, "has_mp5", &resume->flags.has_mp5));
        M_OPTIONAL(M_ReadNum(ctx, "mp5_ammo", &resume->mp5_ammo));
        M_OPTIONAL(M_ReadBool(ctx, "has_rocket", &resume->flags.has_rocket));
        M_OPTIONAL(M_ReadNum(ctx, "rocket_ammo", &resume->rocket_ammo));
    }

    M_MUST(M_ReadNum(ctx, "timer", &resume->stats.timer));
    // TR1X <4.9
    M_OPTIONAL(M_ReadNum(ctx, "ammo_hits", &resume->stats.ammo_hits));
    M_OPTIONAL(M_ReadNum(ctx, "ammo_used", &resume->stats.ammo_used));
    M_OPTIONAL(M_ReadNum(ctx, "medipacks_used", &resume->stats.medipacks_used));
    M_OPTIONAL(M_ReadNum(
        ctx, "distance_travelled", &resume->stats.distance_travelled));
    M_MUST(M_ReadNum(ctx, "kills", &resume->stats.kill_count));
    // TR2X <1.6
    M_OPTIONAL(M_ReadNum(ctx, "pickups", &resume->stats.pickup_count));
    M_MUST(M_ReadNum(ctx, "secrets", &resume->stats.secret_flags));
    Stats_UpdateSecrets(&resume->stats);
    M_FINISH();
}

SAVEGAME_BSON_READ_CONTEXT *Savegame_BSON_StartRead(
    JSON_VALUE *const root, const uint16_t sg_version)
{
    M_CONTEXT *const ctx = Memory_Alloc(sizeof(*ctx));
    ctx->stack[0] = root;
    ctx->current_pos = 0;
    ctx->current = ctx->stack[0];
    ctx->sg_version = sg_version;
    return ctx;
}

void Savegame_BSON_FinishRead(
    SAVEGAME_BSON_READ_CONTEXT *const ctx, const bool success)
{
    if (!success && ctx->error_msg[0] != '\0') {
        LOG_ERROR("%s", ctx->error_msg);
    }
    Memory_Free(ctx);
}

bool Savegame_BSON_LoadInventory(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    M_MUST(M_PushObject(ctx, "inventory"));
    const GF_LEVEL *const current_level = Game_GetCurrentLevel();

    struct {
        OBJECT_ID object_id;
        const char *const key;
    } objects[] = {
        { O_PICKUP_ITEM_1, "pickup1" }, { O_PICKUP_ITEM_2, "pickup2" },
        { O_QUEST_ITEM_1, "quest1" },   { O_QUEST_ITEM_2, "quest2" },
        { O_QUEST_ITEM_3, "quest3" },   { O_QUEST_ITEM_4, "quest4" },
        { O_PUZZLE_ITEM_1, "puzzle1" }, { O_PUZZLE_ITEM_2, "puzzle2" },
        { O_PUZZLE_ITEM_3, "puzzle3" }, { O_PUZZLE_ITEM_4, "puzzle4" },
        { O_KEY_ITEM_1, "key1" },       { O_KEY_ITEM_2, "key2" },
        { O_KEY_ITEM_3, "key3" },       { O_KEY_ITEM_4, "key4" },
        { O_LEADBAR_ITEM, "leadbar" },  { NO_OBJECT, nullptr },
    };

    Lara_InitialiseInventory(current_level);
    for (int32_t i = 0; objects[i].key != nullptr; i++) {
        int16_t qty;
        if (M_ReadNum(ctx, objects[i].key, &qty)) {
            Inv_RemoveItem(objects[i].object_id);
            Inv_AddItemNTimes(objects[i].object_id, qty);
        }
    }

    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadFlipmaps(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    M_MUST(M_PushObject(ctx, "flipmap"));

    bool status;
    M_MUST(M_ReadBool(ctx, "status", &status));
    if (status) {
        Room_FlipMap();
    }

    int32_t flip_effect;
    int32_t flip_timer;
    M_MUST(M_ReadNum(ctx, "effect", &flip_effect));
    M_MUST(M_ReadNum(ctx, "timer", &flip_timer));
    Room_SetFlipEffect(flip_effect);
    Room_SetFlipTimer(flip_timer);

    M_MUST(M_PushObject(ctx, "table"));
    const size_t count = M_GetArrayLength(ctx);
    if (count != MAX_FLIP_MAPS) {
        M_SetError(
            ctx, "expected %d flipmap elements, got %d", MAX_FLIP_MAPS, count);
        M_FAIL();
    }
    for (size_t i = 0; i < count; i++) {
        if (!M_PushArrayElem(ctx, i)) {
            break;
        }
        uint32_t flags;
        M_MUST(M_ReadNumDirect(ctx, &flags));
        Room_SetFlipSlotFlags(i, flags << 8);
        M_MUST(M_Pop(ctx));
    }
    M_MUST(M_Pop(ctx));

    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadCameras(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    M_MUST(M_PushObject(ctx, "cameras"));
    const size_t count = M_GetArrayLength(ctx);
    if (count != (size_t)Camera_GetFixedObjectCount()) {
        M_SetError(
            ctx, "expected %d cameras, got %d", Camera_GetFixedObjectCount(),
            count);
        M_FAIL();
    }
    for (size_t i = 0; i < count; i++) {
        M_MUST(M_PushArrayElem(ctx, i));
        OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        M_MUST(M_ReadNumDirect(ctx, &object->flags));
        M_MUST(M_Pop(ctx));
    }
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadLara(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    M_MUST(M_PushObject(ctx, "lara"));
    M_MUST(M_ReadLara(ctx));
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadItems(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    M_MUST(M_PushObject(ctx, "items"));
    const int32_t count = M_GetArrayLength(ctx);
    if (count != Item_GetLevelCount()) {
        M_SetError(
            ctx, "expected %d items, got %d", Item_GetLevelCount(), count);
        M_FAIL();
    }

    Savegame_ProcessItemsBeforeLoad();

    for (int32_t i = 0; i < count; i++) {
        M_MUST(M_PushArrayElem(ctx, i));
        M_MUST(M_ReadItem(ctx, i));
        M_MUST(M_Pop(ctx));
    }

    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadEffects(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return true;
    }

    // TR1X <=v2.15.3, TR2X <=v1.1 may not have fx effects
    if (M_SHOULD(M_PushObject(ctx, "fx"))) {
        for (int32_t i = 0;; i++) {
            if (!M_PushArrayElem(ctx, i)) {
                break;
            }
            if (i < MAX_EFFECTS) {
                M_ReadEffect(ctx);
            } else {
                LOG_WARNING(
                    "Malformed save: expected a max of %d effect, got at least "
                    "%d. "
                    "extra effects will be ignored.",
                    MAX_EFFECTS - 1, i);
            }
            M_MUST(M_Pop(ctx));
        }
        M_MUST(M_Pop(ctx));
    }
    M_FINISH();
}

bool Savegame_BSON_LoadFlares(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (M_SHOULD(M_PushObject(ctx, "flares"))) {
        for (int32_t i = 0;; i++) {
            if (!M_PushArrayElem(ctx, i)) {
                break;
            }
            M_MUST(M_ReadFlare(ctx));
            M_MUST(M_Pop(ctx));
        }
        M_MUST(M_Pop(ctx));
    }
    M_FINISH();
}

bool Savegame_BSON_LoadMusic(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (M_HasKey(ctx, "music_track_flags")) {
        // TR1X <4.16
        M_MUST(M_PushObject(ctx, "music_track_flags"));
        M_MUST(M_ReadMusicTrackFlags(ctx));
        M_MUST(M_Pop(ctx));
        M_MUST(M_PushObject(ctx, "music"));
        M_MUST(M_ReadMusicTracks(ctx));
        M_MUST(M_Pop(ctx));
    } else {
        M_MUST(M_PushObject(ctx, "music"));
        M_MUST(M_PushObject(ctx, "current"));
        M_MUST(M_ReadMusicTracks(ctx));
        M_MUST(M_Pop(ctx));
        M_MUST(M_PushObject(ctx, "flags"));
        M_MUST(M_ReadMusicTrackFlags(ctx));
        M_MUST(M_Pop(ctx));
        M_MUST(M_Pop(ctx));
    }

    M_FINISH();
}

bool Savegame_BSON_LoadResumeInfoList(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (M_HasKey(ctx, "current_info")) {
        // TR1X <4.16
        M_MUST(M_PushObject(ctx, "current_info"));
    } else {
        // TR2X <1.6
        M_MUST(M_PushObject(ctx, "resume_info"));
    }
    const int32_t length = M_GetArrayLength(ctx);
    const int32_t expected_length = GF_GetLevelTable(GFLT_MAIN)->count;
    if (length != expected_length) {
        M_SetError(
            ctx, "expected %d resume info elements, got %d", expected_length,
            length);
        M_FAIL();
    }
    for (int32_t i = 0; i < length; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        M_MUST(M_PushArrayElem(ctx, i));
        M_MUST(M_ReadResumeInfo(ctx, resume));
        M_MUST(M_Pop(ctx));
    }
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadMisc(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    M_MUST(M_PushObject(ctx, "misc"));

    {
        int32_t bonus_flag = false;
        M_OPTIONAL(M_ReadNum(ctx, "bonus_flag", &bonus_flag));
        Game_SetBonusFlag(bonus_flag);
    }

    {
        bool allies_hostile = false;
        M_OPTIONAL(M_ReadBool(ctx, "are_monks_angry", &allies_hostile));
        Creature_SetAlliesHostile(allies_hostile);
    }

    {
        int32_t sunset_timer;
        M_OPTIONAL(M_ReadNum(ctx, "sunset_timer", &sunset_timer));
        Output_SetTimeInGame(sunset_timer);
    }

    {
        const GF_LEVEL *const current_level = Game_GetCurrentLevel();
        RESUME_INFO *const resume = Savegame_GetCurrentInfo(current_level);
        resume->stats.death_count = -1;
        M_OPTIONAL(M_ReadNum(ctx, "death_count", &resume->stats.death_count));
    }

    {
        if (M_HasKey(ctx, "weather_type") != 0) {
            int32_t weather_type = (int32_t)WEATHER_NONE;
            if (M_ReadNum(ctx, "weather_type", &weather_type)) {
                if (weather_type >= (int32_t)WEATHER_NONE
                    && weather_type <= (int32_t)WEATHER_SNOW) {
                    WeatherFX_SetWeather((WEATHER_TYPE)weather_type);
                } else {
                    WeatherFX_SetWeather(WEATHER_NONE);
                }
            }
        }
    }

    M_MUST(M_Pop(ctx));
    M_FINISH();
}
