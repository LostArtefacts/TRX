#include "config.h"
#include "debug.h"
#include "game/effects.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/objects.h"
#include "game/rooms.h"
#include "game/savegame/bson.h"
#include "memory.h"
#include "strings.h"
#include "version.h"

#define M_MAX_STACK_SIZE 10
#define M_SHOULD(x)                                                            \
    if (!(x)) {                                                                \
        goto success;                                                          \
    }
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

bool Savegame_BSON_LoadEffects(SAVEGAME_BSON_READ_CONTEXT *const ctx)
{
    if (!g_Config.gameplay.enable_enhanced_saves) {
        return true;
    }

    // TR1X ..v2.15.3, TR2X ..v1.1 may not have fx effects
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
