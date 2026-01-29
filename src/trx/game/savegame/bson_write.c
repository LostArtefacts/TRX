#include <trx/config.h>
#include <trx/debug.h>
#include <trx/game/camera.h>
#include <trx/game/carrier.h>
#include <trx/game/effects.h>
#include <trx/game/game.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/music.h>
#include <trx/game/objects.h>
#include <trx/game/objects/traps/sliding_pillar.h>
#include <trx/game/objects/vehicles/skidoo_common.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/savegame.h>
#include <trx/game/savegame/bson.h>
#include <trx/game/weather_fx.h>
#include <trx/memory.h>
#include <trx/strings.h>
#include <trx/version.h>

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

static void M_PushString(M_CONTEXT *const ctx, const char *const value)
{
    M_PushValue(ctx, JSON_ValueFromString(value));
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

static void M_WriteString(
    M_CONTEXT *const ctx, const char *const key, const char *const value)
{
    M_PushString(ctx, value);
    M_PopAndSet(ctx, key);
}

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

static void M_WriteNumNZ_Int(
    M_CONTEXT *const ctx, const char *const key, const int32_t value)
{
    if (value != 0) {
        M_WriteNum_Int(ctx, key, value);
    }
}

static void M_WriteNumNZ_Double(
    M_CONTEXT *const ctx, const char *const key, const double value)
{
    if (value != 0.0) {
        M_WriteNum_Double(ctx, key, value);
    }
}

#define M_WriteNumNZ(ctx, key, value)                                          \
    _Generic(                                                                  \
        (value),                                                               \
        int8_t: M_WriteNumNZ_Int,                                              \
        uint8_t: M_WriteNumNZ_Int,                                             \
        int16_t: M_WriteNumNZ_Int,                                             \
        uint16_t: M_WriteNumNZ_Int,                                            \
        int32_t: M_WriteNumNZ_Int,                                             \
        uint32_t: M_WriteNumNZ_Int,                                            \
        float: M_WriteNumNZ_Double,                                            \
        double: M_WriteNumNZ_Double)(ctx, key, value)

static int32_t M_GetMusicTrackFlagsCount(void)
{
    int32_t last_index = -1;
    for (int32_t i = 0; i < MAX_MUSIC_TRACKS; i++) {
        const uint16_t flags = Music_GetTrackFlags(i);
        if (flags != 0) {
            last_index = i;
        }
    }
    return last_index + 1;
}

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
        M_WriteNum(ctx, "prev_frame_num", item->prev_frame_num);
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
        M_WriteNumNZ(ctx, "ai_bits", item->ai_bits);
        M_WriteNumNZ(ctx, "ai_tag", item->ai_tag);
        if (obj->intelligent && item->data != nullptr) {
            const CREATURE *const creature = item->data;
            M_WriteNum(ctx, "head_rot", creature->head_rotation);
            M_WriteNum(ctx, "neck_rot", creature->neck_rotation);
            M_WriteNum(ctx, "max_turn", creature->maximum_turn);
            M_WriteNum(ctx, "creature_flags", creature->flags);
            M_WriteNum(ctx, "creature_mood", creature->mood);
            M_PushObject(ctx);
            M_WriteBool(ctx, "alerted", creature->alerted);
            M_WriteBool(ctx, "head_left", creature->head_left);
            M_WriteBool(ctx, "head_right", creature->head_right);
            M_WriteBool(ctx, "reached_goal", creature->reached_goal);
            M_WriteBool(ctx, "patrol_2", creature->patrol_2);
            M_WriteBool(ctx, "hurt_by_lara", creature->hurt_by_lara);
            M_WriteNum(ctx, "damage_from_lara", creature->damage_from_lara);
            M_PushArray(ctx);
            for (int32_t i = 0; i < 4; i++) {
                M_PushNum(ctx, creature->joint_rotation[i]);
                M_PopAndAppend(ctx);
            }
            M_PopAndSet(ctx, "joint_rotations");
            M_PopAndSet(ctx, "creature");
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
        M_WriteNum(ctx, "spawn_num", drop_item->spawn_num);
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
    case O_FLAME_EMITTER_BIG:
    case O_FLAME_EMITTER_SMALL:
    case O_FLAME_EMITTER_JET:
    case O_FLAME_EMITTER_SIDE:
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

    case O_SLIDING_PILLAR:
        if (item->data != nullptr) {
            const SLIDING_PILLAR_INFO *const data = item->data;
            M_PushObject(ctx);
            M_WriteXYZ32(ctx, "linked", data->linked.pos);
            M_PopAndSet(ctx, "data");
        }
        break;

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

    default:
        break;
    }

    if (obj->priv_size > 0 && obj->priv_save_func != nullptr) {
        M_PushObject(ctx);
        JSON_OBJECT *const priv_root = JSON_ValueAsObject(ctx->current);
        ASSERT(priv_root != nullptr);
        obj->priv_save_func(item, priv_root);
        M_PopAndSet(ctx, "priv");
    }
}

static void M_WriteArm(
    M_CONTEXT *const ctx, const char *const key, const LARA_ARM *const arm)
{
    ASSERT(arm != nullptr);
    M_PushObject(ctx);
    M_WriteNum(ctx, "anim_num", arm->anim_num);
    M_WriteNum(ctx, "frame_num", arm->frame_num);
    M_WriteNum(ctx, "lock", arm->lock);
    M_WriteNum(ctx, "flash_gun", arm->flash_gun);
    M_WriteXYZ16(ctx, "rot", arm->rot);
    M_WriteNum(ctx, "x_rot", arm->rot.x);
    M_WriteNum(ctx, "y_rot", arm->rot.y);
    M_WriteNum(ctx, "z_rot", arm->rot.z);
    M_PopAndSet(ctx, key);
}

static void M_WriteAmmo(
    M_CONTEXT *const ctx, const char *const key, const AMMO_INFO *const ammo)
{
    ASSERT(ammo != nullptr);
    M_PushObject(ctx);
    M_WriteNum(ctx, "ammo", ammo->ammo);
    M_PopAndSet(ctx, key);
}

static void M_WriteLOT(M_CONTEXT *const ctx, const LOT_INFO *const lot)
{
    ASSERT(lot != nullptr);
    M_WriteNum(ctx, "head", lot->head);
    M_WriteNum(ctx, "tail", lot->tail);
    M_WriteNum(ctx, "search_num", lot->search_num);
    M_WriteNum(ctx, "block_mask", lot->setup.block_mask);
    M_WriteNum(ctx, "step", lot->setup.step);
    M_WriteNum(ctx, "drop", lot->setup.drop);
    M_WriteNum(ctx, "fly", lot->setup.fly);
    M_WriteNum(ctx, "zone_count", lot->zone_count);
    M_WriteNum(ctx, "target_box", lot->target_box);
    M_WriteNum(ctx, "required_box", lot->required_box);
    M_WriteNum(ctx, "x", lot->target.x);
    M_WriteNum(ctx, "y", lot->target.y);
    M_WriteNum(ctx, "z", lot->target.z);
}

static void M_WriteResumeInfo(
    M_CONTEXT *const ctx, const RESUME_INFO *const resume)
{
    M_WriteBool(ctx, "available", resume->flags.available);
    M_WriteBool(ctx, "level_completed", resume->level_completed);
    M_WriteNum(ctx, "prev_level", resume->prev_level);

    M_WriteBool(ctx, "hurt_allies", resume->hurt_allies);

    M_WriteNum(ctx, "lara_hitpoints", resume->lara_hitpoints);
    M_WriteNum(ctx, "pistol_ammo", resume->pistol_ammo);
    M_WriteNum(ctx, "shotgun_ammo", resume->shotgun_ammo);
    M_WriteNum(ctx, "magnum_ammo", resume->magnum_ammo);
    M_WriteNum(ctx, "autos_ammo", resume->autos_ammo);
    M_WriteNum(ctx, "desert_eagle_ammo", resume->desert_eagle_ammo);
    M_WriteNum(ctx, "uzi_ammo", resume->uzi_ammo);
    M_WriteNum(ctx, "m16_ammo", resume->m16_ammo);
    M_WriteNum(ctx, "mp5_ammo", resume->mp5_ammo);
    M_WriteNum(ctx, "grenade_ammo", resume->grenade_ammo);
    M_WriteNum(ctx, "rocket_ammo", resume->rocket_ammo);
    M_WriteNum(ctx, "harpoon_ammo", resume->harpoon_ammo);
    M_WriteNum(ctx, "num_medis", resume->small_medipacks);
    M_WriteNum(ctx, "num_big_medis", resume->large_medipacks);
    M_WriteNum(ctx, "num_flares", resume->flares);
    M_WriteNum(ctx, "num_scions", resume->num_scions);
    M_WriteNum(ctx, "num_quest_item_1", resume->num_quest_item_1);
    M_WriteNum(ctx, "num_quest_item_2", resume->num_quest_item_2);
    M_WriteNum(ctx, "num_quest_item_3", resume->num_quest_item_3);
    M_WriteNum(ctx, "num_quest_item_4", resume->num_quest_item_4);
    M_WriteNum(ctx, "gun_status", resume->gun_status);
    M_WriteNum(ctx, "gun_type", resume->equipped_gun_type);
    M_WriteNum(ctx, "holsters_gun_type", resume->holsters_gun_type);
    M_WriteNum(ctx, "back_gun_type", resume->back_gun_type);

    // TR1X <4.16
    M_WriteBool(ctx, "got_pistols", resume->flags.has_pistols);
    M_WriteBool(ctx, "got_shotgun", resume->flags.has_shotgun);
    M_WriteBool(ctx, "got_magnums", resume->flags.has_magnums);
    M_WriteBool(ctx, "got_uzis", resume->flags.has_uzis);
    M_WriteBool(ctx, "has_pistols", resume->flags.has_pistols);
    M_WriteBool(ctx, "has_shotgun", resume->flags.has_shotgun);
    M_WriteBool(ctx, "has_magnums", resume->flags.has_magnums);
    M_WriteBool(ctx, "has_autos", resume->flags.has_autos);
    M_WriteBool(ctx, "has_desert_eagle", resume->flags.has_desert_eagle);
    M_WriteBool(ctx, "has_uzis", resume->flags.has_uzis);
    M_WriteBool(ctx, "has_m16", resume->flags.has_m16);
    M_WriteBool(ctx, "has_mp5", resume->flags.has_mp5);
    M_WriteBool(ctx, "has_grenade", resume->flags.has_grenade);
    M_WriteBool(ctx, "has_rocket", resume->flags.has_rocket);
    M_WriteBool(ctx, "has_harpoon", resume->flags.has_harpoon);

    M_WriteBool(ctx, "costume", resume->flags.costume);
    M_WriteNum(ctx, "timer", resume->stats.timer);
    M_WriteNum(ctx, "kills", resume->stats.kill_count);
    M_WriteNum(ctx, "secrets", resume->stats.secret_flags);
    M_WriteNum(ctx, "pickups", resume->stats.pickup_count);
    M_WriteNum(ctx, "ammo_hits", resume->stats.ammo_hits);
    M_WriteNum(ctx, "ammo_used", resume->stats.ammo_used);
    M_WriteNum(ctx, "distance_travelled", resume->stats.distance_travelled);
    M_WriteNum(ctx, "medipacks_used", resume->stats.medipacks_used);
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

JSON_VALUE *Savegame_BSON_GetWriteRoot(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    ASSERT(ctx->stack[0] != nullptr);
    return ctx->stack[0];
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
    M_WriteNum(ctx, "quest1", Inv_RequestItem(O_QUEST_ITEM_1));
    M_WriteNum(ctx, "quest2", Inv_RequestItem(O_QUEST_ITEM_2));
    M_WriteNum(ctx, "quest3", Inv_RequestItem(O_QUEST_ITEM_3));
    M_WriteNum(ctx, "quest4", Inv_RequestItem(O_QUEST_ITEM_4));
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
    const int32_t track_flag_count = M_GetMusicTrackFlagsCount();
    M_PushObject(ctx);
    M_PushArray(ctx);
    for (int32_t i = 0; i < track_flag_count; i++) {
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
    for (int32_t i = 0; i < track_flag_count; i++) {
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

void Savegame_BSON_DumpLara(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    ASSERT(lara != nullptr);

    M_PushObject(ctx);

    M_WriteNum(ctx, "item_number", lara->item_num);
    M_WriteNum(ctx, "gun_status", lara->gun_status);
    M_WriteNum(ctx, "gun_type", lara->gun_type);
    M_WriteNum(ctx, "request_gun_type", lara->request_gun_type);
    M_WriteNum(ctx, "last_gun_type", lara->last_gun_type);
    M_WriteNum(ctx, "back_gun_obj_id", Object_ToGameID(lara->back_gun_obj_id));
    M_WriteNum(ctx, "calc_fall_speed", lara->calc_fall_speed);
    M_WriteNum(ctx, "water_status", lara->water_status);
    M_WriteBool(ctx, "climb_status", lara->climb_status);
    M_WriteBool(ctx, "is_crouched", lara->is_crouched);
    M_WriteBool(ctx, "keep_crouched", lara->keep_crouched);

    M_WriteNum(ctx, "pose_count", lara->pose_count);
    M_WriteNum(ctx, "hit_frame", lara->hit_frame);
    M_WriteNum(ctx, "hit_direction", lara->hit_direction);
    M_WriteNum(ctx, "hit_effect_count", lara->hit_effect_count);
    M_WriteNum(
        ctx, "hit_effect",
        lara->hit_effect ? Effect_GetNum(lara->hit_effect) : 0);

    M_WriteNum(ctx, "air", lara->air);
    M_WriteNum(ctx, "sprint_timer", lara->sprint_timer);
    M_WriteNum(ctx, "exposure_timer", lara->exposure_timer);
    M_WriteNum(ctx, "poison_timer", lara->poison_timer);
    M_WriteNum(ctx, "dive_count", lara->dive_timer);
    M_WriteNum(ctx, "death_count", lara->death_timer);

    M_WriteNum(ctx, "current_active", lara->current.active);
    M_WriteNum(ctx, "current_vel_x", lara->current.vel.x);
    M_WriteNum(ctx, "current_vel_z", lara->current.vel.z);
    M_WriteBool(ctx, "burn", lara->burn);
    M_WriteNum(ctx, "water_surface_dist", lara->water_surface_dist);

    M_WriteNum(ctx, "flare_age", lara->flare.age);
    M_WriteNum(ctx, "flare_frame", lara->flare.frame_num);
    M_WriteBool(ctx, "flare_control_left", lara->flare.control);
    M_WriteBool(ctx, "extra_anim", lara->extra_anim);
    M_WriteNum(ctx, "vehicle_item_number", Lara_Vehicle_GetIndex());

    M_WriteNum(ctx, "mesh_effects", lara->mesh_effects);
    M_PushArray(ctx);
    for (int32_t i = 0; i < LM_NUMBER_OF; i++) {
        M_PushNum(ctx, Object_GetMeshOffset(lara->mesh_ptrs[i]));
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "meshes");

    M_WriteNum(ctx, "target_angle1", lara->target_angles[0]);
    M_WriteNum(ctx, "target_angle2", lara->target_angles[1]);
    M_WriteNum(ctx, "turn_rate", lara->turn_rate);
    M_WriteNum(ctx, "move_angle", lara->move_angle);
    M_WriteXYZ16(ctx, "head_rot", lara->head_rot);
    // TR1X <4.16
    M_WriteNum(ctx, "head_rot.y", lara->head_rot.y);
    M_WriteNum(ctx, "head_rot.x", lara->head_rot.x);
    M_WriteNum(ctx, "head_rot.z", lara->head_rot.z);
    M_WriteXYZ16(ctx, "torso_rot", lara->torso_rot);
    // TR1X <4.16
    M_WriteNum(ctx, "torso_rot.y", lara->torso_rot.y);
    M_WriteNum(ctx, "torso_rot.x", lara->torso_rot.x);
    M_WriteNum(ctx, "torso_rot.z", lara->torso_rot.z);
    M_WriteXYZ32(ctx, "last_pos", lara->last_pos);
    // TR1X <4.16
    M_WriteNum(ctx, "last_pos.x", lara->last_pos.x);
    M_WriteNum(ctx, "last_pos.y", lara->last_pos.y);
    M_WriteNum(ctx, "last_pos.z", lara->last_pos.z);

    M_WriteArm(ctx, "left_arm", &lara->left_arm);
    M_WriteArm(ctx, "right_arm", &lara->right_arm);
    M_WriteAmmo(ctx, "pistols", &lara->pistol_ammo);
    M_WriteAmmo(ctx, "shotgun", &lara->shotgun_ammo);
    M_WriteAmmo(ctx, "magnums", &lara->magnum_ammo);
    M_WriteAmmo(ctx, "autos", &lara->autos_ammo);
    M_WriteAmmo(ctx, "desert_eagle", &lara->desert_eagle_ammo);
    M_WriteAmmo(ctx, "uzis", &lara->uzi_ammo);
    M_WriteAmmo(ctx, "harpoon", &lara->harpoon_ammo);
    M_WriteAmmo(ctx, "grenade", &lara->grenade_ammo);
    M_WriteAmmo(ctx, "rocket", &lara->rocket_ammo);
    M_WriteAmmo(ctx, "m16", &lara->m16_ammo);
    M_WriteAmmo(ctx, "mp5", &lara->mp5_ammo);

    if (lara->gun_item_num != NO_ITEM) {
        M_PushObject(ctx);
        const ITEM *const weapon_item = Item_Get(lara->gun_item_num);
        M_WriteNum(ctx, "obj_id", Object_ToGameID(weapon_item->object_id));
        M_WriteNum(ctx, "anim_num", weapon_item->anim_num);
        M_WriteNum(ctx, "frame_num", weapon_item->frame_num);
        M_WriteNum(ctx, "current_anim_state", weapon_item->current_anim_state);
        M_WriteNum(ctx, "goal_anim_state", weapon_item->goal_anim_state);
        M_PopAndSet(ctx, "weapon");
    }

    M_PushObject(ctx);
    M_WriteNum(ctx, "item_num", lara->interact_target.item_num);
    M_WriteNum(ctx, "move_count", lara->interact_target.move_count);
    M_WriteBool(ctx, "is_moving", lara->interact_target.is_moving);
    M_PopAndSet(ctx, "interact_target");
    // TR1X <4.16, TR2X <1.6
    M_WriteNum(ctx, "interact_target.item_num", lara->interact_target.item_num);
    M_WriteNum(
        ctx, "interact_target.move_count", lara->interact_target.move_count);
    M_WriteBool(
        ctx, "interact_target.is_moving", lara->interact_target.is_moving);

    if (g_TRVersion == 1) {
        // TR1X <4.16
        M_PushObject(ctx);
        M_WriteLOT(ctx, &lara->lot);
        M_PopAndSet(ctx, "lot");
    }

    M_PopAndSet(ctx, "lara");
}

void Savegame_BSON_DumpResumeInfoList(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    const int32_t count = GF_GetLevelTable(GFLT_MAIN)->count;
    M_PushArray(ctx);
    for (int32_t i = 0; i < count; i++) {
        const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
        const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
        M_PushObject(ctx);
        M_WriteResumeInfo(ctx, resume);
        M_PopAndAppend(ctx);
    }
    M_PopAndSet(ctx, "resume_info");

    if (g_TRVersion == 1) {
        // < TR1X <4.16
        M_PushArray(ctx);
        for (int32_t i = 0; i < count; i++) {
            const GF_LEVEL *const level = GF_GetLevel(GFLT_MAIN, i);
            const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);
            M_PushObject(ctx);
            M_WriteResumeInfo(ctx, resume);
            M_PopAndAppend(ctx);
        }
        M_PopAndSet(ctx, "current_info");
    }
}

void Savegame_BSON_DumpMisc(SAVEGAME_BSON_WRITE_CONTEXT *const ctx)
{
    const GF_LEVEL *const level = Game_GetCurrentLevel();
    const RESUME_INFO *const resume = Savegame_GetCurrentInfo(level);

    M_PushObject(ctx);
    M_WriteString(ctx, "game_version", g_TRXVersion);
    M_WriteNum(ctx, "bonus_flag", Game_GetBonusFlag());
    M_WriteNum(ctx, "death_count", resume->stats.death_count);
    M_WriteBool(ctx, "are_monks_angry", Creature_AreAlliesHostile());
    M_WriteNum(ctx, "sunset_timer", Output_GetTimeInGame());
    M_WriteNum(ctx, "weather_type", WeatherFX_GetWeather());
    M_PopAndSet(ctx, "misc");

    M_WriteString(
        ctx, "level_title", level->title != nullptr ? level->title : "");
    M_WriteNum(ctx, "save_counter", Savegame_GetCounter());
    M_WriteNum(ctx, "level_num", level->num);
}
