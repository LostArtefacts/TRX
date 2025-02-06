#include "game/lara/cheat.h"

#include "decomp/flares.h"
#include "game/camera.h"
#include "game/console/common.h"
#include "game/creature.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/gun/gun.h"
#include "game/inventory.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/objects/vars.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/spawn.h"
#include "game/viewport.h"
#include "global/utils.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/utils.h>
#include <libtrx/vector.h>

static void M_GiveAllGunsImpl(void);
static void M_GiveAllMedpacksImpl(void);
static void M_GiveAllKeysImpl(void);
static void M_ReinitialiseGunMeshes(void);
static void M_ResetGunStatus(void);

static void M_ReinitialiseGunMeshes(void)
{
    // TODO: consider refactoring flare check once more is known about overall
    // flare control.
    const bool has_flare = Lara_GetMesh(LM_HAND_L)
        == Object_GetMesh(Object_Get(O_LARA_FLARE)->mesh_idx + LM_HAND_L);

    Lara_InitialiseMeshes(Game_GetCurrentLevel());
    Gun_InitialiseNewWeapon();
    if (has_flare) {
        Flare_DrawMeshes();
    }
}

static void M_GiveAllGunsImpl(void)
{
    Inv_AddItem(O_PISTOL_ITEM);
    Inv_AddItem(O_MAGNUM_ITEM);
    Inv_AddItem(O_UZI_ITEM);
    Inv_AddItem(O_SHOTGUN_ITEM);
    Inv_AddItem(O_HARPOON_ITEM);
    Inv_AddItem(O_M16_ITEM);
    Inv_AddItem(O_GRENADE_ITEM);
    g_Lara.magnum_ammo.ammo = 1000;
    g_Lara.uzi_ammo.ammo = 2000;
    g_Lara.shotgun_ammo.ammo = 300;
    g_Lara.harpoon_ammo.ammo = 300;
    g_Lara.m16_ammo.ammo = 300;
    g_Lara.grenade_ammo.ammo = 300;
}

static void M_GiveAllMedpacksImpl(void)
{
    Inv_AddItemNTimes(O_FLARES_ITEM, 10);
    Inv_AddItemNTimes(O_SMALL_MEDIPACK_ITEM, 10);
    Inv_AddItemNTimes(O_LARGE_MEDIPACK_ITEM, 10);
}

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
}

static void M_ResetGunStatus(void)
{
    const bool has_flare = Lara_GetMesh(LM_HAND_L)
        == Object_GetMesh(Object_Get(O_LARA_FLARE)->mesh_idx + LM_HAND_L);
    if (has_flare) {
        g_Lara.gun_type = LGT_FLARE;
        return;
    }

    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.gun_type = LGT_UNARMED;
    g_Lara.request_gun_type = LGT_UNARMED;
    g_Lara.weapon_item = NO_ITEM;
    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.left_arm.frame_num = 0;
    g_Lara.left_arm.lock = 0;
    g_Lara.right_arm.frame_num = 0;
    g_Lara.right_arm.lock = 0;
    g_Lara.left_arm.anim_num = g_LaraItem->anim_num;
    g_Lara.right_arm.anim_num = g_LaraItem->anim_num;

    const ANIM *const anim = Item_GetAnim(g_LaraItem);
    g_Lara.left_arm.frame_base = anim->frame_ptr;
    g_Lara.right_arm.frame_base = anim->frame_ptr;
}

void Lara_Cheat_EndLevel(void)
{
    g_LevelComplete = true;
    Console_Log(GS(OSD_COMPLETE_LEVEL));
}

bool Lara_Cheat_EnterFlyMode(void)
{
    if (g_LaraItem == nullptr) {
        return false;
    }

    if (g_Lara.extra_anim) {
        M_ResetGunStatus();
    }

    if (g_Lara.gun_status == LGS_HANDS_BUSY
        || (g_Lara.gun_status == LGS_UNDRAW && g_Lara.back_gun != O_LARA)) {
        g_Lara.gun_status = LGS_ARMLESS;
    }

    Lara_GetOffVehicle();

    if (g_Lara.water_status != LWS_UNDERWATER || g_LaraItem->hit_points <= 0) {
        g_LaraItem->pos.y -= STEP_L;
        g_LaraItem->current_anim_state = LS_SWIM;
        g_LaraItem->goal_anim_state = LS_SWIM;
        Item_SwitchToAnim(g_LaraItem, LA_UNDERWATER_SWIM_FORWARD_DRIFT, 0);
        g_LaraItem->gravity = 0;
        g_LaraItem->rot.x = 30 * DEG_1;
        g_LaraItem->fall_speed = 30;
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }

    g_Lara.water_status = LWS_CHEAT;
    g_Lara.hit_effect_count = 0;
    g_Lara.hit_effect = nullptr;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = -1;
    g_Lara.air = LARA_MAX_AIR;
    g_Lara.death_timer = 0;
    g_Lara.mesh_effects = 0;
    g_Lara.burn = 0;
    g_Lara.extra_anim = 0;
    g_LaraItem->hit_points = LARA_MAX_HITPOINTS;

    M_ReinitialiseGunMeshes();
    g_Camera.type = CAM_CHASE;
    Viewport_AlterFOV(-1);

    Console_Log(GS(OSD_FLY_MODE_ON));
    return true;
}

bool Lara_Cheat_ExitFlyMode(void)
{
    if (g_LaraItem == nullptr) {
        return false;
    }

    const ROOM *const room = Room_Get(g_LaraItem->room_num);
    const bool room_submerged = (room->flags & RF_UNDERWATER) != 0;
    const int16_t water_height = Room_GetWaterHeight(
        g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z,
        g_LaraItem->room_num);

    if (room_submerged || (water_height != NO_HEIGHT && water_height > 0)) {
        g_Lara.water_status = LWS_UNDERWATER;
    } else {
        g_Lara.water_status = LWS_ABOVE_WATER;
        Item_SwitchToAnim(g_LaraItem, LA_STAND_STILL, 0);
        g_LaraItem->rot.x = 0;
        g_LaraItem->rot.z = 0;
        g_Lara.head_rot.x = 0;
        g_Lara.head_rot.y = 0;
        g_Lara.torso_rot.x = 0;
        g_Lara.torso_rot.y = 0;
    }

    if (g_Lara.weapon_item != NO_ITEM) {
        g_Lara.gun_status = LGS_UNDRAW;
    } else {
        g_Lara.gun_status = LGS_ARMLESS;
        M_ReinitialiseGunMeshes();
    }

    Console_Log(GS(OSD_FLY_MODE_OFF));
    return true;
}

bool Lara_Cheat_OpenNearestDoor(void)
{
    if (g_LaraItem == nullptr) {
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

        const int32_t dx = (item->pos.x - g_LaraItem->pos.x) >> shift;
        const int32_t dy = (item->pos.y - g_LaraItem->pos.y) >> shift;
        const int32_t dz = (item->pos.z - g_LaraItem->pos.z) >> shift;
        const int32_t dist = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
        if (dist > max_dist) {
            continue;
        }

        if (!item->active) {
            Item_AddActive(item_num);
            item->flags |= IF_CODE_BITS;
            opened++;
        } else if (item->flags & IF_CODE_BITS) {
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

void Lara_Cheat_GetStuff(void)
{
    M_GiveAllGunsImpl();
    M_GiveAllMedpacksImpl();
}

bool Lara_Cheat_GiveAllKeys(void)
{
    if (g_LaraItem == nullptr) {
        return false;
    }

    M_GiveAllKeysImpl();

    Sound_Effect(SFX_LARA_KEY, nullptr, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_KEYS));
    return true;
}

bool Lara_Cheat_GiveAllGuns(void)
{
    if (g_LaraItem == nullptr) {
        return false;
    }

    M_GiveAllGunsImpl();

    Sound_Effect(SFX_LARA_RELOAD, nullptr, SPM_ALWAYS);
    Console_Log(GS(OSD_GIVE_ITEM_ALL_GUNS));
    return true;
}

bool Lara_Cheat_GiveAllItems(void)
{
    if (g_LaraItem == nullptr) {
        return false;
    }

    M_GiveAllGunsImpl();
    M_GiveAllKeysImpl();
    M_GiveAllMedpacksImpl();

    Sound_Effect(SFX_LARA_HOLSTER, &g_LaraItem->pos, SPM_NORMAL);
    Console_Log(GS(OSD_GIVE_ITEM_CHEAT));
    return true;
}

bool Lara_Cheat_Teleport(int32_t x, int32_t y, int32_t z)
{
    int16_t room_num = Room_GetIndexFromPos(x, y, z);
    if (room_num == NO_ROOM) {
        return false;
    }

    const SECTOR *sector = Room_GetSector(x, y, z, &room_num);
    int16_t height = Room_GetHeight(sector, x, y, z);

    if (height == NO_HEIGHT) {
        // Sample a sphere of points around target x, y, z
        // and teleport to the first available location.
        VECTOR *const points = Vector_Create(sizeof(XYZ_32));

        const int32_t radius = 10;
        const int32_t unit = STEP_L;
        for (int32_t dx = -radius; dx <= radius; dx++) {
            for (int32_t dz = -radius; dz <= radius; dz++) {
                if (SQUARE(dx) + SQUARE(dz) > SQUARE(radius)) {
                    continue;
                }

                const XYZ_32 point = {
                    .x = ROUND_TO_SECTOR(x + dx * unit) + WALL_L / 2,
                    .y = y,
                    .z = ROUND_TO_SECTOR(z + dz * unit) + WALL_L / 2,
                };
                room_num = Room_GetIndexFromPos(point.x, point.y, point.z);
                if (room_num == NO_ROOM) {
                    continue;
                }
                sector = Room_GetSector(point.x, point.y, point.z, &room_num);
                height = Room_GetHeight(sector, point.x, point.y, point.z);
                if (height == NO_HEIGHT) {
                    continue;
                }
                Vector_Add(points, (void *)&point);
            }
        }

        int32_t best_distance = INT32_MAX;
        for (int32_t i = 0; i < points->count; i++) {
            const XYZ_32 *const point = (const XYZ_32 *)Vector_Get(points, i);
            const int32_t distance =
                XYZ_32_GetDistance(point, &g_LaraItem->pos);
            if (distance < best_distance) {
                best_distance = distance;
                x = point->x;
                y = point->y;
                z = point->z;
            }
        }

        Vector_Free(points);
        if (best_distance == INT32_MAX) {
            return false;
        }
    }

    room_num = Room_GetIndexFromPos(x, y, z);
    if (room_num == NO_ROOM) {
        return false;
    }
    sector = Room_GetSector(x, y, z, &room_num);
    height = Room_GetHeight(sector, x, y, z);
    if (height == NO_HEIGHT) {
        return false;
    }

    g_LaraItem->pos.x = x;
    g_LaraItem->pos.y = y;
    g_LaraItem->pos.z = z;
    g_LaraItem->floor = height;

    if (g_LaraItem->room_num != room_num) {
        const int16_t item_num = Item_GetIndex(g_LaraItem);
        Item_NewRoom(item_num, room_num);
    }

    if (g_Lara.gun_status == LGS_HANDS_BUSY) {
        g_Lara.gun_status = LGS_ARMLESS;
    }

    Lara_GetOffVehicle();

    if (g_Lara.extra_anim) {
        const ROOM *const room = Room_Get(g_LaraItem->room_num);
        const bool room_submerged = (room->flags & RF_UNDERWATER) != 0;
        const int16_t water_height = Room_GetWaterHeight(
            g_LaraItem->pos.x, g_LaraItem->pos.y, g_LaraItem->pos.z,
            g_LaraItem->room_num);

        if (room_submerged || (water_height != NO_HEIGHT && water_height > 0)) {
            g_Lara.water_status = LWS_UNDERWATER;
            g_LaraItem->current_anim_state = LS_SWIM;
            g_LaraItem->goal_anim_state = LS_SWIM;
            Item_SwitchToAnim(g_LaraItem, LA_UNDERWATER_SWIM_FORWARD_DRIFT, 0);
        } else {
            g_Lara.water_status = LWS_ABOVE_WATER;
            g_LaraItem->current_anim_state = LS_STOP;
            g_LaraItem->goal_anim_state = LS_STOP;
            Item_SwitchToAnim(g_LaraItem, LA_STAND_STILL, 0);
            g_LaraItem->rot.x = 0;
            g_LaraItem->rot.z = 0;
            g_Lara.head_rot.x = 0;
            g_Lara.head_rot.y = 0;
            g_Lara.torso_rot.x = 0;
            g_Lara.torso_rot.y = 0;
        }

        g_Lara.extra_anim = 0;
        M_ResetGunStatus();
        M_ReinitialiseGunMeshes();
    }

    g_Lara.hit_effect_count = 0;
    g_Lara.hit_effect = nullptr;
    g_Lara.hit_frame = 0;
    g_Lara.hit_direction = -1;
    g_Lara.air = LARA_MAX_AIR;
    g_Lara.death_timer = 0;
    g_Lara.mesh_effects = 0;

    g_Camera.type = CAM_CHASE;
    Viewport_AlterFOV(-1);

    Camera_ResetPosition();
    return true;
}

bool Lara_Cheat_KillEnemy(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->hit_points <= 0 && item->object_id != O_WINSTON) {
        return false;
    }

    Sound_Effect(SFX_EXPLOSION_1, &item->pos, SPM_NORMAL);
    Creature_Die(item_num, true);
    return true;
}
