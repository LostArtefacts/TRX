#include "3dsystem/phd_math.h"
#include "game/const.h"
#include "game/control.h"
#include "game/collide.h"
#include "game/vars.h"
#include "util.h"

void GetCollisionInfo(
    COLL_INFO* coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t objheight)
{
    coll->coll_type = COLL_NONE;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
    coll->quadrant = (uint16_t)(coll->facing + 0x2000) / 0x4000;

    int32_t x = xpos;
    int32_t y = ypos - objheight;
    int32_t z = zpos;
    int32_t ytop = y - 160;

    FLOOR_INFO* floor = GetFloor(x, ytop, z, &room_num);
    int32_t height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    int32_t ceiling = GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->mid_floor = height;
    coll->mid_ceiling = ceiling;
    coll->mid_type = HeightType;
    coll->trigger = TriggerIndex;

    int16_t tilt = GetTiltType(floor, x, LaraItem->pos.y, z);
    coll->tilt_z = tilt >> 8;
    coll->tilt_x = (int8_t)tilt;

    int32_t xleft;
    int32_t zleft;
    int32_t xright;
    int32_t zright;
    int32_t xfront;
    int32_t zfront;
    switch (coll->quadrant) {
    case 0:
        xfront = (phd_sin(coll->facing) * coll->radius) >> W2V_SHIFT;
        zfront = coll->radius;
        xleft = -coll->radius;
        zleft = coll->radius;
        xright = coll->radius;
        zright = coll->radius;
        break;

    case 1:
        xfront = coll->radius;
        zfront = (phd_cos(coll->facing) * coll->radius) >> W2V_SHIFT;
        xleft = coll->radius;
        zleft = coll->radius;
        xright = coll->radius;
        zright = -coll->radius;
        break;

    case 2:
        xfront = (phd_sin(coll->facing) * coll->radius) >> W2V_SHIFT;
        zfront = -coll->radius;
        xleft = coll->radius;
        zleft = -coll->radius;
        xright = -coll->radius;
        zright = -coll->radius;
        break;

    case 3:
        xfront = -coll->radius;
        zfront = (phd_cos(coll->facing) * coll->radius) >> W2V_SHIFT;
        xleft = -coll->radius;
        zleft = -coll->radius;
        xright = -coll->radius;
        zright = coll->radius;
        break;

    default:
        xfront = 0;
        zfront = 0;
        xleft = 0;
        zleft = 0;
        xright = 0;
        zright = 0;
        break;
    }

    x = xpos + xfront;
    z = zpos + zfront;
    floor = GetFloor(x, ytop, z, &room_num);
    height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->front_floor = height;
    coll->front_ceiling = ceiling;
    coll->front_type = HeightType;
    if (coll->slopes_are_walls && coll->front_type == HT_BIG_SLOPE
        && coll->front_floor < 0) {
        coll->front_floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->front_type == HT_BIG_SLOPE
        && coll->front_floor > 0) {
        coll->front_floor = 512;
    } else if (
        coll->lava_is_pit && coll->front_floor > 0 && TriggerIndex
        && (TriggerIndex[0] & DATA_TYPE) == FT_LAVA) {
        coll->front_floor = 512;
    }

    x = xpos + xleft;
    z = zpos + zleft;
    floor = GetFloor(x, ytop, z, &room_num);
    height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->left_floor = height;
    coll->left_ceiling = ceiling;
    coll->left_type = HeightType;
    if (coll->slopes_are_walls && coll->left_type == HT_BIG_SLOPE
        && coll->left_floor < 0) {
        coll->left_floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->left_type == HT_BIG_SLOPE
        && coll->left_floor > 0) {
        coll->left_floor = 512;
    } else if (
        coll->lava_is_pit && coll->left_floor > 0 && TriggerIndex
        && (TriggerIndex[0] & DATA_TYPE) == FT_LAVA) {
        coll->left_floor = 512;
    }

    x = xpos + xright;
    z = zpos + zright;
    floor = GetFloor(x, ytop, z, &room_num);
    height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->right_floor = height;
    coll->right_ceiling = ceiling;
    coll->right_type = HeightType;
    if (coll->slopes_are_walls && coll->right_type == HT_BIG_SLOPE
        && coll->right_floor < 0) {
        coll->right_floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->right_type == HT_BIG_SLOPE
        && coll->right_floor > 0) {
        coll->right_floor = 512;
    } else if (
        coll->lava_is_pit && coll->right_floor > 0 && TriggerIndex
        && (TriggerIndex[0] & DATA_TYPE) == FT_LAVA) {
        coll->right_floor = 512;
    }

    CollideStaticObjects(coll, xpos, ypos, zpos, room_num, objheight);

    if (coll->mid_floor == NO_HEIGHT) {
        coll->shift.x = coll->old.x - xpos;
        coll->shift.y = coll->old.y - ypos;
        coll->shift.z = coll->old.z - zpos;
        coll->coll_type = COLL_FRONT;
        return;
    }

    if (coll->mid_floor - coll->mid_ceiling <= 0) {
        coll->shift.x = coll->old.x - xpos;
        coll->shift.y = coll->old.y - ypos;
        coll->shift.z = coll->old.z - zpos;
        coll->coll_type = COLL_CLAMP;
        return;
    }

    if (coll->mid_ceiling >= 0) {
        coll->shift.y = coll->mid_ceiling;
        coll->coll_type = COLL_TOP;
    }

    if (coll->front_floor > coll->bad_pos || coll->front_floor < coll->bad_neg
        || coll->front_ceiling > coll->bad_ceiling) {
        switch (coll->quadrant) {
        case 0:
        case 2:
            coll->shift.x = coll->old.x - xpos;
            coll->shift.z = FindGridShift(zpos + zfront, zpos);
            break;

        case 1:
        case 3:
            coll->shift.x = FindGridShift(xpos + xfront, xpos);
            coll->shift.z = coll->old.z - zpos;
            break;
        }

        coll->coll_type = COLL_FRONT;
        return;
    }

    if (coll->front_ceiling >= coll->bad_ceiling) {
        coll->shift.x = coll->old.x - xpos;
        coll->shift.y = coll->old.y - ypos;
        coll->shift.z = coll->old.z - zpos;
        coll->coll_type = COLL_TOPFRONT;
        return;
    }

    if (coll->left_floor > coll->bad_pos || coll->left_floor < coll->bad_neg) {
        switch (coll->quadrant) {
        case 0:
        case 2:
            coll->shift.x = FindGridShift(xpos + xleft, xpos + xfront);
            break;

        case 1:
        case 3:
            coll->shift.z = FindGridShift(zpos + zleft, zpos + zfront);
            break;
        }

        coll->coll_type = COLL_LEFT;
        return;
    }

    if (coll->right_floor > coll->bad_pos
        || coll->right_floor < coll->bad_neg) {
        switch (coll->quadrant) {
        case 0:
        case 2:
            coll->shift.x = FindGridShift(xpos + xright, xpos + xfront);
            break;

        case 1:
        case 3:
            coll->shift.z = FindGridShift(zpos + zright, zpos + zfront);
            break;
        }

        coll->coll_type = COLL_RIGHT;
        return;
    }
}

int32_t FindGridShift(int32_t src, int32_t dst)
{
    int32_t srcw = src >> WALL_SHIFT;
    int32_t dstw = dst >> WALL_SHIFT;
    if (srcw == dstw) {
        return 0;
    }

    src &= WALL_L - 1;
    if (dstw > srcw) {
        return WALL_L - (src - 1);
    } else {
        return -(src + 1);
    }
}

void GetNearByRooms(
    int32_t x, int32_t y, int32_t z, int32_t r, int32_t h, int16_t room_num)
{
    RoomsToDraw[0] = room_num;
    RoomsToDrawNum = 1;
    GetNewRoom(x + r, y, z + r, room_num);
    GetNewRoom(x - r, y, z + r, room_num);
    GetNewRoom(x + r, y, z - r, room_num);
    GetNewRoom(x - r, y, z - r, room_num);
    GetNewRoom(x + r, y - h, z + r, room_num);
    GetNewRoom(x - r, y - h, z + r, room_num);
    GetNewRoom(x + r, y - h, z - r, room_num);
    GetNewRoom(x - r, y - h, z - r, room_num);
}

void GetNewRoom(int32_t x, int32_t y, int32_t z, int16_t room_num)
{
    GetFloor(x, y, z, &room_num);

    int i;
    for (i = 0; i < RoomsToDrawNum; i++) {
        if (RoomsToDraw[i] == room_num) {
            break;
        }
    }

    // NOTE: this access violation check was not present in the original code
    if (i >= MAX_ROOMS_TO_DRAW) {
        return;
    }

    if (i == RoomsToDrawNum) {
        RoomsToDraw[RoomsToDrawNum++] = room_num;
    }
}

int16_t GetTiltType(FLOOR_INFO* floor, int32_t x, int32_t y, int32_t z)
{
    ROOM_INFO* r;

    while (floor->pit_room != NO_ROOM) {
        r = &RoomInfo[floor->pit_room];
        floor = &r->floor
                     [((z - r->z) >> WALL_SHIFT)
                      + ((x - r->x) >> WALL_SHIFT) * r->x_size];
    }

    if (y + 512 < ((int32_t)floor->floor << 8)) {
        return 0;
    }

    if (floor->index) {
        int16_t* data = &FloorData[floor->index];
        if ((data[0] & DATA_TYPE) == FT_TILT) {
            return data[1];
        }
    }

    return 0;
}

void T1MInjectGameCollide()
{
    INJECT(0x00411780, GetCollisionInfo);
    INJECT(0x00412390, GetNearByRooms);
}
