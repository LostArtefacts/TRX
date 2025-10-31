#include "config.h"
#include "debug.h"
#include "game/camera.h"
#include "game/carrier.h"
#include "game/effects.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/music.h"
#include "game/objects.h"
#include "game/objects/general/lift.h"
#include "game/objects/traps/movable_block.h"
#include "game/objects/traps/sliding_pillar.h"
#include "game/objects/vehicles/boat.h"
#include "game/objects/vehicles/skidoo_common.h"
#include "game/rooms.h"
#include "game/savegame.h"
#include "game/savegame/bson.h"
#include "memory.h"

#define M_MAX_STACK_SIZE 10

typedef struct {
    int16_t count;
    int16_t id_map[MAX_EFFECTS];
} M_FX_ORDER;

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

static void M_GetFXOrder(M_FX_ORDER *const order)
{
    order->count = 0;
    for (int32_t i = 0; i < MAX_EFFECTS; i++) {
        order->id_map[i] = -1;
    }

    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        order->id_map[link_num] = order->count;
        order->count++;
    }
}

static void M_WriteItem(
    SAVEGAME_BSON_WRITE_CONTEXT *const ctx, const ITEM *const item,
    const M_FX_ORDER *const fx_order)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    // TR1X <4.16, TR2X <1.6
    M_WriteNum(ctx, "obj_num", Object_ToGameID(item->object_id));
    M_WriteNum(ctx, "object_id", Object_ToGameID(item->object_id));

    if (obj->save_position) {
        // TR1X <4.16
        M_WriteNum(ctx, "x", item->pos.x);
        M_WriteNum(ctx, "y", item->pos.y);
        M_WriteNum(ctx, "z", item->pos.z);
        M_WriteNum(ctx, "x_rot", item->rot.x);
        M_WriteNum(ctx, "y_rot", item->rot.y);
        M_WriteNum(ctx, "z_rot", item->rot.z);

        M_WriteXYZ32(ctx, "pos", item->pos);
        M_WriteXYZ16(ctx, "rot", item->rot);
        M_WriteNum(ctx, "room_num", item->room_num);
        M_WriteNum(ctx, "speed", item->speed);
        M_WriteNum(ctx, "fall_speed", item->fall_speed);
    }

    if (obj->save_anim) {
        M_WriteNum(ctx, "current_anim", item->current_anim_state);
        M_WriteNum(ctx, "goal_anim", item->goal_anim_state);
        M_WriteNum(ctx, "required_anim", item->required_anim_state);
        M_WriteNum(ctx, "anim_num", item->anim_num);
        M_WriteNum(ctx, "frame_num", item->frame_num);
    }

    if (obj->save_hitpoints) {
        M_WriteNum(ctx, "hitpoints", item->hit_points);
        M_WriteNum(ctx, "max_hitpoints", item->max_hit_points);
    }

    if (obj->save_flags) {
        M_WriteNum(ctx, "flags", item->flags);
        M_WriteNum(ctx, "status", item->status);
        M_WriteBool(ctx, "active", item->active);
        M_WriteBool(ctx, "gravity", item->gravity);
        M_WriteBool(ctx, "collidable", item->collidable);
        M_WriteBool(ctx, "intelligent", obj->intelligent && item->data);
        M_WriteNum(ctx, "timer", item->timer);
        if (obj->intelligent && item->data != nullptr) {
            const CREATURE *const creature = item->data;
            M_WriteNum(ctx, "head_rot", creature->head_rotation);
            M_WriteNum(ctx, "neck_rot", creature->neck_rotation);
            M_WriteNum(ctx, "max_turn", creature->maximum_turn);
            M_WriteNum(ctx, "creature_flags", creature->flags);
            M_WriteNum(ctx, "creature_mood", creature->mood);
        }
    }

    M_PushArray(ctx);
    const CARRIED_ITEM *drop_item = item->carried_item;
    while (drop_item != nullptr) {
        M_PushObject(ctx);
        M_WriteNum(ctx, "object_id", Object_ToGameID(drop_item->object_id));
        M_WriteXYZ32(ctx, "pos", drop_item->pos);
        M_WriteNum(ctx, "y_rot", drop_item->rot.y);
        M_WriteNum(ctx, "room_num", drop_item->room_num);
        M_WriteNum(ctx, "fall_speed", drop_item->fall_speed);
        M_WriteNum(ctx, "status", (int32_t)Carrier_GetSaveStatus(drop_item));

        // TR1X <4.16
        M_WriteNum(ctx, "x", drop_item->pos.x);
        M_WriteNum(ctx, "y", drop_item->pos.y);
        M_WriteNum(ctx, "z", drop_item->pos.z);

        M_PopAndAppend(ctx);
        drop_item = drop_item->next_item;
    }
    M_PopAndSet(ctx, "carried_items");

    switch (item->object_id) {
    case O_FLAME_EMITTER:
        if (item->data != nullptr) {
            const int32_t effect_num =
                fx_order->id_map[(int32_t)(intptr_t)item->data - 1];
            // TR1X <4.16, TR2X <1.6
            M_WriteNum(ctx, "fx_num", effect_num);
            M_PushObject(ctx);
            M_WriteNum(ctx, "fx_num", effect_num);
            M_PopAndSet(ctx, "data");
        }
        break;

    case O_BACON_LARA:
        if (item->data != nullptr) {
            const int32_t status = (int32_t)(intptr_t)item->priv;
            // TR1X <4.16, TR2X <1.6
            M_WriteNum(ctx, "bl_status", status);
            M_PushObject(ctx);
            M_WriteNum(ctx, "status", status);
            M_PopAndSet(ctx, "data");
        }
        break;

    case O_MOVABLE_BLOCK_1:
    case O_MOVABLE_BLOCK_2:
    case O_MOVABLE_BLOCK_3:
    case O_MOVABLE_BLOCK_4:
        if (item->data != nullptr) {
            const MOVABLE_BLOCK_INFO *const data = item->data;
            M_PushObject(ctx);
            M_WriteNum(ctx, "counter_rot_0", data->counter_rot[0]);
            M_WriteNum(ctx, "counter_rot_1", data->counter_rot[1]);
            M_WriteNum(ctx, "counter_rot_2", data->counter_rot[2]);
            M_WriteNum(ctx, "original_rot", data->original_rot);
            M_WriteNum(ctx, "gravity_frames", data->gravity_frames);
            M_WriteBool(ctx, "is_push_pull", data->is_push_pull);
            M_WriteBool(ctx, "is_forced_moving", data->is_forced_moving);
            M_WriteXYZ32(ctx, "linked", data->linked.pos);
            M_PopAndSet(ctx, "data");
        }
        break;

    case O_SLIDING_PILLAR:
        if (item->data != nullptr) {
            const SLIDING_PILLAR_INFO *const data = item->data;
            M_PushObject(ctx);
            M_WriteXYZ32(ctx, "linked", data->linked.pos);
            M_PopAndSet(ctx, "data");
        }
        break;

    case O_BOAT: {
        if (item->data != nullptr) {
            const BOAT_INFO *const data = (BOAT_INFO *)item->data;
            M_PushObject(ctx);
            M_WriteNum(ctx, "boat_turn", data->boat_turn);
            M_WriteNum(ctx, "left_fallspeed", data->left_fallspeed);
            M_WriteNum(ctx, "right_fallspeed", data->right_fallspeed);
            M_WriteNum(ctx, "tilt_angle", data->tilt_angle);
            M_WriteNum(ctx, "extra_rotation", data->extra_rotation);
            M_WriteNum(ctx, "water", data->water);
            M_WriteNum(ctx, "pitch", data->pitch);
            M_PopAndSet(ctx, "data");
        }
        break;
    }

    case O_SKIDOO_FAST: {
        if (item->data != nullptr) {
            const SKIDOO_INFO *const data = (SKIDOO_INFO *)item->data;
            M_PushObject(ctx);
            M_WriteNum(ctx, "track_mesh", data->track_mesh);
            M_WriteNum(ctx, "skidoo_turn", data->skidoo_turn);
            M_WriteNum(ctx, "left_fallspeed", data->left_fallspeed);
            M_WriteNum(ctx, "right_fallspeed", data->right_fallspeed);
            M_WriteNum(ctx, "momentum_angle", data->momentum_angle);
            M_WriteNum(ctx, "extra_rotation", data->extra_rotation);
            M_WriteNum(ctx, "pitch", data->pitch);
            M_PopAndSet(ctx, "data");
        }
        break;
    }

    case O_LIFT: {
        if (item->data != nullptr) {
            const LIFT_INFO *const data = (LIFT_INFO *)item->data;
            M_PushObject(ctx);
            M_WriteNum(ctx, "start_height", data->start_height);
            M_WriteNum(ctx, "wait_time", data->wait_time);
            M_WriteBool(ctx, "is_moving", data->is_moving);
            for (int32_t j = 0; j < LIFT_NUM_SECTORS; j++) {
                const char *const pos_key = String_FormatStatic("linked_%d", j);
                M_WriteXYZ32(ctx, pos_key, data->linked[j].pos);
            }
            M_PopAndSet(ctx, "data");
        }
        break;
    }

    default:
        break;
    }
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

void Savegame_BSON_DumpEffects(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    M_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    M_PushArray(ctx);
    for (int16_t link_num = Effect_GetActiveNum(); link_num != NO_ITEM;
         link_num = Effect_Get(link_num)->next_active) {
        EFFECT *const effect = Effect_Get(link_num);
        if (Object_ToGameID(effect->object_id) == -1) {
            continue;
        }
        M_PushObject(ctx);
        M_WriteXYZ32(ctx, "pos", effect->pos);
        M_WriteXYZ16(ctx, "rot", effect->rot);
        // TR1X <4.16
        M_WriteNum(ctx, "x", effect->pos.x);
        M_WriteNum(ctx, "y", effect->pos.y);
        M_WriteNum(ctx, "z", effect->pos.z);
        M_WriteNum(ctx, "x_rot", effect->rot.x);
        M_WriteNum(ctx, "y_rot", effect->rot.y);
        M_WriteNum(ctx, "z_rot", effect->rot.z);

        // TR1X <4.16, TR2X<1.6
        M_WriteNum(ctx, "room_number", effect->room_num);
        M_WriteNum(ctx, "room_num", effect->room_num);

        // TR1X <4.16, TR2X<1.6
        M_WriteNum(ctx, "object_number", Object_ToGameID(effect->object_id));
        M_WriteNum(ctx, "object_id", Object_ToGameID(effect->object_id));

        M_WriteNum(ctx, "speed", effect->speed);
        M_WriteNum(ctx, "fall_speed", effect->fall_speed);
        M_WriteNum(ctx, "frame_number", effect->frame_num);
        M_WriteNum(ctx, "counter", effect->counter);
        M_WriteNum(ctx, "shade", effect->shade);
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "fx");
}

void Savegame_BSON_DumpInventory(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    M_PushObject(ctx);
    M_WriteNum(ctx, "pickup1", Inv_RequestItem(O_PICKUP_ITEM_1));
    M_WriteNum(ctx, "pickup2", Inv_RequestItem(O_PICKUP_ITEM_2));
    M_WriteNum(ctx, "puzzle1", Inv_RequestItem(O_PUZZLE_ITEM_1));
    M_WriteNum(ctx, "puzzle2", Inv_RequestItem(O_PUZZLE_ITEM_2));
    M_WriteNum(ctx, "puzzle3", Inv_RequestItem(O_PUZZLE_ITEM_3));
    M_WriteNum(ctx, "puzzle4", Inv_RequestItem(O_PUZZLE_ITEM_4));
    M_WriteNum(ctx, "key1", Inv_RequestItem(O_KEY_ITEM_1));
    M_WriteNum(ctx, "key2", Inv_RequestItem(O_KEY_ITEM_2));
    M_WriteNum(ctx, "key3", Inv_RequestItem(O_KEY_ITEM_3));
    M_WriteNum(ctx, "key4", Inv_RequestItem(O_KEY_ITEM_4));
    M_WriteNum(ctx, "leadbar", Inv_RequestItem(O_LEADBAR_ITEM));
    M_PopAndSet(ctx, "inventory");
}

void Savegame_BSON_DumpFlipmaps(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    M_PushObject(ctx);
    M_WriteBool(ctx, "status", Room_GetFlipStatus());
    M_WriteNum(ctx, "effect", Room_GetFlipEffect());
    M_WriteNum(ctx, "timer", Room_GetFlipTimer());
    M_PushArray(ctx);
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        M_PushNum(ctx, Room_GetFlipSlotFlags(i) >> 8);
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "table");
    M_PopAndSet(ctx, "flipmap");
}

void Savegame_BSON_DumpCameras(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    M_PushArray(ctx);
    JSON_ARRAY *const cameras_arr = JSON_ArrayNew();
    for (int32_t i = 0; i < Camera_GetFixedObjectCount(); i++) {
        const OBJECT_VECTOR *const object = Camera_GetFixedObject(i);
        M_PushNum(ctx, object->flags);
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "cameras");
}

void Savegame_BSON_DumpMusic(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    M_PushObject(ctx);
    M_PushArray(ctx);
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        M_PushNum(ctx, Music_GetTrackFlags(i));
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "flags");

    const MUSIC_ID current_track = Music_GetCurrentPlayingTrack();
    const MUSIC_ID current_ambient = Music_GetCurrentLoopedTrack();
    // TR1X >=4.16, TR2X – music/current/…
    M_PushObject(ctx);
    M_WriteNum(ctx, "current_track", current_track);
    M_WriteNum(ctx, "current_ambient", current_ambient);
    M_WriteNum(ctx, "timestamp", Music_GetTimestamp());
    M_PopAndSet(ctx, "current");

    // TR1X <4.16 - music/…
    M_WriteNum(ctx, "current_track", current_track);
    M_WriteNum(ctx, "current_ambient", current_ambient);
    M_WriteNum(ctx, "timestamp", Music_GetTimestamp());

    M_PopAndSet(ctx, "music");

    // TR1X <4.16
    M_PushArray(ctx);
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        M_PushNum(ctx, Music_GetTrackFlags(i));
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "music_track_flags");
}

void Savegame_BSON_DumpItems(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    Savegame_ProcessItemsBeforeSave();
    M_FX_ORDER fx_order;
    M_GetFXOrder(&fx_order);

    M_PushArray(ctx);
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        M_PushObject(ctx);
        const ITEM *const item = Item_Get(i);
        M_WriteItem(ctx, item, &fx_order);
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "items");
}
