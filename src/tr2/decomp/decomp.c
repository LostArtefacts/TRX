#include "decomp/decomp.h"

#include "game/camera.h"
#include "game/collide.h"
#include "game/game_flow.h"
#include "game/items.h"
#include "game/lara/control.h"
#include "game/lara/draw.h"
#include "game/level.h"
#include "game/music.h"
#include "game/objects/vars.h"
#include "game/output.h"
#include "game/phase.h"
#include "game/requester.h"
#include "game/room.h"
#include "game/viewport.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/utils.h>

// TODO: delegate these constants to individual vehicle code
#define VEHICLE_MIN_BOUNCE 50
#define VEHICLE_MAX_KICK -80

// TODO: delegate this enum to individual vehicle code
typedef enum {
    LA_VEHICLE_HIT_LEFT = 11,
    LA_VEHICLE_HIT_RIGHT = 12,
    LA_VEHICLE_HIT_FRONT = 13,
    LA_VEHICLE_HIT_BACK = 14,
} LARA_ANIM_VEHICLE;

static CAMERA_INFO m_LocalCamera = {};

void CutscenePlayer_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->rot.y = m_LocalCamera.target_angle;
    item->pos.x = m_LocalCamera.pos.pos.x;
    item->pos.y = m_LocalCamera.pos.pos.y;
    item->pos.z = m_LocalCamera.pos.pos.z;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_FindByPos(pos.x, pos.y, pos.z);
    if (room_num != NO_ROOM_NEG && item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    if (item->dynamic_light && item->status != IS_INVISIBLE) {
        pos.x = 0;
        pos.y = 0;
        pos.z = 0;
        Collide_GetJointAbsPosition(item, &pos, 0);
        Output_AddDynamicLight(pos, 12, 11);
    }

    Item_Animate(item);
}

void Lara_Control_Cutscene(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->rot.y = m_LocalCamera.target_angle;
    item->pos.x = m_LocalCamera.pos.pos.x;
    item->pos.y = m_LocalCamera.pos.pos.y;
    item->pos.z = m_LocalCamera.pos.pos.z;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_FindByPos(pos.x, pos.y, pos.z);
    if (room_num != NO_ROOM_NEG && item->room_num != room_num) {
        Item_NewRoom(item_num, room_num);
    }

    Lara_Animate(item);
}

void CutscenePlayer1_Initialise(const int16_t item_num)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->draw_routine = Lara_Draw;
    obj->control = Lara_Control_Cutscene;

    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);
    m_LocalCamera.pos.pos.x = item->pos.x;
    m_LocalCamera.pos.pos.y = item->pos.y;
    m_LocalCamera.pos.pos.z = item->pos.z;
    m_LocalCamera.target_angle = Camera_GetCineData()->position.target_angle;
    m_LocalCamera.pos.room_num = item->room_num;

    item->rot.y = 0;
    item->dynamic_light = 0;
    item->goal_anim_state = 0;
    item->current_anim_state = 0;
    item->frame_num = 0;
    item->anim_num = 0;

    g_Lara.hit_direction = -1;
}

void CutscenePlayerGen_Initialise(const int16_t item_num)
{
    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);
    item->rot.y = 0;
    item->dynamic_light = 0;
}

int32_t Misc_Move3DPosTo3DPos(
    PHD_3DPOS *const src_pos, const PHD_3DPOS *const dst_pos,
    const int32_t velocity, const int16_t ang_add)
{
    // TODO: this function's only usage is in Lara_MovePosition. inline it
    const XYZ_32 dpos = {
        .x = dst_pos->pos.x - src_pos->pos.x,
        .y = dst_pos->pos.y - src_pos->pos.y,
        .z = dst_pos->pos.z - src_pos->pos.z,
    };
    const int32_t dist = XYZ_32_GetDistance0(&dpos);
    if (velocity >= dist) {
        src_pos->pos.x = dst_pos->pos.x;
        src_pos->pos.y = dst_pos->pos.y;
        src_pos->pos.z = dst_pos->pos.z;
    } else {
        src_pos->pos.x += velocity * dpos.x / dist;
        src_pos->pos.y += velocity * dpos.y / dist;
        src_pos->pos.z += velocity * dpos.z / dist;
    }

#define ADJUST_ROT(source, target, rot)                                        \
    do {                                                                       \
        if ((int16_t)(target - source) > rot) {                                \
            source += rot;                                                     \
        } else if ((int16_t)(target - source) < -rot) {                        \
            source -= rot;                                                     \
        } else {                                                               \
            source = target;                                                   \
        }                                                                      \
    } while (0)

    ADJUST_ROT(src_pos->rot.x, dst_pos->rot.x, ang_add);
    ADJUST_ROT(src_pos->rot.y, dst_pos->rot.y, ang_add);
    ADJUST_ROT(src_pos->rot.z, dst_pos->rot.z, ang_add);

    // clang-format off
    return (
        src_pos->pos.x == dst_pos->pos.x &&
        src_pos->pos.y == dst_pos->pos.y &&
        src_pos->pos.z == dst_pos->pos.z &&
        src_pos->rot.x == dst_pos->rot.x &&
        src_pos->rot.y == dst_pos->rot.y &&
        src_pos->rot.z == dst_pos->rot.z
    );
    // clang-format on
}

void IncreaseScreenSize(void)
{
    if (g_Config.rendering.sizer < 1.0) {
        g_Config.rendering.sizer += 0.08;
        CLAMPG(g_Config.rendering.sizer, 1.0);
        Viewport_Reset();
    }
}

void DecreaseScreenSize(void)
{
    if (g_Config.rendering.sizer > 0.44) {
        g_Config.rendering.sizer -= 0.08;
        CLAMPL(g_Config.rendering.sizer, 0.44);
        Viewport_Reset();
    }
}

void GetValidLevelsList(REQUEST_INFO *const req)
{
    Requester_RemoveAllItems(req);
    const GF_LEVEL_TABLE *const level_table = GF_GetLevelTable(GFLT_MAIN);
    for (int32_t i = 0; i < level_table->count; i++) {
        const GF_LEVEL *const level = &level_table->levels[i];
        if (level->type != GFL_GYM) {
            Requester_AddItem(req, level->title, 0, nullptr, 0);
        }
    }
}

void InitialiseGameFlags(void)
{
    Music_ResetTrackFlags();
    for (GAME_OBJECT_ID obj_id = 0; obj_id < O_NUMBER_OF; obj_id++) {
        Object_Get(obj_id)->loaded = 0;
    }

    g_FlipStatus = false;
    for (int32_t i = 0; i < MAX_FLIP_MAPS; i++) {
        g_FlipMaps[i] = 0;
    }

    g_SunsetTimer = 0;
    g_LevelComplete = false;
    g_FlipEffect = -1;
    g_DetonateAllMines = false;
    g_IsMonkAngry = false;
}

void InitialiseLevelFlags(void)
{
    g_SaveGame.current_stats.timer = 0;
    g_SaveGame.current_stats.kills = 0;
    g_SaveGame.current_stats.distance = 0;
    g_SaveGame.current_stats.ammo_hits = 0;
    g_SaveGame.current_stats.ammo_used = 0;
    g_SaveGame.current_stats.medipacks = 0;
    g_SaveGame.current_stats.secret_flags = 0;
}

void GetCarriedItems(void)
{
    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        ITEM *const item = Item_Get(item_num);
        if (!Object_Get(item->object_id)->intelligent) {
            continue;
        }
        item->carried_item = NO_ITEM;

        const ROOM *const room = Room_Get(item->room_num);
        int16_t pickup_item_num = room->item_num;
        do {
            ITEM *const pickup_item = Item_Get(pickup_item_num);

            if (pickup_item->pos.x == item->pos.x
                && pickup_item->pos.y == item->pos.y
                && pickup_item->pos.z == item->pos.z
                && Object_IsType(pickup_item->object_id, g_PickupObjects)) {
                pickup_item->carried_item = item->carried_item;
                item->carried_item = pickup_item_num;
                Item_RemoveDrawn(pickup_item_num);
                pickup_item->room_num = NO_ROOM;
            }

            pickup_item_num = pickup_item->next_item;
        } while (pickup_item_num != NO_ITEM);
    }
}

int32_t DoShift(
    ITEM *const vehicle, const XYZ_32 *const pos, const XYZ_32 *const old)
{
    int32_t x = pos->x >> WALL_SHIFT;
    int32_t z = pos->z >> WALL_SHIFT;
    const int32_t old_x = old->x >> WALL_SHIFT;
    const int32_t old_z = old->z >> WALL_SHIFT;
    const int32_t shift_x = pos->x & (WALL_L - 1);
    const int32_t shift_z = pos->z & (WALL_L - 1);

    if (x == old_x) {
        if (z == old_z) {
            vehicle->pos.x += old->x - pos->x;
            vehicle->pos.z += old->z - pos->z;
        } else if (z > old_z) {
            vehicle->pos.z -= shift_z + 1;
            return pos->x - vehicle->pos.x;
        } else {
            vehicle->pos.z += WALL_L - shift_z;
            return vehicle->pos.x - pos->x;
        }
    } else if (z == old_z) {
        if (x > old_x) {
            vehicle->pos.x -= shift_x + 1;
            return vehicle->pos.z - pos->z;
        } else {
            vehicle->pos.x += WALL_L - shift_x;
            return pos->z - vehicle->pos.z;
        }
    } else {
        int16_t room_num;
        const SECTOR *sector;
        int32_t height;

        x = 0;
        z = 0;

        room_num = vehicle->room_num;
        sector = Room_GetSector(old->x, pos->y, pos->z, &room_num);
        height = Room_GetHeight(sector, old->x, pos->y, pos->z);
        if (height < old->y - STEP_L) {
            if (pos->z > old->z) {
                z = -shift_z - 1;
            } else {
                z = WALL_L - shift_z;
            }
        }

        room_num = vehicle->room_num;
        sector = Room_GetSector(pos->x, pos->y, old->z, &room_num);
        height = Room_GetHeight(sector, pos->x, pos->y, old->z);
        if (height < old->y - STEP_L) {
            if (pos->x > old->x) {
                x = -shift_x - 1;
            } else {
                x = WALL_L - shift_x;
            }
        }

        if (x != 0 && z != 0) {
            vehicle->pos.x += x;
            vehicle->pos.z += z;
        } else if (z != 0) {
            vehicle->pos.z += z;
            if (z > 0) {
                return vehicle->pos.x - pos->x;
            } else {
                return pos->x - vehicle->pos.x;
            }
        } else if (x != 0) {
            vehicle->pos.x += x;
            if (x > 0) {
                return pos->z - vehicle->pos.z;
            } else {
                return vehicle->pos.z - pos->z;
            }
        } else {
            vehicle->pos.x += old->x - pos->x;
            vehicle->pos.z += old->z - pos->z;
        }
    }

    return 0;
}

int32_t DoDynamics(
    const int32_t height, const int32_t fall_speed, int32_t *const out_y)
{
    if (height > *out_y) {
        *out_y += fall_speed;
        if (*out_y > height - VEHICLE_MIN_BOUNCE) {
            *out_y = height;
            return 0;
        }
        return fall_speed + GRAVITY;
    }

    int32_t kick = 4 * (height - *out_y);
    CLAMPL(kick, VEHICLE_MAX_KICK);
    CLAMPG(*out_y, height);
    return fall_speed + ((kick - fall_speed) >> 3);
}

int32_t GetCollisionAnim(const ITEM *const vehicle, XYZ_32 *const moved)
{
    moved->x = vehicle->pos.x - moved->x;
    moved->z = vehicle->pos.z - moved->z;

    if (moved->x != 0 || moved->z != 0) {
        const int32_t c = Math_Cos(vehicle->rot.y);
        const int32_t s = Math_Sin(vehicle->rot.y);
        const int32_t front = (moved->x * s + moved->z * c) >> W2V_SHIFT;
        const int32_t side = (moved->x * c - moved->z * s) >> W2V_SHIFT;
        if (ABS(front) > ABS(side)) {
            if (front > 0) {
                return LA_VEHICLE_HIT_BACK;
            } else {
                return LA_VEHICLE_HIT_FRONT;
            }
        } else {
            if (side > 0) {
                return LA_VEHICLE_HIT_LEFT;
            } else {
                return LA_VEHICLE_HIT_RIGHT;
            }
        }
    }

    return 0;
}

void InitialiseFinalLevel(void)
{
    g_FinalBossActive = 0;
    g_FinalBossCount = 0;
    g_FinalLevelCount = 0;

    for (int32_t item_num = 0; item_num < Item_GetLevelCount(); item_num++) {
        const ITEM *const item = Item_Get(item_num);

        switch (item->object_id) {
        case O_DOG:
        case O_CULT_1:
        case O_WORKER_3:
            g_FinalLevelCount++;
            break;

        case O_CULT_3:
            g_FinalBossItem[g_FinalBossCount] = item_num;
            g_FinalBossCount++;
            if (item->status == IS_ACTIVE) {
                g_FinalBossActive = 1;
            } else if (item->status == IS_DEACTIVATED) {
                g_FinalBossActive = 2;
            }
            break;

        default:
            break;
        }
    }
}
