#include "game/console/common.h"
#include "game/game.h"
#include "game/game_flow.h"
#include "game/game_string.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/game/camera.h>
#include <libtrx/game/gun.h>
#include <libtrx/game/lara.h>
#include <libtrx/vector.h>

static void M_ReinitialiseGunMeshes(void);
static void M_ResetGunStatus(void);

static void M_ReinitialiseGunMeshes(void)
{
    const bool has_flare = Lara_Flare_IsMeshActive();
    Lara_InitialiseMeshes(Game_GetCurrentLevel());
    Gun_InitialiseNewWeapon();
    if (has_flare) {
        Lara_Flare_DrawMeshes();
    }
}

static void M_ResetGunStatus(void)
{
    const bool has_flare = Lara_Flare_IsMeshActive();
    if (has_flare) {
        g_Lara.gun_type = LGT_FLARE;
        return;
    }

    g_Lara.gun_status = LGS_ARMLESS;
    g_Lara.gun_type = LGT_UNARMED;
    g_Lara.request_gun_type = LGT_UNARMED;
    g_Lara.gun_item_num = NO_ITEM;
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

bool Lara_Cheat_Teleport(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    if (room_num == NO_ROOM) {
        room_num = Room_GetIndexFromPos(x, y, z);
    }
    if (room_num == NO_ROOM) {
        return false;
    }

    const ROOM *const room = Room_Get(room_num);
    if (room->flip_status == RFS_FLIPPED && Room_GetFlipStatus()) {
        room_num = Room_GetFlippedBaseRoom(room_num);
        if (room_num == NO_ROOM) {
            return false;
        }
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
                sector = Room_GetSector(point.x, point.y, point.z, &room_num);
                height =
                    Room_GetHeightEx(sector, point.x, point.y, point.z, true);
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

    sector = Room_GetSector(x, y, z, &room_num);
    height = Room_GetHeightEx(sector, x, y, z, true);
    if (height == NO_HEIGHT) {
        return false;
    }

    g_LaraItem->pos.x = x;
    g_LaraItem->pos.y = y;
    g_LaraItem->pos.z = z;
    g_LaraItem->floor = height;

    const int16_t item_num = Item_GetIndex(g_LaraItem);
    Item_UpdateRoom(item_num, room_num);

    if (g_Lara.gun_status == LGS_HANDS_BUSY) {
        g_Lara.gun_status = LGS_ARMLESS;
    }

    Lara_DismountVehicle();

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

        g_Lara.extra_anim = false;
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
