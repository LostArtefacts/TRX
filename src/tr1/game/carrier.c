#include "game/carrier.h"

#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/objects/vars.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/log.h>

#include <stdbool.h>
#include <stddef.h>

#define DROP_FAST_RATE GRAVITY
#define DROP_SLOW_RATE 1
#define DROP_FAST_TURN (PHD_DEGREE * 5)
#define DROP_SLOW_TURN (PHD_DEGREE * 3)

static int16_t m_AnimatingCount = 0;

static ITEM *M_GetCarrier(int16_t item_num);
static void M_AnimateDrop(CARRIED_ITEM *item);

static const GAME_OBJECT_PAIR m_LegacyMap[] = {
    { O_PIERRE, O_SCION_ITEM_2 }, { O_COWBOY, O_MAGNUM_ITEM },
    { O_SKATEKID, O_UZI_ITEM },   { O_BALDY, O_SHOTGUN_ITEM },
    { NO_OBJECT, NO_OBJECT },
};

static ITEM *M_GetCarrier(const int16_t item_num)
{
    if (item_num < 0 || item_num >= g_LevelItemCount) {
        return NULL;
    }

    // Allow carried items to be allocated to holder objects (pods/statues),
    // but then have those items dropped by the actual creatures within.
    ITEM *item = Item_Get(item_num);
    if (Object_IsObjectType(item->object_id, g_PlaceholderObjects)) {
        const int16_t child_item_num = *(int16_t *)item->data;
        item = Item_Get(child_item_num);
    }

    const OBJECT *object = Object_GetObject(item->object_id);
    if (!object->loaded) {
        return NULL;
    }

    return item;
}

static void M_AnimateDrop(CARRIED_ITEM *const item)
{
    if (item->status != DS_FALLING) {
        return;
    }

    ITEM *const pickup = Item_Get(item->spawn_num);
    int16_t room_num = pickup->room_num;
    const SECTOR *const sector =
        Room_GetSector(pickup->pos.x, pickup->pos.y, pickup->pos.z, &room_num);
    const int16_t height =
        Room_GetHeight(sector, pickup->pos.x, pickup->pos.y, pickup->pos.z);
    const bool in_water = g_RoomInfo[pickup->room_num].flags & RF_UNDERWATER;

    if (sector->portal_room.pit == NO_ROOM && pickup->pos.y >= height) {
        item->status = DS_DROPPED;
        pickup->pos.y = height;
        pickup->fall_speed = 0;
        m_AnimatingCount--;
    } else {
        pickup->fall_speed += (!in_water && pickup->fall_speed < FASTFALL_SPEED)
            ? DROP_FAST_RATE
            : DROP_SLOW_RATE;
        pickup->pos.y += pickup->fall_speed;
        pickup->rot.y += in_water ? DROP_SLOW_TURN : DROP_FAST_TURN;

        if (sector->portal_room.pit != NO_ROOM
            && pickup->pos.y > sector->floor.height) {
            room_num = sector->portal_room.pit;
        }
    }

    if (room_num != pickup->room_num) {
        Item_NewRoom(item->spawn_num, room_num);
    }

    // Track animating status in the carrier for saving/loading.
    item->pos = pickup->pos;
    item->rot = pickup->rot;
    item->room_num = pickup->room_num;
    item->fall_speed = pickup->fall_speed;
}

void Carrier_InitialiseLevel(const int32_t level_num)
{
    m_AnimatingCount = 0;

    int32_t total_item_count = g_LevelItemCount;
    const GAMEFLOW_LEVEL level = g_GameFlow.levels[level_num];
    for (int32_t i = 0; i < level.item_drops.count; i++) {
        const GAMEFLOW_DROP_ITEM_DATA *const data = &level.item_drops.data[i];

        ITEM *const item = M_GetCarrier(data->enemy_num);
        if (!item) {
            LOG_WARNING("%d does not refer to a loaded item", data->enemy_num);
            continue;
        }

        if (total_item_count + data->count > MAX_ITEMS) {
            LOG_WARNING("Too many items being loaded");
            return;
        }

        if (item->carried_item) {
            LOG_WARNING("Item %d is already carrying", data->enemy_num);
            continue;
        }

        if (!Object_IsObjectType(item->object_id, g_EnemyObjects)) {
            LOG_WARNING(
                "Item %d of type %d cannot carry items", data->enemy_num,
                item->object_id);
            continue;
        }

        item->carried_item =
            GameBuf_Alloc(sizeof(CARRIED_ITEM) * data->count, GBUF_ITEMS);
        CARRIED_ITEM *drop = item->carried_item;
        for (int32_t i = 0; i < data->count; i++) {
            drop->object_id = data->object_ids[i];
            drop->spawn_num = NO_ITEM;
            drop->room_num = NO_ROOM;
            drop->fall_speed = 0;

            if (Object_IsObjectType(drop->object_id, g_PickupObjects)) {
                drop->status = DS_CARRIED;
                total_item_count++;
            } else {
                LOG_WARNING(
                    "Items of type %d cannot be carried", drop->object_id);
                drop->object_id = NO_OBJECT;
                drop->status = DS_COLLECTED;
            }

            if (i < data->count - 1) {
                drop->next_item = drop + 1;
                drop++;
            } else {
                drop->next_item = NULL;
            }
        }
    }
}

int32_t Carrier_GetItemCount(const int16_t item_num)
{
    const ITEM *const carrier = M_GetCarrier(item_num);
    if (!carrier) {
        return 0;
    }

    const CARRIED_ITEM *item = carrier->carried_item;
    int32_t count = 0;
    while (item) {
        if (item->object_id != NO_OBJECT) {
            count++;
        }
        item = item->next_item;
    }

    return count;
}

DROP_STATUS Carrier_GetSaveStatus(const CARRIED_ITEM *item)
{
    // This allows us to save drops as still being carried to allow accurate
    // placement again in Carrier_TestItemDrops on load.
    if (item->status == DS_DROPPED) {
        const ITEM *const pickup = Item_Get(item->spawn_num);
        return pickup->status == IS_INVISIBLE ? DS_COLLECTED : DS_CARRIED;
    } else if (item->status == DS_FALLING) {
        return DS_CARRIED;
    }

    return item->status;
}

void Carrier_TestItemDrops(const int16_t item_num)
{
    const ITEM *const carrier = Item_Get(item_num);
    CARRIED_ITEM *item = carrier->carried_item;
    if (carrier->hit_points > 0 || !item
        || (carrier->object_id == O_PIERRE
            && !(carrier->flags & IF_ONE_SHOT))) {
        return;
    }

    // The enemy is killed (plus is not runaway) and is carrying at
    // least one item. Ensure that each item has not already spawned,
    // convert guns to ammo if applicable, and spawn the items.
    do {
        if (item->status != DS_CARRIED) {
            continue;
        }

        GAME_OBJECT_ID object_id = item->object_id;
        if (g_GameFlow.convert_dropped_guns
            && Object_IsObjectType(object_id, g_GunObjects)
            && Inv_RequestItem(object_id) && object_id != O_PISTOL_ITEM) {
            object_id = Object_GetCognate(object_id, g_GunAmmoObjectMap);
        }

        item->spawn_num = Item_Spawn(carrier, object_id);
        item->status = DS_FALLING;
        m_AnimatingCount++;

        if (item->room_num != NO_ROOM) {
            // Handle reloading a save with a falling or landed item.
            ITEM *const pickup = Item_Get(item->spawn_num);
            pickup->pos = item->pos;
            pickup->fall_speed = item->fall_speed;
            if (pickup->room_num != item->room_num) {
                Item_NewRoom(item->spawn_num, item->room_num);
            }
        }

    } while ((item = item->next_item));
}

void Carrier_TestLegacyDrops(const int16_t item_num)
{
    const ITEM *const carrier = Item_Get(item_num);
    if (carrier->hit_points > 0) {
        return;
    }

    // Handle cases where legacy saves have been loaded. Ensure that
    // the OG enemy will still spawn items if Lara hasn't yet collected
    // them by using a test cognate in each case. Ensure also that
    // collected items do not re-spawn now or in future saves.
    const GAME_OBJECT_ID test_id =
        Object_GetCognate(carrier->object_id, m_LegacyMap);
    if (test_id == NO_OBJECT) {
        return;
    }

    if (!Inv_RequestItem(test_id)) {
        Carrier_TestItemDrops(item_num);
    } else {
        CARRIED_ITEM *item = carrier->carried_item;
        while (item) {
            // Simulate Lara having picked up the item.
            item->status = DS_COLLECTED;
            item = item->next_item;
        }
    }
}

void Carrier_AnimateDrops(void)
{
    if (!m_AnimatingCount) {
        return;
    }

    // Make items that spawn in mid-air or water gracefully fall to the floor.
    for (int32_t i = 0; i < g_LevelItemCount; i++) {
        const ITEM *const carrier = Item_Get(i);
        CARRIED_ITEM *item = carrier->carried_item;
        while (item) {
            M_AnimateDrop(item);
            item = item->next_item;
        }
    }
}
