#include "config.h"
#include "debug.h"
#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/music.h"
#include "game/objects.h"
#include "game/objects/general/lift.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/sliding_pillar.h"
#include "game/objects/vars.h"
#include "game/objects/vehicles/boat.h"
#include "game/objects/vehicles/skidoo_common.h"
#include "game/pathing.h"
#include "game/rooms.h"
#include "game/savegame.h"
#include "memory.h"
#include "strings.h"
#include "version.h"

#define M_NO_ROOM_LEGACY 255

#define M_MAX_STACK_SIZE 10
#define M_SHOULD(x)                                                            \
    if (!(x)) {                                                                \
        goto success;                                                          \
    }
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
        snprintf(ctx->error_msg, sizeof(ctx->error_msg), "%s", ctx->path);
        strncat(
            ctx->error_msg, ": ",
            sizeof(ctx->error_msg) - strlen(ctx->error_msg) - 1);
        strncat(
            ctx->error_msg, body,
            sizeof(ctx->error_msg) - strlen(ctx->error_msg) - 1);
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
// Start of SG data readers
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
        // TR1X <=v4.15
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
        case O_UZI_AMMO_ITEM: return initial_obj_id == O_UZI_ITEM;
        case O_HARPOON_AMMO_ITEM: return initial_obj_id == O_HARPOON_ITEM;
        case O_M16_AMMO_ITEM: return initial_obj_id == O_M16_ITEM;
        case O_GRENADE_AMMO_ITEM: return initial_obj_id == O_GRENADE_ITEM;
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
    SAVEGAME_BSON_READ_CONTEXT *const ctx, const int16_t item_num,
    const uint16_t header_version)
{
    ITEM *const item = Item_Get(item_num);

    int16_t game_object_id = -1;
    M_MUST(M_ReadNum(ctx, "obj_num", &game_object_id));
    const OBJECT_ID object_id = Object_FromGameID(game_object_id);
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
        M_MUST(M_ReadNum(ctx, "current_anim", &item->current_anim_state));
        M_MUST(M_ReadNum(ctx, "goal_anim", &item->goal_anim_state));
        M_MUST(M_ReadNum(ctx, "required_anim", &item->required_anim_state));
        M_MUST(M_ReadNum(ctx, "anim_num", &item->anim_num));
        M_MUST(M_ReadNum(ctx, "frame_num", &item->frame_num));

        // Prevent issues with pre-injection saves and Lara's enhanced
        // animation set.
        if (item->object_id == O_LARA
            && item->anim_num < LARA_ORIGINAL_ANIM_COUNT) {
            item->anim_num += obj->anim_idx;
        }
    }

    if (obj->save_hitpoints) {
        M_MUST(M_ReadNum(ctx, "hitpoints", &item->hit_points));
        M_OPTIONAL(M_ReadNum(ctx, "max_hitpoints", &item->max_hit_points));
    }

    if (obj->save_flags) {
        M_MUST(M_ReadNum(ctx, "flags", &item->flags));
        M_MUST(M_ReadNum(ctx, "timer", &item->timer));

        if ((item->flags & IF_KILLED) != 0) {
            Item_Kill(item_num);
            item->status = IS_DEACTIVATED;
        } else {
            bool is_active;
            M_MUST(M_ReadBool(ctx, "active", &is_active));
            if (is_active && !item->active) {
                Item_AddActive(item_num);
            }
            M_MUST(M_ReadNum(ctx, "status", &item->status));
            M_MUST(M_ReadBool(ctx, "gravity", &item->gravity));
            M_OPTIONAL(M_ReadBool(ctx, "collidable", &item->collidable));
        }

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
            }
        } else if (obj->intelligent) {
            item->data = nullptr;
#if TR_VERSION == 2
            if (item->killed && item->hit_points <= 0
                && !(item->flags & IF_KILLED)) {
                item->next_active = Item_GetPrevActive();
                Item_SetPrevActive(item_num);
            }
#endif
        }
    }

    if (M_HasKey(ctx, "carried_items")) {
        M_MUST(M_PushObject(ctx, "carried_items"));
        CARRIED_ITEM *carried_item = item->carried_item;
        for (int32_t j = 0;; j++) {
            if (!M_PushArrayElem(ctx, j)) {
                break;
            }
            if (carried_item == nullptr) {
                M_SetError(ctx, "carried item mismatch");
                M_FAIL();
            }

            int16_t game_object_id;
            M_MUST(M_ReadNum(ctx, "object_id", &game_object_id));
            carried_item->object_id = Object_FromGameID(game_object_id);

            M_MUST(M_ReadPos(ctx, &carried_item->pos));
            M_MUST(M_ReadNum(ctx, "y_rot", &carried_item->rot.y));
            M_MUST(M_ReadNum(ctx, "room_num", &carried_item->room_num));
            M_MUST(M_ReadNum(ctx, "fall_speed", &carried_item->fall_speed));
            M_MUST(M_ReadNum(ctx, "status", &carried_item->status));

#if TR_VERSION == 1
            if (header_version < VERSION_10
                && carried_item->room_num == M_NO_ROOM_LEGACY) {
                carried_item->room_num = NO_ROOM;
            }
#endif

            carried_item = carried_item->next_item;
            M_MUST(M_Pop(ctx));
        }
        Carrier_TestItemDrops(item_num);
        M_MUST(M_Pop(ctx));
    } else {
#if TR_VERSION == 1
        if (header_version < VERSION_4) {
            Carrier_TestLegacyDrops(item_num);
        }
#endif
    }

    switch (item->object_id) {
    case O_BACON_LARA: {
        if (g_TRVersion == 2 || header_version >= VERSION_5) {
            int32_t status;
            if (M_ReadNum(ctx, "bl_status", &status)) {
                item->priv = (void *)(intptr_t)status;
            }
        }
        break;
    }

    case O_FLAME_EMITTER: {
        if ((g_TRVersion == 2 || header_version >= VERSION_3)
            && g_Config.gameplay.enable_enhanced_saves) {
            int32_t effect_num = NO_EFFECT;
            M_OPTIONAL(M_ReadNum(ctx, "fx_num", &effect_num));
            if (effect_num != -1) {
                item->data = (void *)(intptr_t)(effect_num + 1);
            }
        }
        break;
    }

    case O_MOVABLE_BLOCK_1:
    case O_MOVABLE_BLOCK_2:
    case O_MOVABLE_BLOCK_3:
    case O_MOVABLE_BLOCK_4: {
        if (header_version >= VERSION_12) {
            M_MUST(M_PushObject(ctx, "data"));
            MOVABLE_BLOCK_INFO *const data = item->data;
            M_MUST(M_ReadNum(ctx, "counter_rot_0", &data->counter_rot[0]));
            M_MUST(M_ReadNum(ctx, "counter_rot_1", &data->counter_rot[1]));
            M_MUST(M_ReadNum(ctx, "counter_rot_2", &data->counter_rot[2]));
            M_MUST(M_ReadNum(ctx, "original_rot", &data->original_rot));
            M_MUST(M_ReadNum(ctx, "gravity_frames", &data->gravity_frames));
            M_MUST(M_ReadBool(ctx, "is_push_pull", &data->is_push_pull));
            M_MUST(
                M_ReadBool(ctx, "is_forced_moving", &data->is_forced_moving));
            M_MUST(M_ReadXYZ32(ctx, "linked", &data->linked.pos));
            M_MUST(M_Pop(ctx));
        } else {
            // For old saves, guess linked sector is at item position.
            MOVABLE_BLOCK_INFO *const data = item->data;
            data->linked.pos = item->pos;
            data->linked.room_num = item->room_num;
        }
        break;
    }

    case O_SLIDING_PILLAR:
        if (header_version >= VERSION_12 && item->data != nullptr) {
            M_MUST(M_PushObject(ctx, "data"));
            SLIDING_PILLAR_INFO *const data = item->data;
            M_MUST(M_ReadXYZ32(ctx, "linked", &data->linked.pos));
            M_MUST(M_Pop(ctx));
        } else {
            // For old saves, guess linked sector is at item position.
            SLIDING_PILLAR_INFO *const data = item->data;
            data->linked.pos = item->pos;
            data->linked.room_num = item->room_num;
        }
        break;

    case O_BOAT: {
        M_MUST(M_PushObject(ctx, "data"));
        BOAT_INFO *const data = item->data;
        M_MUST(M_ReadNum(ctx, "boat_turn", &data->boat_turn));
        M_MUST(M_ReadNum(ctx, "left_fallspeed", &data->left_fallspeed));
        M_MUST(M_ReadNum(ctx, "right_fallspeed", &data->right_fallspeed));
        M_MUST(M_ReadNum(ctx, "tilt_angle", &data->tilt_angle));
        M_MUST(M_ReadNum(ctx, "extra_rotation", &data->extra_rotation));
        M_MUST(M_ReadNum(ctx, "water", &data->water));
        M_MUST(M_ReadNum(ctx, "pitch", &data->pitch));
        M_MUST(M_Pop(ctx));
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

    case O_LIFT: {
        M_MUST(M_PushObject(ctx, "data"));
        LIFT_INFO *const data = item->data;
        M_MUST(M_ReadNum(ctx, "start_height", &data->start_height));
        M_MUST(M_ReadNum(ctx, "wait_time", &data->wait_time));
        if (header_version >= VERSION_12) {
            M_MUST(M_ReadBool(ctx, "is_moving", &data->is_moving));
            for (int32_t j = 0; j < LIFT_NUM_SECTORS; j++) {
                const char *const pos_key = String_FormatStatic("linked_%d", j);
                M_MUST(M_ReadXYZ32(ctx, pos_key, &data->linked[j].pos));
            }
        }
        M_MUST(M_Pop(ctx));
        break;
    }

    default:
        break;
    }

#if TR_VERSION == 2
    // TODO: make this call in both engines consistently
    if (obj->handle_save_func != nullptr) {
        obj->handle_save_func(item, SAVEGAME_STAGE_AFTER_LOAD);
    }
#endif

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
    const MUSIC_ID track_id, const uint16_t header_version)
{
    if (g_TRVersion == 1) {
        return track_id;
    }
    // Added in TR2X 1.2 after removing OG music track shifting, so allowing
    // previous saves to still load. Remove after a suitable period.
    if (track_id == MX_INACTIVE || header_version >= VERSION_11) {
        return track_id;
    }
    return Music_ConvertLegacyTrack(track_id);
}

static bool M_ReadMusicTracks(
    SAVEGAME_BSON_READ_CONTEXT *const ctx, const uint16_t header_version)
{
    MUSIC_ID current_track = MX_INACTIVE;
    MUSIC_ID ambient_track = MX_INACTIVE;
    double timestamp;
    M_MUST(M_ReadNum(ctx, "current_track", &current_track));
    M_OPTIONAL(M_ReadNum(ctx, "current_ambient", &ambient_track));
    M_MUST(M_ReadNum(ctx, "timestamp", &timestamp));

    // TR1X <=4.11 / TR2X <=1.1 fallback behavior
    if (g_TRVersion == 1 && header_version < VERSION_9) {
        bool legacy_ambient = false;
        // TR1X <=4.5 has no is_ambient
        M_OPTIONAL(M_ReadBool(ctx, "is_ambient", &legacy_ambient));
        if (legacy_ambient && current_track != MX_INACTIVE) {
            ambient_track = current_track;
        }
    }

    current_track = M_ConvertMusicTrack(current_track, header_version);
    ambient_track = M_ConvertMusicTrack(ambient_track, header_version);

    Music_Stop();
    if (ambient_track != MX_INACTIVE) {
        // Always restart the ambient as it may have changed based on the
        // current position in the level.
        Music_Play_Direct(ambient_track, MPM_LOOPED);
    }

    if (g_Config.audio.music_load_condition == MUSIC_LOAD_NEVER) {
        return true;
    }

    const bool is_ambient =
        current_track != MX_INACTIVE && current_track == ambient_track;
    if (!is_ambient && current_track != MX_INACTIVE) {
        Music_Play_Direct(current_track, MPM_ALWAYS);
    }

    const bool load_timestamp =
        !is_ambient || g_Config.audio.music_load_condition == MUSIC_LOAD_ALWAYS;
    if (load_timestamp && !Music_SeekTimestamp(timestamp)) {
        LOG_WARNING(
            "Could not load current track %d at timestamp %" PRId64 ".",
            current_track, timestamp);
    }

    M_FINISH();
}

static bool M_ReadMusicTrackFlags(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
#if TR_VERSION == 1
    if (!g_Config.audio.load_music_triggers) {
        return true;
    }
#endif

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

SAVEGAME_BSON_READ_CONTEXT *Savegame_BSON_StartRead(JSON_VALUE *const root)
{
    M_CONTEXT *const ctx = Memory_Alloc(sizeof(*ctx));
    ctx->stack[0] = root;
    ctx->current_pos = 0;
    ctx->current = ctx->stack[0];
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

bool Savegame_BSON_LoadItems(
    SAVEGAME_BSON_READ_CONTEXT *const ctx, const uint16_t header_version)
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
        M_MUST(M_ReadItem(ctx, i, header_version));
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
    M_SHOULD(M_PushObject(ctx, "fx"));
    for (int32_t i = 0;; i++) {
        if (!M_PushArrayElem(ctx, i)) {
            break;
        }
        if (i < MAX_EFFECTS) {
            M_ReadEffect(ctx);
        } else {
            LOG_WARNING(
                "Malformed save: expected a max of %d effect, got at least %d. "
                "extra effects will be ignored.",
                MAX_EFFECTS - 1, i);
        }
        M_MUST(M_Pop(ctx));
    }
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadFlares(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (g_TRVersion == 1) {
        M_SHOULD(M_PushObject(ctx, "flares"));
    } else {
        M_MUST(M_PushObject(ctx, "flares"));
    }
    for (int32_t i = 0;; i++) {
        if (!M_PushArrayElem(ctx, i)) {
            break;
        }
        M_MUST(M_ReadFlare(ctx));
        M_MUST(M_Pop(ctx));
    }
    M_MUST(M_Pop(ctx));
    M_FINISH();
}

bool Savegame_BSON_LoadMusic(
    SAVEGAME_BSON_READ_CONTEXT *const ctx, const uint16_t header_version)
{
    if (M_HasKey(ctx, "music_track_flags")) {
        // TR1X <4.16
        M_MUST(M_PushObject(ctx, "music_track_flags"));
        M_MUST(M_ReadMusicTrackFlags(ctx));
        M_MUST(M_Pop(ctx));
        M_MUST(M_PushObject(ctx, "music"));
        M_MUST(M_ReadMusicTracks(ctx, header_version));
        M_MUST(M_Pop(ctx));
    } else {
        M_MUST(M_PushObject(ctx, "music"));
        M_MUST(M_PushObject(ctx, "current"));
        M_MUST(M_ReadMusicTracks(ctx, header_version));
        M_MUST(M_Pop(ctx));
        M_MUST(M_PushObject(ctx, "flags"));
        M_MUST(M_ReadMusicTrackFlags(ctx));
        M_MUST(M_Pop(ctx));
        M_MUST(M_Pop(ctx));
    }

    M_FINISH();
}
