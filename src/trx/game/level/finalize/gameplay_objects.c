#include <trx/game/items.h>
#include <trx/game/items/walkable.h>
#include <trx/game/lara.h>
#include <trx/game/level/context.h>
#include <trx/game/level/finalize.h>
#include <trx/game/level/format/format.h>
#include <trx/game/lua.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/vars.h>
#include <trx/game/rooms.h>
#include <trx/game/rope.h>
#include <trx/version.h>

static uint8_t M_GetAIBit(const OBJECT_ID object_id)
{
    switch (object_id) {
        // clang-format off
    case O_AI_GUARD:    return AI_GUARD;
    case O_AI_AMBUSH:   return AI_AMBUSH;
    case O_AI_PATROL_1: return AI_PATROL_1;
    case O_AI_MODIFY:   return AI_MODIFY;
    case O_AI_FOLLOW:   return AI_FOLLOW;
    // clang-format on
    default:
        return 0;
    }
}

static void M_AssignTR123AIBits(const LEVEL_CONTEXT *const ctx)
{
    const int32_t item_count = Item_GetLevelCount();
    for (int32_t i = 0; i < item_count; i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (!obj->intelligent || item->room_num == NO_ROOM) {
            continue;
        }

        ROOM *const room = Room_Get(item->room_num);
        int16_t ai_item_num = room->item_num;
        while (ai_item_num != NO_ITEM) {
            ITEM *const ai_item = Item_Get(ai_item_num);
            const int16_t next_num = ai_item->next_item;
            const uint8_t ai_bit = M_GetAIBit(ai_item->object_id);
            if (ai_bit != 0 && (item->ai_bits & ai_bit) == 0
                && ai_item->pos.x == item->pos.x
                && ai_item->pos.z == item->pos.z) {
                item->ai_bits |= ai_bit;
                item->ai_tag = ai_item->rot.y;
                Item_Kill(ai_item_num);
                ai_item->room_num = NO_ROOM;
            }
            ai_item_num = next_num;
        }
    }
}

static void M_AssignTR4AIBits(const LEVEL_CONTEXT *const ctx)
{
    const LEVEL_CONTEXT_INFO *const info = &ctx->info;
    const int32_t item_count = Item_GetLevelCount();
    for (int32_t i = 0; i < item_count; i++) {
        ITEM *const item = Item_Get(i);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (!obj->intelligent || item->room_num == NO_ROOM) {
            continue;
        }

        for (int32_t j = 0; j < info->tr4.ai_item_count; j++) {
            const LEVEL_TR4_AI_ITEM_INFO *const ai_item =
                &info->tr4.ai_items[j];
            const OBJECT_ID ai_object_id =
                Object_FromGameID(ai_item->object_id);
            const uint8_t ai_bit = M_GetAIBit(ai_object_id);
            if (ai_bit == 0 || (item->ai_bits & ai_bit) != 0
                || ai_item->room_num != item->room_num
                || ai_item->pos.x != item->pos.x
                || ai_item->pos.z != item->pos.z) {
                continue;
            }

            item->ai_bits |= ai_bit;
            item->ai_tag = ai_item->y_rot;
        }
    }
}

static void M_AssignAIBits(const LEVEL_CONTEXT *const ctx)
{
    if (ctx->loader->game_version == 4) {
        M_AssignTR4AIBits(ctx);
    } else {
        M_AssignTR123AIBits(ctx);
    }
}

static void M_CloneTR4InvOptionObjects(void)
{
    // TR4 level files have no separate inventory display slots, so let each
    // unloaded *_OPTION object borrow the model of its pickup counterpart.
    for (const GAME_OBJECT_PAIR *pair = g_ItemToInvObjectMap;
         pair->key_id != NO_OBJECT; pair++) {
        OBJECT *const option_obj = Object_Get(pair->value_id);
        const OBJECT *const item_obj = Object_Get(pair->key_id);
        if (option_obj->loaded || !item_obj->loaded) {
            continue;
        }
        option_obj->mesh_count = item_obj->mesh_count;
        option_obj->mesh_idx = item_obj->mesh_idx;
        option_obj->bone_idx = item_obj->bone_idx;
        option_obj->frame_ofs = item_obj->frame_ofs;
        option_obj->frame_base = item_obj->frame_base;
        option_obj->anim_idx = item_obj->anim_idx;
        option_obj->anim_count = item_obj->anim_count;
        option_obj->loaded = true;
    }
}

static void M_PrepareTR4Items(LEVEL_CONTEXT *const ctx)
{
    if (ctx->loader->layout != LEVEL_FORMAT_LAYOUT_TR4) {
        return;
    }

    M_CloneTR4InvOptionObjects();

    LEVEL_CONTEXT_INFO *const info = &ctx->info;
    for (int32_t i = 0; i < info->tr4.item_count && i < Item_GetLevelCount();
         i++) {
        ITEM *const item = Item_Get(i);
        const LEVEL_TR4_ITEM_INFO *const tr4_item = &info->tr4.items[i];
        if (item->object_id == NO_OBJECT) {
            item->object_id = O_CAMERA_TARGET;
            item->room_num = NO_ROOM;
        }

        ObjectProperty_SetItemValueRaw(
            item, "ocb",
            (OBJECT_PROPERTY_VALUE) {
                .type = OBJECT_PROPERTY_TYPE_INT,
                .as_int = tr4_item->ocb,
            });
    }
}

void Level_Finalize_LoadObjectsAndItems(LEVEL_CONTEXT *const ctx)
{
    // Object and item setup/initialisation must take place after injections
    // have been processed. A cached item count must be used as individual
    // initialisations may increment the total item count.
    Object_SetupAllObjects();
    Walkable_ResetLevel();
    // Must precede Item_Initialise() below, which creates the ropes.
    Rope_Reset();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ObjectProperty_ResetItem(Item_Get(i));
    }

    M_PrepareTR4Items(ctx);

    Lua_FireEventInt32(LUA_EVENT_BEFORE_ITEM_SETUP, GF_GetCurrentLevel()->num);

    const int32_t item_count = Item_GetLevelCount();
    for (int32_t i = 0; i < item_count; i++) {
        if (Item_Get(i)->room_num == NO_ROOM) {
            continue;
        }
        Item_Initialise(i);
    }

    // Must take place after item initialization.
    Level_Finalize_LoadWalkables(ctx);

    M_AssignAIBits(ctx);
    Lara_State_Initialise();
}

void Level_Finalize_LoadWalkables(LEVEL_CONTEXT *const ctx)
{
    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        const OBJECT *const obj = Object_Get(item->object_id);
        if (obj->add_walkable_func != nullptr) {
            obj->add_walkable_func(item_num);
        }
    }
}
