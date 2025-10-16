#include "decomp/decomp.h"

#include "game/cutscene.h"
#include "game/game_flow.h"
#include "game/level.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/camera.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/game.h>
#include <libtrx/game/game_string_table.h>
#include <libtrx/game/lara.h>
#include <libtrx/game/music.h>
#include <libtrx/game/objects/vars.h>
#include <libtrx/game/output.h>
#include <libtrx/game/viewport.h>
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

void Lara_Control_Cutscene(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    item->rot.y = camera->target_angle;
    item->pos.x = camera->pos.pos.x;
    item->pos.y = camera->pos.pos.y;
    item->pos.z = camera->pos.pos.z;

    XYZ_32 pos = {};
    Collide_GetJointAbsPosition(item, &pos, 0);

    const int16_t room_num = Room_GetIndexFromPos(pos.x, pos.y, pos.z);
    if (room_num != NO_ROOM) {
        Item_UpdateRoom(item_num, room_num);
    }

    Lara_Animate(item);
}

void CutscenePlayer1_Initialise(const int16_t item_num)
{
    OBJECT *const obj = Object_Get(O_LARA);
    obj->draw_func = Lara_Draw;
    obj->control_func = Lara_Control_Cutscene;

    Item_AddActive(item_num);
    ITEM *const item = Item_Get(item_num);
    CAMERA_INFO *const camera = Cutscene_GetCamera();
    camera->pos.pos.x = item->pos.x;
    camera->pos.pos.y = item->pos.y;
    camera->pos.pos.z = item->pos.z;
    camera->target_angle = Camera_GetCineData()->position.target_angle;
    camera->pos.room_num = item->room_num;

    item->rot.y = 0;
    item->dynamic_light = false;
    item->goal_anim_state = 0;
    item->current_anim_state = 0;
    item->frame_num = 0;
    item->anim_num = 0;

    LARA_INFO *const lara = Lara_GetLaraInfo();
    lara->hit_direction = -1;
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
