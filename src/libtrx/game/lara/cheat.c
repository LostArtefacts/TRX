#include "game/lara/cheat.h"

#include "game/console.h"
#include "game/const.h"
#include "game/game.h"
#include "game/game_string.h"
#include "game/inventory.h"
#include "game/lara.h"
#include "game/objects.h"
#include "game/sound.h"

static void M_GiveAllKeysImpl(void);
static void M_GiveAllGunsImpl(void);
static void M_GiveAllMedpacksImpl(void);

static void M_GiveAllKeysImpl(void)
{
    Inv_AddItem(O_PUZZLE_ITEM_1);
    Inv_AddItem(O_PUZZLE_ITEM_2);
    Inv_AddItem(O_PUZZLE_ITEM_3);
    Inv_AddItem(O_PUZZLE_ITEM_4);
    Inv_AddItem(O_KEY_ITEM_1);
    Inv_AddItem(O_KEY_ITEM_2);
    Inv_AddItem(O_KEY_ITEM_3);
    Inv_AddItem(O_KEY_ITEM_4);
    Inv_AddItem(O_PICKUP_ITEM_1);
    Inv_AddItem(O_PICKUP_ITEM_2);
#if TR_VERSION == 1
    Inv_AddItem(O_LEADBAR_ITEM);
#endif
}

static void M_GiveAllGunsImpl(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const bool bonus_flag = Game_IsBonusFlagSet(GBF_NGPLUS);
    Inv_AddItem(O_PISTOL_ITEM);
    Inv_AddItem(O_SHOTGUN_ITEM);
    Inv_AddItem(O_MAGNUM_ITEM);
    Inv_AddItem(O_UZI_ITEM);
    lara_info->shotgun_ammo.ammo = bonus_flag ? 10001 : 300;
    lara_info->magnum_ammo.ammo = bonus_flag ? 10001 : 1000;
    lara_info->uzi_ammo.ammo = bonus_flag ? 10001 : 2000;
#if TR_VERSION >= 2
    Inv_AddItem(O_HARPOON_ITEM);
    Inv_AddItem(O_M16_ITEM);
    Inv_AddItem(O_GRENADE_ITEM);
    lara_info->harpoon_ammo.ammo = bonus_flag ? 10001 : 300;
    lara_info->m16_ammo.ammo = bonus_flag ? 10001 : 300;
    lara_info->grenade_ammo.ammo = bonus_flag ? 10001 : 300;
#endif
}

static void M_GiveAllMedpacksImpl(void)
{
#if TR_VERSION >= 2
    Inv_AddItemNTimes(O_FLARES_ITEM, 10);
#endif
    Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, 10);
    Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, 10);
}

bool Lara_Cheat_GiveAllKeys(void)
{
    if (Lara_GetItem() == nullptr) {
        return false;
    }

    M_GiveAllKeysImpl();

    Sound_Effect(SFX_LARA_KEY, nullptr, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_KEYS));
    return true;
}

bool Lara_Cheat_GiveAllGuns(void)
{
    if (Lara_GetItem() == nullptr) {
        return false;
    }

    M_GiveAllGunsImpl();

    Sound_Effect(SFX_LARA_RELOAD, nullptr, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_GUNS));
    return true;
}

bool Lara_Cheat_GiveAllItems(void)
{
    if (Lara_GetItem() == nullptr) {
        return false;
    }

    M_GiveAllGunsImpl();
    M_GiveAllKeysImpl();
    M_GiveAllMedpacksImpl();

    Sound_Effect(SFX_LARA_HOLSTER, nullptr, SPM_NORMAL);
    Console_Log(GS(OSD_GIVE_ITEM_CHEAT));
    return true;
}

void Lara_Cheat_GetStuff(void)
{
    M_GiveAllGunsImpl();
    M_GiveAllMedpacksImpl();
}

void Lara_Cheat_EndLevel(void)
{
    Game_SetIsLevelComplete(true);
    Console_Log(GS(OSD_COMPLETE_LEVEL));
}

bool Lara_Cheat_KillEnemy(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if ((item->flags & IF_KILLED) != 0) {
        return false;
    }

    Sound_Effect(SFX_EXPLOSION_CHEAT, &item->pos, SPM_NORMAL);
    Creature_Die(item_num, true);
    return true;
}

bool Lara_Cheat_OpenNearestDoor(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    if (lara_item == nullptr) {
        return false;
    }

    int32_t opened = 0;
    int32_t closed = 0;

    const int32_t shift = 8; // constant shift to avoid overflow errors
    const int32_t max_dist = SQUARE((WALL_L * 2) >> shift);
    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (!Object_IsType(item->object_id, g_DoorObjects)
            && !Object_IsType(item->object_id, g_TrapdoorObjects)) {
            continue;
        }

        const int32_t dx = (item->pos.x - lara_item->pos.x) >> shift;
        const int32_t dy = (item->pos.y - lara_item->pos.y) >> shift;
        const int32_t dz = (item->pos.z - lara_item->pos.z) >> shift;
        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > max_dist) {
            continue;
        }

        if (!item->active) {
            Item_AddActive(item_num);
            item->flags |= IF_CODE_BITS;
            opened++;
        } else if ((item->flags & IF_CODE_BITS) != 0) {
            item->flags &= ~IF_CODE_BITS;
            closed++;
        } else {
            item->flags |= IF_CODE_BITS;
            opened++;
        }
        item->timer = 0;
        item->touch_bits = 0;
    }

    if (opened > 0 || closed > 0) {
        Console_Log(opened > 0 ? GS(OSD_DOOR_OPEN) : GS(OSD_DOOR_CLOSE));
        return true;
    }
    Console_Log(GS(OSD_DOOR_OPEN_FAIL));
    return false;
}
