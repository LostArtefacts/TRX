#include "game/carrier.h"

#include "game/gamebuf.h"
#include "game/gameflow.h"
#include "game/inventory.h"
#include "game/items.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "log.h"

#include <stdbool.h>
#include <stddef.h>

#define NO_OBJECT O_NUMBER_OF

typedef struct GAME_OBJECT_PAIR {
    const GAME_OBJECT_ID key_id;
    const GAME_OBJECT_ID value_id;
} GAME_OBJECT_PAIR;

static ITEM_INFO *Carrier_GetCarrier(int16_t item_num);
static bool Carrier_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr);
static GAME_OBJECT_ID Carrier_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map);

static const GAME_OBJECT_ID m_CarrierObjects[] = {
    O_WOLF,   O_BEAR,   O_BAT,     O_LION,  O_LIONESS, O_PUMA,   O_APE,
    O_TREX,   O_RAPTOR, O_CENTAUR, O_MUMMY, O_LARSON,  O_PIERRE, O_SKATEKID,
    O_COWBOY, O_BALDY,  O_NATLA,   O_TORSO, NO_OBJECT
};

static const GAME_OBJECT_ID m_PlaceholderObjects[] = { O_STATUE, O_PODS,
                                                       O_BIG_POD, NO_OBJECT };

static const GAME_OBJECT_ID m_DropObjects[] = {
    O_GUN_ITEM,     O_SHOTGUN_ITEM,  O_MAGNUM_ITEM,   O_UZI_ITEM,
    O_SG_AMMO_ITEM, O_MAG_AMMO_ITEM, O_UZI_AMMO_ITEM, O_MEDI_ITEM,
    O_BIGMEDI_ITEM, O_PUZZLE_ITEM1,  O_PUZZLE_ITEM2,  O_PUZZLE_ITEM3,
    O_PUZZLE_ITEM4, O_KEY_ITEM1,     O_KEY_ITEM2,     O_KEY_ITEM3,
    O_KEY_ITEM4,    O_PICKUP_ITEM1,  O_PICKUP_ITEM2,  O_LEADBAR_ITEM,
    O_SCION_ITEM2,  NO_OBJECT
};

static const GAME_OBJECT_ID m_GunObjects[] = { O_SHOTGUN_ITEM, O_MAGNUM_ITEM,
                                               O_UZI_ITEM, NO_OBJECT };

static const GAME_OBJECT_PAIR m_GunAmmoMap[] = {
    { O_SHOTGUN_ITEM, O_SG_AMMO_ITEM },
    { O_MAGNUM_ITEM, O_MAG_AMMO_ITEM },
    { O_UZI_ITEM, O_UZI_AMMO_ITEM },
    { NO_OBJECT, NO_OBJECT },
};

static const GAME_OBJECT_PAIR m_LegacyMap[] = {
    { O_PIERRE, O_SCION_ITEM2 }, { O_COWBOY, O_MAGNUM_ITEM },
    { O_SKATEKID, O_UZI_ITEM },  { O_BALDY, O_SHOTGUN_ITEM },
    { NO_OBJECT, NO_OBJECT },
};

void Carrier_InitialiseLevel(int32_t level_num)
{
    int32_t total_item_count = g_LevelItemCount;
    GAMEFLOW_LEVEL level = g_GameFlow.levels[level_num];
    for (int i = 0; i < level.item_drops.count; i++) {
        GAMEFLOW_DROP_ITEM_DATA *data = &level.item_drops.data[i];

        ITEM_INFO *item = Carrier_GetCarrier(data->enemy_num);
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

        if (!Carrier_IsObjectType(item->object_number, m_CarrierObjects)) {
            LOG_WARNING(
                "Item %d of type %d cannot carry items", data->enemy_num,
                item->object_number);
            continue;
        }

        item->carried_item =
            GameBuf_Alloc(sizeof(CARRIED_ITEM) * data->count, GBUF_ITEMS);
        CARRIED_ITEM *drop = item->carried_item;
        for (int i = 0; i < data->count; i++) {
            drop->object_id = data->object_ids[i];
            drop->spawn_number = NO_ITEM;

            if (Carrier_IsObjectType(drop->object_id, m_DropObjects)) {
                drop->status = IS_NOT_ACTIVE;
                total_item_count++;
            } else {
                LOG_WARNING(
                    "Items of type %d cannot be carried", drop->object_id);
                drop->object_id = NO_OBJECT;
                drop->status = IS_INVISIBLE;
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

static ITEM_INFO *Carrier_GetCarrier(int16_t item_num)
{
    if (item_num < 0 || item_num >= g_LevelItemCount) {
        return NULL;
    }

    // Allow carried items to be allocated to holder objects (pods/statues),
    // but then have those items dropped by the actual creatures within.
    ITEM_INFO *item = &g_Items[item_num];
    if (Carrier_IsObjectType(item->object_number, m_PlaceholderObjects)) {
        int16_t child_item_num = *(int16_t *)item->data;
        item = &g_Items[child_item_num];
    }

    if (!g_Objects[item->object_number].loaded) {
        return NULL;
    }

    return item;
}

static bool Carrier_IsObjectType(
    GAME_OBJECT_ID object_id, const GAME_OBJECT_ID *test_arr)
{
    for (int i = 0; test_arr[i] != NO_OBJECT; i++) {
        if (test_arr[i] == object_id) {
            return true;
        }
    }
    return false;
}

static GAME_OBJECT_ID Carrier_GetCognate(
    GAME_OBJECT_ID key_id, const GAME_OBJECT_PAIR *test_map)
{
    const GAME_OBJECT_PAIR *pair = &test_map[0];
    while (pair->key_id != NO_OBJECT) {
        if (pair->key_id == key_id) {
            return pair->value_id;
        }
        pair++;
    }

    return NO_OBJECT;
}

int32_t Carrier_GetItemCount(int16_t item_num)
{
    ITEM_INFO *carrier = Carrier_GetCarrier(item_num);
    if (!carrier) {
        return 0;
    }

    CARRIED_ITEM *item = carrier->carried_item;
    int32_t count = 0;
    while (item) {
        if (item->object_id != NO_OBJECT) {
            count++;
        }
        item = item->next_item;
    }

    return count;
}

void Carrier_TestItemDrops(int16_t item_num)
{
    ITEM_INFO *carrier = &g_Items[item_num];
    CARRIED_ITEM *item = carrier->carried_item;
    if (carrier->status != IS_DEACTIVATED || !item
        || (carrier->object_number == O_PIERRE
            && !(carrier->flags & IF_ONESHOT))) {
        return;
    }

    // The enemy is killed (plus is not runaway) and is carrying at
    // least one item. Ensure that each item has not already spawned,
    // convert guns to ammo if applicable, and spawn the items.
    do {
        if (item->status == IS_INVISIBLE) {
            continue;
        }

        GAME_OBJECT_ID object_id = item->object_id;
        if (g_GameFlow.convert_dropped_guns
            && Carrier_IsObjectType(object_id, m_GunObjects)
            && Inv_RequestItem(object_id)) {
            object_id = Carrier_GetCognate(object_id, m_GunAmmoMap);
        }

        item->spawn_number = Item_Spawn(carrier, object_id);
        item->status = IS_ACTIVE;
    } while ((item = item->next_item));
}

void Carrier_TestLegacyDrops(int16_t item_num)
{
    ITEM_INFO *carrier = &g_Items[item_num];
    if (carrier->status != IS_DEACTIVATED) {
        return;
    }

    // Handle cases where legacy saves have been loaded. Ensure that
    // the OG enemy will still spawn items if Lara hasn't yet collected
    // them by using a test cognate in each case. Ensure also that
    // collected items do not re-spawn now or in future saves.
    GAME_OBJECT_ID test_id =
        Carrier_GetCognate(carrier->object_number, m_LegacyMap);
    if (test_id == NO_OBJECT) {
        return;
    }

    if (!Inv_RequestItem(test_id)) {
        Carrier_TestItemDrops(item_num);
    } else {
        CARRIED_ITEM *item = carrier->carried_item;
        while (item) {
            // Simulate Lara having picked up the item.
            item->status = IS_INVISIBLE;
            item = item->next_item;
        }
    }
}
