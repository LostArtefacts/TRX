#include "game/lara/misc.h"

#include "game/effects.h"
#include "game/input.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/random.h"
#include "global/const.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/collision.h>
#include <libtrx/game/lara/const.h>
#include <libtrx/game/math.h>
#include <libtrx/utils.h>

#include <stdint.h>

#define LF_FASTFALL 1
#define LF_STOPHANG 9
#define LF_STARTHANG 12
#define LF_HANG 21

void Lara_HangTest(ITEM *item, COLL_INFO *coll)
{
    int flag = 0;
    const BOUNDS_16 *bounds;

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = NO_BAD_NEG;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);
    if (coll->side_front.floor < 200) {
        flag = 1;
    }

    g_Lara.move_angle = item->rot.y;
    item->gravity = 0;
    item->fall_speed = 0;

    PHD_ANGLE angle = (uint16_t)(item->rot.y + DEG_45) / DEG_90;
    switch (angle) {
    case DIR_NORTH:
        item->pos.z += 2;
        break;

    case DIR_WEST:
        item->pos.x -= 2;
        break;

    case DIR_SOUTH:
        item->pos.z -= 2;
        break;

    case DIR_EAST:
        item->pos.x += 2;
        break;
    }

    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -STEPUP_HEIGHT;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (!g_Input.action || item->hit_points <= 0) {
        item->goal_anim_state = LS_JUMP_UP;
        item->current_anim_state = LS_JUMP_UP;
        Item_SwitchToAnim(item, LA_STOP_HANG, LF_STOPHANG);
        bounds = Item_GetBoundsAccurate(item);
        if (g_Config.gameplay.enable_swing_cancel && item->hit_points > 0) {
            item->pos.y += bounds->max.y;
        } else {
            item->pos.y += coll->side_front.floor - bounds->min.y + 2;
        }
        item->pos.x += coll->shift.x;
        item->pos.z += coll->shift.z;
        item->gravity = 1;
        item->fall_speed = 1;
        item->speed = 2;
        g_Lara.gun_status = LGS_ARMLESS;
        return;
    }

    bounds = Item_GetBoundsAccurate(item);
    const int32_t hdif = coll->side_front.floor - bounds->min.y;

    if (ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF
        || coll->side_mid.ceiling >= 0 || coll->coll_type != COLL_FRONT
        || hdif < -SLOPE_DIF || hdif > SLOPE_DIF || flag) {
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
        if (item->current_anim_state == LS_HANG_LEFT
            || item->current_anim_state == LS_HANG_RIGHT) {
            item->goal_anim_state = LS_HANG;
            item->current_anim_state = LS_HANG;
            Item_SwitchToAnim(item, LA_HANG, LF_HANG);
        }
        return;
    }

    switch (angle) {
    case DIR_NORTH:
    case DIR_SOUTH:
        item->pos.z += coll->shift.z;
        break;

    case DIR_WEST:
    case DIR_EAST:
        item->pos.x += coll->shift.x;
        break;
    }

    if (hdif >= -STEP_L && hdif <= STEP_L) {
        item->pos.y += hdif;
    }
}

void Lara_SlideSlope(ITEM *item, COLL_INFO *coll)
{
    coll->bad_pos = NO_BAD_POS;
    coll->bad_neg = -512;
    coll->bad_ceiling = 0;
    Lara_GetCollisionInfo(item, coll);

    if (Lara_HitCeiling(item, coll)) {
        return;
    }

    Lara_DeflectEdge(item, coll);

    if (coll->side_mid.floor > 200) {
        if (item->current_anim_state == LS_SLIDE) {
            item->current_anim_state = LS_JUMP_FORWARD;
            item->goal_anim_state = LS_JUMP_FORWARD;
            Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
        } else {
            item->current_anim_state = LS_FALL_BACK;
            item->goal_anim_state = LS_FALL_BACK;
            Item_SwitchToAnim(item, LA_FALL_BACK, 0);
        }
        item->gravity = 1;
        item->fall_speed = 0;
        return;
    }

    Lara_TestSlide(item, coll);
    item->pos.y += coll->side_mid.floor;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        item->goal_anim_state = LS_STOP;
    }
}

bool Lara_Fallen(ITEM *item, COLL_INFO *coll)
{
    if (coll->side_mid.floor <= STEPUP_HEIGHT
        || g_Lara.water_status == LWS_WADE) {
        return false;
    }
    item->current_anim_state = LS_JUMP_FORWARD;
    item->goal_anim_state = LS_JUMP_FORWARD;
    Item_SwitchToAnim(item, LA_FALL_DOWN, 0);
    item->gravity = 1;
    item->fall_speed = 0;
    return true;
}

bool Lara_HitCeiling(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_TOP && coll->coll_type != COLL_CLAMP) {
        return false;
    }

    item->pos.x = coll->old.x;
    item->pos.y = coll->old.y;
    item->pos.z = coll->old.z;
    item->goal_anim_state = LS_STOP;
    item->current_anim_state = LS_STOP;
    Item_SwitchToAnim(item, LA_STOP, 0);
    item->gravity = 0;
    item->fall_speed = 0;
    item->speed = 0;
    return true;
}
bool Lara_DeflectEdge(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type == COLL_FRONT || coll->coll_type == COLL_TOP_FRONT) {
        Lara_ShiftCol(coll);
        item->goal_anim_state = LS_STOP;
        item->current_anim_state = LS_STOP;
        item->gravity = 0;
        item->speed = 0;
        return true;
    }

    if (coll->coll_type == COLL_LEFT) {
        Lara_ShiftCol(coll);
        item->rot.y += LARA_DEF_ADD_EDGE;
    } else if (coll->coll_type == COLL_RIGHT) {
        Lara_ShiftCol(coll);
        item->rot.y -= LARA_DEF_ADD_EDGE;
    }
    return false;
}

void Lara_DeflectEdgeJump(ITEM *item, COLL_INFO *coll)
{
    Lara_ShiftCol(coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->rot.y += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_FRONT:
    case COLL_TOP_FRONT:
        item->goal_anim_state = LS_FAST_FALL;
        item->current_anim_state = LS_FAST_FALL;
        Item_SwitchToAnim(item, LA_FAST_FALL, LF_FASTFALL);
        item->speed /= 4;
        g_Lara.move_angle -= DEG_180;
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_TOP:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_CLAMP:
        item->pos.z -= (Math_Cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (Math_Sin(coll->facing) * 100) >> W2V_SHIFT;
        item->speed = 0;
        coll->side_mid.floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
        }
        break;
    }
}

void Lara_SlideEdgeJump(ITEM *item, COLL_INFO *coll)
{
    Lara_ShiftCol(coll);
    switch (coll->coll_type) {
    case COLL_LEFT:
        item->rot.y += LARA_DEF_ADD_EDGE;
        break;

    case COLL_RIGHT:
        item->rot.y -= LARA_DEF_ADD_EDGE;
        break;

    case COLL_TOP:
    case COLL_TOP_FRONT:
        if (item->fall_speed <= 0) {
            item->fall_speed = 1;
        }
        break;

    case COLL_CLAMP:
        item->pos.z -= (Math_Cos(coll->facing) * 100) >> W2V_SHIFT;
        item->pos.x -= (Math_Sin(coll->facing) * 100) >> W2V_SHIFT;
        item->speed = 0;
        coll->side_mid.floor = 0;
        if (item->fall_speed <= 0) {
            item->fall_speed = 16;
        }
        break;
    }
}

bool Lara_TestVault(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= 0 - VAULT_ANGLE && angle <= 0 + VAULT_ANGLE) {
        angle = 0;
    } else if (angle >= DEG_90 - VAULT_ANGLE && angle <= DEG_90 + VAULT_ANGLE) {
        angle = DEG_90;
    } else if (
        angle >= (DEG_180 - 1) - VAULT_ANGLE
        || angle <= -(DEG_180 - 1) + VAULT_ANGLE) {
        angle = -DEG_180;
    } else if (
        angle >= -DEG_90 - VAULT_ANGLE && angle <= -DEG_90 + VAULT_ANGLE) {
        angle = -DEG_90;
    }

    if (angle & (DEG_90 - 1)) {
        return false;
    }

    int32_t hdif = coll->side_front.floor;
    if (hdif >= -STEP_L * 2 - STEP_L / 2 && hdif <= -STEP_L * 2 + STEP_L / 2) {
        if (hdif - coll->side_front.ceiling < 0
            || coll->side_left.floor - coll->side_left.ceiling < 0
            || coll->side_right.floor - coll->side_right.ceiling < 0) {
            return false;
        }
        item->current_anim_state = LS_CLIMB_UP;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_VAULT_12, 0);
        item->pos.y += STEP_L * 2 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->rot.y = angle;
        Lara_ShiftCol(coll);
        return true;
    } else if (
        hdif >= -STEP_L * 3 - STEP_L / 2 && hdif <= -STEP_L * 3 + STEP_L / 2) {
        if (hdif - coll->side_front.ceiling < 0
            || coll->side_left.floor - coll->side_left.ceiling < 0
            || coll->side_right.floor - coll->side_right.ceiling < 0) {
            return false;
        }
        item->current_anim_state = LS_CLIMB_UP;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_VAULT_34, 0);
        item->pos.y += STEP_L * 3 + hdif;
        g_Lara.gun_status = LGS_HANDS_BUSY;
        item->rot.y = angle;
        Lara_ShiftCol(coll);
        return true;
    } else if (
        hdif >= -STEP_L * 7 - STEP_L / 2 && hdif <= -STEP_L * 4 + STEP_L / 2) {
        item->goal_anim_state = LS_JUMP_UP;
        item->current_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_STOP, 0);
        g_Lara.calc_fall_speed =
            -(int16_t)(Math_Sqrt((int)(-2 * GRAVITY * (hdif + 800))) + 3);
        Lara_Animate(item);
        item->rot.y = angle;
        Lara_ShiftCol(coll);
        return true;
    }

    return false;
}

bool Lara_TestHangJump(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF) {
        return false;
    }

    if (coll->side_front.ceiling > 0 || coll->side_mid.ceiling > -384
        || coll->side_mid.floor < 200) {
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    const int32_t hdif = coll->side_front.floor - bounds->min.y;
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return false;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= -HANG_ANGLE && angle <= HANG_ANGLE) {
        angle = 0;
    } else if (angle >= DEG_90 - HANG_ANGLE && angle <= DEG_90 + HANG_ANGLE) {
        angle = DEG_90;
    } else if (
        angle >= (DEG_180 - 1) - HANG_ANGLE
        || angle <= -(DEG_180 - 1) + HANG_ANGLE) {
        angle = -DEG_180;
    } else if (angle >= -DEG_90 - HANG_ANGLE && angle <= -DEG_90 + HANG_ANGLE) {
        angle = -DEG_90;
    }

    if (angle & (DEG_90 - 1)) {
        return false;
    }

    if (Lara_TestHangSwingIn(item, angle)) {
        Item_SwitchToAnim(item, LA_GRAB_LEDGE_IN, 0);
    } else {
        Item_SwitchToAnim(item, LA_GRAB_LEDGE, 0);
    }
    item->current_anim_state = LS_HANG;
    item->goal_anim_state = LS_HANG;

    // bounds = Item_GetBoundsAccurate(item);
    item->pos.y += hdif;
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->rot.y = angle;
    item->gravity = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    return true;
}

bool Lara_TestHangSwingIn(ITEM *item, PHD_ANGLE angle)
{
    int x = item->pos.x;
    int y = item->pos.y;
    int z = item->pos.z;
    int16_t room_num = item->room_num;
    switch (angle) {
    case 0:
        z += 256;
        break;
    case DEG_90:
        x += 256;
        break;
    case -DEG_90:
        x -= 256;
        break;
    case -DEG_180:
        z -= 256;
        break;
    }

    const SECTOR *const sector = Room_GetSector(x, y, z, &room_num);
    const int32_t h = Room_GetHeight(sector, x, y, z);
    const int32_t c = Room_GetCeiling(sector, x, y, z);

    if (h != NO_HEIGHT) {
        if ((h - y) > 0 && (c - y) < -400) {
            return true;
        }
    }
    return false;
}

bool Lara_TestHangJumpUp(ITEM *item, COLL_INFO *coll)
{
    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || g_Lara.gun_status != LGS_ARMLESS
        || ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF) {
        return false;
    }

    if (coll->side_front.ceiling > 0 || coll->side_mid.ceiling > -384) {
        return false;
    }

    const BOUNDS_16 *bounds = Item_GetBoundsAccurate(item);
    const int32_t hdif = coll->side_front.floor - bounds->min.y;
    if (hdif < 0 && hdif + item->fall_speed < 0) {
        return false;
    }
    if (hdif > 0 && hdif + item->fall_speed > 0) {
        return false;
    }

    PHD_ANGLE angle = item->rot.y;
    if (angle >= 0 - HANG_ANGLE && angle <= 0 + HANG_ANGLE) {
        angle = 0;
    } else if (angle >= DEG_90 - HANG_ANGLE && angle <= DEG_90 + HANG_ANGLE) {
        angle = DEG_90;
    } else if (
        angle >= (DEG_180 - 1) - HANG_ANGLE
        || angle <= -(DEG_180 - 1) + HANG_ANGLE) {
        angle = -DEG_180;
    } else if (angle >= -DEG_90 - HANG_ANGLE && angle <= -DEG_90 + HANG_ANGLE) {
        angle = -DEG_90;
    }

    if (angle & (DEG_90 - 1)) {
        return false;
    }

    item->goal_anim_state = LS_HANG;
    item->current_anim_state = LS_HANG;
    Item_SwitchToAnim(item, LA_HANG, LF_STARTHANG);
    bounds = Item_GetBoundsAccurate(item);
    item->pos.y += coll->side_front.floor - bounds->min.y;
    item->pos.x += coll->shift.x;
    item->pos.z += coll->shift.z;
    item->rot.y = angle;
    item->gravity = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    return true;
}

bool Lara_TestSlide(ITEM *item, COLL_INFO *coll)
{
    static PHD_ANGLE old_angle = 1;

    if (ABS(coll->tilt_x) <= 2 && ABS(coll->tilt_z) <= 2) {
        return false;
    }

    PHD_ANGLE ang = 0;
    if (coll->tilt_x > 2) {
        ang = -DEG_90;
    } else if (coll->tilt_x < -2) {
        ang = DEG_90;
    }
    if (coll->tilt_z > 2 && coll->tilt_z > ABS(coll->tilt_x)) {
        ang = -DEG_180;
    } else if (coll->tilt_z < -2 && -coll->tilt_z > ABS(coll->tilt_x)) {
        ang = 0;
    }

    PHD_ANGLE adif = ang - item->rot.y;
    Lara_ShiftCol(coll);
    if (adif >= -DEG_90 && adif <= DEG_90) {
        if (item->current_anim_state != LS_SLIDE || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE;
            item->current_anim_state = LS_SLIDE;
            Item_SwitchToAnim(item, LA_SLIDE, 0);
            item->rot.y = ang;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    } else {
        if (item->current_anim_state != LS_SLIDE_BACK || old_angle != ang) {
            item->goal_anim_state = LS_SLIDE_BACK;
            item->current_anim_state = LS_SLIDE_BACK;
            Item_SwitchToAnim(item, LA_SLIDE_BACK, 0);
            item->rot.y = ang - DEG_180;
            g_Lara.move_angle = ang;
            old_angle = ang;
        }
    }
    return true;
}

bool Lara_LandedBad(ITEM *item, COLL_INFO *coll)
{
    int16_t room_num = item->room_num;

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);

    const int32_t old_y = item->pos.y;
    const int32_t height = Room_GetHeight(
        sector, item->pos.x, item->pos.y - LARA_HEIGHT, item->pos.z);

    item->floor = height;
    item->pos.y = height;
    Room_TestTriggers(item);
    item->pos.y = old_y;

    int landspeed = item->fall_speed - DAMAGE_START;
    if (landspeed <= 0) {
        return false;
    } else if (landspeed > DAMAGE_LENGTH) {
        item->hit_points = -1;
    } else {
        Lara_TakeDamage(
            (LARA_MAX_HITPOINTS * landspeed * landspeed)
                / (DAMAGE_LENGTH * DAMAGE_LENGTH),
            false);
    }

    // #675: Original bug to keep. Correct operator would be <=
    if (item->hit_points < 0) {
        return true;
    }
    return false;
}

void Lara_SurfaceCollision(ITEM *item, COLL_INFO *coll)
{
    coll->facing = g_Lara.move_angle;

    int32_t obj_height = SURF_HEIGHT;
    if (g_Config.gameplay.enable_wading) {
        obj_height += 100;
    }
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + SURF_HEIGHT, item->pos.z,
        item->room_num, obj_height);

    Lara_ShiftCol(coll);

    if (coll->coll_type == COLL_LEFT) {
        item->rot.y += 5 * DEG_1;
    } else if (coll->coll_type == COLL_RIGHT) {
        item->rot.y -= 5 * DEG_1;
    } else if (
        coll->coll_type != COLL_NONE
        || (coll->side_mid.floor < 0 && coll->side_mid.type == HT_BIG_SLOPE)) {
        item->fall_speed = 0;
        item->pos.x = coll->old.x;
        item->pos.y = coll->old.y;
        item->pos.z = coll->old.z;
    }

    int16_t wh = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_num);
    if (wh - item->pos.y <= -100) {
        item->goal_anim_state = LS_SWIM;
        item->current_anim_state = LS_DIVE;
        Item_SwitchToAnim(item, LA_SURF_DIVE, 0);
        item->rot.x = -45 * DEG_1;
        item->fall_speed = 80;
        g_Lara.water_status = LWS_UNDERWATER;
        return;
    }

    if (g_Config.gameplay.enable_wading) {
        Lara_TestWaterStepOut(item, coll);
    } else {
        Lara_TestWaterClimbOut(item, coll);
    }
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

    if (room->flags & RF_UNDERWATER) {
        while (sector->portal_room.sky != NO_ROOM) {
            room = Room_Get(sector->portal_room.sky);
            if (!(room->flags & RF_UNDERWATER)) {
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
        if (room->flags & RF_UNDERWATER) {
            const int32_t water_height = sector->floor.height;
            sector = Room_GetSector(x, y, z, &room_num);
            return Room_GetHeight(sector, x, y, z) - water_height;
        }
        sector = Room_GetWorldSector(room, x, z);
    }
    return NO_HEIGHT;
}

void Lara_TestWaterDepth(ITEM *const item, const COLL_INFO *const coll)
{
    int16_t room_num = item->room_num;

    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t water_depth =
        Lara_GetWaterDepth(item->pos.x, item->pos.y, item->pos.z, room_num);

    if (water_depth == NO_HEIGHT || water_depth > STEP_L * 2) {
        return;
    }

    Item_SwitchToAnim(item, LA_UNDERWATER_TO_STAND, 0);
    item->current_anim_state = LS_WATER_OUT;
    item->goal_anim_state = LS_STOP;
    item->rot.x = 0;
    item->rot.z = 0;
    item->gravity = 0;
    item->speed = 0;
    item->fall_speed = 0;
    g_Lara.water_status = LWS_WADE;
    item->pos.y = Room_GetHeight(sector, item->pos.x, item->pos.y, item->pos.z);
}

bool Lara_TestWaterStepOut(ITEM *const item, const COLL_INFO *const coll)
{
    if (coll->coll_type == COLL_FRONT || coll->side_mid.type == HT_BIG_SLOPE
        || coll->side_mid.floor >= 0) {
        return false;
    }

    if (coll->side_mid.floor < -STEP_L / 2) {
        item->current_anim_state = LS_WATER_OUT;
        item->goal_anim_state = LS_STOP;
        Item_SwitchToAnim(item, LA_SURF_TO_WADE, 0);
    } else if (item->goal_anim_state == LS_SURF_LEFT) {
        item->goal_anim_state = LS_STEP_LEFT;
    } else if (item->goal_anim_state == LS_SURF_RIGHT) {
        item->goal_anim_state = LS_STEP_RIGHT;
    } else {
        item->current_anim_state = LS_WADE;
        item->goal_anim_state = LS_WADE;
        Item_SwitchToAnim(item, LA_WADE, 0);
    }

    item->pos.y += coll->side_front.floor + SURF_HEIGHT - 5;
    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);
    item->gravity = 0;
    item->rot.x = 0;
    item->rot.z = 0;
    item->speed = 0;
    item->fall_speed = 0;
    g_Lara.water_status = LWS_WADE;
    return true;
}

bool Lara_TestWaterClimbOut(ITEM *item, COLL_INFO *coll)
{
    if (item->rot.y != g_Lara.move_angle) {
        return false;
    }

    if (coll->coll_type != COLL_FRONT || !g_Input.action
        || ABS(coll->side_left.floor - coll->side_right.floor) >= SLOPE_DIF) {
        return false;
    }

    if (coll->side_front.ceiling > 0
        || coll->side_mid.ceiling > -STEPUP_HEIGHT) {
        return false;
    }

    const int32_t hdif = coll->side_front.floor + SURF_HEIGHT;
    if (hdif <= -STEP_L * 2 || hdif > SURF_HEIGHT - STEPUP_HEIGHT) {
        return false;
    }

    const DIRECTION dir = Math_GetDirectionCone(item->rot.y, HANG_ANGLE);
    if (dir == DIR_UNKNOWN) {
        return false;
    }

    item->pos.y += hdif - 5;
    Lara_UpdateRoomToHeight(-LARA_HEIGHT / 2);

    switch (dir) {
    case DIR_NORTH:
        item->pos.z = (item->pos.z & -WALL_L) + WALL_L + LARA_RAD;
        break;
    case DIR_WEST:
        item->pos.x = (item->pos.x & -WALL_L) + WALL_L + LARA_RAD;
        break;
    case DIR_SOUTH:
        item->pos.z = (item->pos.z & -WALL_L) - LARA_RAD;
        break;
    case DIR_EAST:
        item->pos.x = (item->pos.x & -WALL_L) - LARA_RAD;
        break;
    case DIR_UNKNOWN:
        return false;
    }

    LARA_ANIMATION animation;
    if (hdif < -STEP_L / 2) {
        animation = LA_SURF_CLIMB_HIGH;
    } else if (hdif < STEP_L / 2) {
        animation = LA_SURF_CLIMB_MEDIUM;
    } else {
        animation = LA_SURF_TO_WADE_LOW;
    }

    Item_SwitchToAnim(item, animation, 0);
    item->current_anim_state = LS_WATER_OUT;
    item->goal_anim_state = LS_STOP;
    item->rot.x = 0;
    item->rot.y = Math_DirectionToAngle(dir);
    item->rot.z = 0;
    item->gravity = 0;
    item->fall_speed = 0;
    item->speed = 0;
    g_Lara.gun_status = LGS_HANDS_BUSY;
    g_Lara.water_status = LWS_ABOVE_WATER;
    return true;
}

void Lara_SwimCollision(ITEM *item, COLL_INFO *coll)
{
    if (item->rot.x < -DEG_90 || item->rot.x > DEG_90) {
        g_Lara.move_angle = item->rot.y + DEG_180;
    } else {
        g_Lara.move_angle = item->rot.y;
    }

    coll->facing = g_Lara.move_angle;

    int32_t height;
    if (g_Config.gameplay.enable_wading) {
        height = (LARA_HEIGHT * Math_Sin(item->rot.x)) >> W2V_SHIFT;
        if (height < 0) {
            height = -height;
        }
        CLAMPL(height, 200);
        coll->bad_neg = -height;
    } else {
        height = UW_HEIGHT;
    }

    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y + height / 2, item->pos.z,
        item->room_num, height);
    Lara_ShiftCol(coll);

    switch (coll->coll_type) {
    case COLL_FRONT:
        if (item->rot.x > 35 * DEG_1) {
            item->rot.x = item->rot.x + UW_WALLDEFLECT;
            break;
        }
        if (item->rot.x < -35 * DEG_1) {
            item->rot.x = item->rot.x - UW_WALLDEFLECT;
            break;
        }
        item->fall_speed = 0;
        break;

    case COLL_TOP:
        if (item->rot.x >= -45 * DEG_1) {
            item->rot.x -= UW_WALLDEFLECT;
        }
        break;

    case COLL_TOP_FRONT:
        item->fall_speed = 0;
        break;

    case COLL_LEFT:
        item->rot.y += 5 * DEG_1;
        break;

    case COLL_RIGHT:
        item->rot.y -= 5 * DEG_1;
        break;

    case COLL_CLAMP:
        item->pos = coll->old;
        item->fall_speed = 0;
        return;
    }

    if (coll->side_mid.floor < 0) {
        item->pos.y += coll->side_mid.floor;
        item->rot.x += UW_WALLDEFLECT;
    }

    if (g_Config.gameplay.enable_wading && g_Lara.water_status != LWS_CHEAT) {
        Lara_TestWaterDepth(item, coll);
    }
}

void Lara_CatchFire(void)
{
    const int16_t effect_num = Effect_Create(g_LaraItem->room_num);
    if (effect_num != NO_EFFECT) {
        EFFECT *const effect = Effect_Get(effect_num);
        effect->frame_num = 0;
        effect->object_id = O_FLAME;
        effect->counter = -1;
    }
}

void Lara_Extinguish(void)
{
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
