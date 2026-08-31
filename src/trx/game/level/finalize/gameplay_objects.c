#include <trx/core/log.h>
#include <trx/core/utils.h>
#include <trx/game/anims.h>
#include <trx/game/game_buf.h>
#include <trx/game/inject.h>
#include <trx/game/items.h>
#include <trx/game/items/walkable.h>
#include <trx/game/lara.h>
#include <trx/game/level/context.h>
#include <trx/game/level/finalize.h>
#include <trx/game/level/format/format.h>
#include <trx/game/lua.h>
#include <trx/game/objects.h>
#include <trx/game/objects/links.h>
#include <trx/game/objects/property.h>
#include <trx/game/rooms.h>
#include <trx/game/rope.h>
#include <trx/version.h>

// TR4's AI objects outlive the load: a guide reads them all level long, by the
// OCB rather than by the ai_bits they also carry. A level parks one it has
// finished with in room 255, which is the removed marker rather than a room.
#define M_AI_OBJECT_NO_ROOM 255

// A guide stands a quarter of a sector in front of the marker it walks to,
// unless the marker asks otherwise. The original offsets the copy it makes of
// the marker; the offset depends only on the marker's own facing and flags,
// which never change, so it is applied to the marker itself.
//
// After the bits are assigned: those match a marker to a creature by position.
#define M_AI_OBJECT_NO_NUDGE 0x20
#define M_AI_OBJECT_NUDGE 256

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

// TR4 stores its AI objects in a list of their own rather than among the
// items. Turning them into items is what makes them the same thing they are in
// TR1-3: a creature standing on one takes its ai_bits from it, and one placed
// away from any creature stays behind as somewhere to walk to.
static void M_MaterialiseTR4AIObjects(LEVEL_CONTEXT *const ctx)
{
    if (ctx->loader->layout != LEVEL_FORMAT_LAYOUT_TR4) {
        return;
    }

    const LEVEL_CONTEXT_INFO *const info = &ctx->info;
    for (int32_t i = 0; i < info->tr4.ai_item_count; i++) {
        const LEVEL_TR4_AI_ITEM_INFO *const ai_item = &info->tr4.ai_items[i];
        if (ai_item->room_num == M_AI_OBJECT_NO_ROOM) {
            continue;
        }

        // A slot TRX has no catalog entry for is one nothing can be made of,
        // and an item pointing at it is an item nothing can initialise.
        const OBJECT_ID object_id = Object_SlotToID(ai_item->object_id);
        if (object_id == NO_OBJECT) {
            LOG_WARNING(
                "Skipping AI object with unknown slot: %d", ai_item->object_id);
            continue;
        }

        const int16_t item_num = Item_CreateLevelItem();
        if (item_num == NO_ITEM) {
            LOG_WARNING("No room left for the level's AI objects");
            return;
        }

        ITEM *const item = Item_Get(item_num);
        item->object_id = object_id;
        item->room_num = ai_item->room_num;
        item->pos = ai_item->pos;
        item->rot.y = ai_item->y_rot;
        item->box_num = ai_item->box_num;
        item->init_flags = ai_item->flags;
        ObjectProperty_SetItemValueRaw(
            item, "ocb",
            (TRX_VALUE) {
                .type = TVT_S32,
                .as_int = ai_item->ocb,
            });
    }
}

static void M_NudgeTR4AIObjects(const LEVEL_CONTEXT *const ctx)
{
    if (ctx->loader->layout != LEVEL_FORMAT_LAYOUT_TR4) {
        return;
    }
    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ITEM *const item = Item_Get(i);
        if (item->room_num == NO_ROOM || M_GetAIBit(item->object_id) == 0
            || (item->init_flags & M_AI_OBJECT_NO_NUDGE) != 0) {
            continue;
        }
        item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, M_AI_OBJECT_NUDGE);
    }
}

static void M_AssignAIBits(const LEVEL_CONTEXT *const ctx)
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
                TRX_VALUE ocb;
                if (ObjectProperty_GetItemValue(ai_item, "ocb", &ocb)) {
                    item->ai_ocb = ocb.as_int;
                }
                Item_Destroy(ai_item_num);
                ai_item->room_num = NO_ROOM;
            }
            ai_item_num = next_num;
        }
    }
}

static void M_CloneTR4InvOptionObjects(void)
{
    // TR4 level files have no separate inventory display slots, so let each
    // unloaded *_OPTION object borrow the model of its pickup counterpart.
    const int32_t count = ObjectLink_GetPairCount(OBJ_LINK_ITEM_TO_OPTION);
    for (int32_t i = 0; i < count; i++) {
        OBJECT_ID item_id;
        OBJECT_ID option_id;
        ObjectLink_GetPairAt(OBJ_LINK_ITEM_TO_OPTION, i, &item_id, &option_id);
        OBJECT *const option_obj = Object_Get(option_id);
        const OBJECT *const item_obj = Object_Get(item_id);
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
            (TRX_VALUE) {
                .type = TVT_S32,
                .as_int = tr4_item->ocb,
            });
    }
}

static void M_ComputeAnimBounds(void)
{
    CATALOG_FOR_EACH(CATALOG_OBJECTS, i)
    {
        OBJECT *const obj = Object_Get(i);
        BOUNDS_16 bounds = {
            .min = { INT16_MAX, INT16_MAX, INT16_MAX },
            .max = { INT16_MIN, INT16_MIN, INT16_MIN },
        };
        for (int32_t j = 0; j < obj->anim_count; j++) {
            const ANIM *const anim = Anim_GetAnim(obj->anim_idx + j);
            if (anim->frame_ptr == nullptr || anim->interpolation == 0) {
                continue;
            }
            const int32_t frame_count =
                (anim->frame_end - anim->frame_base) / anim->interpolation + 1;
            for (int32_t k = 0; k < frame_count; k++) {
                const BOUNDS_16 *const frame_bounds =
                    &anim->frame_ptr[k].bounds;
                bounds.min.x = MIN(bounds.min.x, frame_bounds->min.x);
                bounds.min.y = MIN(bounds.min.y, frame_bounds->min.y);
                bounds.min.z = MIN(bounds.min.z, frame_bounds->min.z);
                bounds.max.x = MAX(bounds.max.x, frame_bounds->max.x);
                bounds.max.y = MAX(bounds.max.y, frame_bounds->max.y);
                bounds.max.z = MAX(bounds.max.z, frame_bounds->max.z);
            }
        }
        obj->anim_bounds =
            bounds.min.x > bounds.max.x ? (BOUNDS_16) {} : bounds;
    }
}

void Level_Finalize_LoadObjectsAndItems(LEVEL_CONTEXT *const ctx)
{
    // Object and item setup/initialisation must take place after injections
    // have been processed. A cached item count must be used as individual
    // initialisations may increment the total item count.
    Object_SetupAllObjects();
    M_ComputeAnimBounds();
    Walkable_ResetLevel();
    // Must precede Item_Initialise() below, which creates the ropes.
    Rope_Reset();

    for (int32_t i = 0; i < Item_GetLevelCount(); i++) {
        ObjectProperty_ResetItem(Item_Get(i));
    }

    M_PrepareTR4Items(ctx);

    Inject_ApplyProperties();

    M_MaterialiseTR4AIObjects(ctx);

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
    M_NudgeTR4AIObjects(ctx);
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
