#include "game/lara/misc.h"

#include "config.h"
#include "game/effects.h"
#include "game/lara.h"
#include "game/random.h"
#include "game/rooms.h"
#include "game/sound.h"

void Lara_RefuseInteraction(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!XYZ_32_AreEquivalent(
            &lara_info->interact_target.initial_pos, &lara_item->pos)) {
        lara_info->interact_target.initial_pos = lara_item->pos;
        Sound_Effect(SFX_LARA_NO, &lara_item->pos, SPM_ALWAYS);
    }
}

void Lara_TakeHit(ITEM *const lara_item, const int32_t dx, const int32_t dz)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    const int16_t hit_angle = lara_item->rot.y + DEG_180 - Math_Atan(dz, dx);
    lara_info->hit_direction = Math_GetDirection(hit_angle);
    if (lara_info->hit_frame == 0) {
        Sound_Effect(
            TR_VERSION == 1 ? SFX_LARA_BODYSL : SFX_LARA_INJURY,
            &lara_item->pos, SPM_NORMAL);
    }
    lara_info->hit_frame++;
    lara_info->interact_target.is_moving = false;
    lara_info->interact_target.item_num = NO_ITEM;
    CLAMPG(lara_info->hit_frame, 34);
}

void Lara_TouchLava(void)
{
    ITEM *const lara_item = Lara_GetItem();
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_item->hit_points < 0 || lara_info->water_status == LWS_CHEAT) {
        return;
    }

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(
        lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, lara_item->pos.x, MAX_HEIGHT, lara_item->pos.z);
    if (lara_item->floor != height) {
        return;
    }

    if (g_Config.debug.enable_invulnerability) {
        Lara_CatchFire();
        return;
    }

    lara_item->hit_points = -1;
    lara_item->hit_status = 1;

    if (lara_info->water_status != LWS_ABOVE_WATER) {
        return;
    }

    const OBJECT *const obj = Object_Get(O_FLAME);
    for (int32_t i = 0; i < 10; i++) {
        const int16_t effect_num = Effect_Create(lara_item->room_num);
        if (effect_num != NO_EFFECT) {
            EFFECT *const effect = Effect_Get(effect_num);
            effect->object_id = O_FLAME;
            effect->frame_num = obj->mesh_count * Random_GetControl() / 0x7FFF;
            effect->counter = -1 - 24 * Random_GetControl() / 0x7FFF;
        }
    }
}

int16_t Lara_FloorFront(
    const ITEM *const item, const int16_t ang, const int32_t dist)
{
    const int32_t x = item->pos.x + ((dist * Math_Sin(ang)) >> W2V_SHIFT);
    const int32_t y = item->pos.y - LARA_HEIGHT;
    const int32_t z = item->pos.z + ((dist * Math_Cos(ang)) >> W2V_SHIFT);
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y, z);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }
    return height;
}

void Lara_UpdateRoomToHeight(const int32_t height)
{
    ITEM *const lara_item = Lara_GetItem();
    const int32_t x = lara_item->pos.x;
    const int32_t y = height + lara_item->pos.y;
    const int32_t z = lara_item->pos.z;

    int16_t room_num = lara_item->room_num;
    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    lara_item->floor = Room_GetHeight(sector, x, y, z);

    const int16_t item_num = Item_GetIndex(lara_item);
    Item_UpdateRoom(item_num, room_num);
}

int32_t Lara_GetWaterDepth(
    const int32_t x, const int32_t y, const int32_t z, int16_t room_num)
{
    const ROOM *room = Room_Get(room_num);
    const SECTOR *sector;

    while (true) {
        int32_t z_sector = (z - room->pos.z) >> WALL_SHIFT;
        int32_t x_sector = (x - room->pos.x) >> WALL_SHIFT;

        if (z_sector <= 0) {
            z_sector = 0;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (z_sector >= room->size.z - 1) {
            z_sector = room->size.z - 1;
            if (x_sector < 1) {
                x_sector = 1;
            } else if (x_sector > room->size.x - 2) {
                x_sector = room->size.x - 2;
            }
        } else if (x_sector < 0) {
            x_sector = 0;
        } else if (x_sector >= room->size.x) {
            x_sector = room->size.x - 1;
        }

        sector = Room_GetUnitSector(room, x_sector, z_sector);
        if (sector->portal_room.wall == NO_ROOM) {
            break;
        }
        room_num = sector->portal_room.wall;
        room = Room_Get(room_num);
    }

    if ((room->flags & RF_UNDERWATER) != 0) {
        while (sector->portal_room.sky != NO_ROOM) {
            room = Room_Get(sector->portal_room.sky);
            if ((room->flags & RF_UNDERWATER) == 0) {
                const int32_t water_height = sector->ceiling.height;
                sector = Room_GetSector(x, y, z, &room_num);
                return Room_GetHeight(sector, x, y, z) - water_height;
            }
            sector = Room_GetWorldSector(room, x, z);
        }
        return 0x7FFF;
    }

    while (sector->portal_room.pit != NO_ROOM) {
        room = Room_Get(sector->portal_room.pit);
        if ((room->flags & RF_UNDERWATER) != 0) {
            const int32_t water_height = sector->floor.height;
            sector = Room_GetSector(x, y, z, &room_num);
            return Room_GetHeight(sector, x, y, z) - water_height;
        }
        sector = Room_GetWorldSector(room, x, z);
    }
    return NO_HEIGHT;
}

bool Lara_IsM16Active(void)
{
#if TR_VERSION == 1
    return false;
#else
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->gun_item_num == NO_ITEM || lara->gun_type != LGT_M16) {
        return false;
    }

    const ITEM *const item = Item_Get(lara->gun_item_num);
    return item->current_anim_state == 0 || item->current_anim_state == 2
        || item->current_anim_state == 4;
#endif
}

void Lara_CatchFire(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->burn || lara_info->water_status == LWS_CHEAT) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int16_t effect_num = Effect_Create(lara_item->room_num);
    if (effect_num == NO_EFFECT) {
        return;
    }

    EFFECT *const effect = Effect_Get(effect_num);
    effect->frame_num = 0;
    effect->object_id = O_FLAME;
    effect->counter = -1;
    lara_info->burn = true;
}

void Lara_Extinguish(void)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!lara_info->burn) {
        return;
    }

    lara_info->burn = false;

    // put out flame objects
    int16_t effect_num = Effect_GetActiveNum();
    while (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        const int16_t next_effect_num = effect->next_active;
        if (effect->object_id == O_FLAME && effect->counter < 0) {
            effect->counter = 0;
            Effect_Kill(effect_num);
        }
        effect_num = next_effect_num;
    }
}

bool Lara_HasState(const LARA_TRX_STATE *const test_arr)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (lara_info->extra_anim) {
        return false;
    }

    const ITEM *const lara_item = Lara_GetItem();
    for (int32_t i = 0; test_arr[i] != LS_TRX_INVALID; i++) {
        if (test_arr[i] == LS_U(lara_item->current_anim_state)) {
            return true;
        }
    }
    return false;
}

bool Lara_HasExtraState(const LARA_EXTRA_STATE *const test_arr)
{
    const LARA_INFO *const lara_info = Lara_GetLaraInfo();
    if (!lara_info->extra_anim) {
        return false;
    }

    const ITEM *const lara_item = Lara_GetItem();
    for (int32_t i = 0; test_arr[i] != (LARA_EXTRA_STATE)-1; i++) {
        if (test_arr[i] == (LARA_EXTRA_STATE)lara_item->current_anim_state) {
            return true;
        }
    }
    return false;
}
