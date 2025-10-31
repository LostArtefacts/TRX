#include "config.h"
#include "debug.h"
#include "game/items.h"
#include "game/savegame/bson.h"
#include "memory.h"

#define M_MAX_STACK_SIZE 10

typedef struct SAVEGAME_BSON_WRITE_CONTEXT {
    JSON_VALUE *stack[M_MAX_STACK_SIZE];
    JSON_VALUE *current;
    size_t current_pos;
} SAVEGAME_BSON_WRITE_CONTEXT;

typedef SAVEGAME_BSON_WRITE_CONTEXT M_CONTEXT;

// =============================================================================
// Start of internal helpers
// =============================================================================

static void M_PushValue(M_CONTEXT *const ctx, JSON_VALUE *const value)
{
    ctx->current_pos++;
    ctx->stack[ctx->current_pos] = value;
    ctx->current = value;
}

static void M_Pop(M_CONTEXT *const ctx)
{
    ctx->current_pos--;
    ctx->current = ctx->stack[ctx->current_pos];
}

static void M_PushObject(M_CONTEXT *const ctx)
{
    JSON_OBJECT *const child = JSON_ObjectNew();
    M_PushValue(ctx, JSON_ValueFromObject(child));
}

static void M_PushArray(M_CONTEXT *const ctx)
{
    JSON_ARRAY *const child = JSON_ArrayNew();
    M_PushValue(ctx, JSON_ValueFromArray(child));
}

static void M_PopAndSet(M_CONTEXT *const ctx, const char *const key)
{
    JSON_OBJECT *const parent =
        JSON_ValueAsObject(ctx->stack[ctx->current_pos - 1]);
    ASSERT(parent != nullptr);
    JSON_ObjectAppend(parent, key, ctx->current);
    M_Pop(ctx);
}

static void M_PopAndAppend(M_CONTEXT *const ctx)
{
    JSON_ARRAY *const parent =
        JSON_ValueAsArray(ctx->stack[ctx->current_pos - 1]);
    ASSERT(parent != nullptr);
    JSON_ArrayAppend(parent, ctx->current);
    M_Pop(ctx);
}

static void M_PushBool(M_CONTEXT *const ctx, const bool value)
{
    M_PushValue(ctx, JSON_ValueFromBool(value));
}

static void M_PushNum_Int(M_CONTEXT *const ctx, const int32_t value)
{
    M_PushValue(ctx, JSON_ValueFromInt(value));
}

static void M_PushNum_Double(M_CONTEXT *const ctx, const double value)
{
    M_PushValue(ctx, JSON_ValueFromDouble(value));
}

#define M_PushNum(ctx, value)                                                  \
    _Generic(                                                                  \
        (value),                                                               \
        int8_t: M_PushNum_Int,                                                 \
        uint8_t: M_PushNum_Int,                                                \
        int16_t: M_PushNum_Int,                                                \
        uint16_t: M_PushNum_Int,                                               \
        int32_t: M_PushNum_Int,                                                \
        uint32_t: M_PushNum_Int,                                               \
        float: M_PushNum_Double,                                               \
        double: M_PushNum_Double)(ctx, value)

static void M_WriteBool(
    M_CONTEXT *const ctx, const char *const key, const bool value)
{
    M_PushBool(ctx, value);
    M_PopAndSet(ctx, key);
}

static void M_WriteNum_Int(
    M_CONTEXT *const ctx, const char *const key, const int32_t value)
{
    M_PushNum_Int(ctx, value);
    M_PopAndSet(ctx, key);
}

static void M_WriteNum_Double(
    M_CONTEXT *const ctx, const char *const key, const double value)
{
    M_PushNum_Double(ctx, value);
    M_PopAndSet(ctx, key);
}

#define M_WriteNum(ctx, key, value)                                            \
    _Generic(                                                                  \
        (value),                                                               \
        int8_t: M_WriteNum_Int,                                                \
        uint8_t: M_WriteNum_Int,                                               \
        int16_t: M_WriteNum_Int,                                               \
        uint16_t: M_WriteNum_Int,                                              \
        int32_t: M_WriteNum_Int,                                               \
        uint32_t: M_WriteNum_Int,                                              \
        float: M_WriteNum_Double,                                              \
        double: M_WriteNum_Double)(ctx, key, value)

// =============================================================================
// End of internal helpers
// =============================================================================

static void M_WriteXYZ32(
    M_CONTEXT *const ctx, const char *const key, const XYZ_32 source)
{
    M_PushObject(ctx);
    M_WriteNum(ctx, "x", source.x);
    M_WriteNum(ctx, "y", source.y);
    M_WriteNum(ctx, "z", source.z);
    M_PopAndSet(ctx, key);
}

static void M_WriteXYZ16(
    M_CONTEXT *const ctx, const char *const key, const XYZ_16 source)
{
    M_PushObject(ctx);
    M_WriteNum(ctx, "x", source.x);
    M_WriteNum(ctx, "y", source.y);
    M_WriteNum(ctx, "z", source.z);
    M_PopAndSet(ctx, key);
}

SAVEGAME_BSON_WRITE_CONTEXT *Savegame_BSON_StartWrite(void)
{
    M_CONTEXT *const ctx = Memory_Alloc(sizeof(*ctx));
    JSON_OBJECT *const root_obj = JSON_ObjectNew();
    ctx->stack[0] = JSON_ValueFromObject(root_obj);
    ctx->current = ctx->stack[0];
    return ctx;
}

void Savegame_BSON_FinishWrite(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    JSON_ValueFree(ctx->stack[0]);
    Memory_Free(ctx);
}

JSON_OBJECT *Savegame_BSON_GetWriteRoot(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    JSON_OBJECT *const root_obj = JSON_ValueAsObject(ctx->stack[0]);
    ASSERT(root_obj != nullptr);
    return root_obj;
}

void Savegame_BSON_DumpFlares(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    M_PushArray(ctx);
    for (int32_t i = 0; i < Item_GetTotalCount(); i++) {
        const ITEM *const item = Item_Get(i);
        if (!item->active || item->object_id != O_FLARE_ITEM) {
            continue;
        }
        M_PushObject(ctx);
        M_WriteXYZ32(ctx, "pos", item->pos);
        M_WriteXYZ16(ctx, "rot", item->rot);
        M_WriteNum(ctx, "room_num", item->room_num);
        M_WriteNum(ctx, "speed", item->speed);
        M_WriteNum(ctx, "fall_speed", item->fall_speed);
        M_WriteNum(ctx, "age", (int32_t)(intptr_t)item->data);
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "flares");
}
