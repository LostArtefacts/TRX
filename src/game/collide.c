#include "3dsystem/3d_gen.h"
#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/const.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/effects.h"
#include "game/items.h"
#include "game/misc.h"
#include "game/sphere.h"
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

int32_t __cdecl CollideStaticObjects(
    COLL_INFO* coll, int32_t x, int32_t y, int32_t z, int16_t room_number,
    int32_t hite)
{
    PHD_VECTOR shifter;

    coll->hit_static = 0;
    int32_t inxmin = x - coll->radius;
    int32_t inxmax = x + coll->radius;
    int32_t inymin = y - hite;
    int32_t inymax = y;
    int32_t inzmin = z - coll->radius;
    int32_t inzmax = z + coll->radius;

    shifter.x = 0;
    shifter.y = 0;
    shifter.z = 0;

    GetNearByRooms(x, y, z, coll->radius + 50, hite + 50, room_number);

    for (int i = 0; i < RoomsToDrawNum; i++) {
        ROOM_INFO* r = &RoomInfo[RoomsToDraw[i]];
        MESH_INFO* mesh = r->mesh;

        for (int j = 0; j < r->num_meshes; j++, mesh++) {
            STATIC_INFO* sinfo = &StaticObjects[mesh->static_number];
            if (sinfo->flags & 1) {
                continue;
            }

            int32_t ymin = mesh->y + sinfo->y_minc;
            int32_t ymax = mesh->y + sinfo->y_maxc;
            int32_t xmin;
            int32_t xmax;
            int32_t zmin;
            int32_t zmax;
            switch (mesh->y_rot) {
            case 16384:
                xmin = mesh->x + sinfo->z_minc;
                xmax = mesh->x + sinfo->z_maxc;
                zmin = mesh->z - sinfo->x_maxc;
                zmax = mesh->z - sinfo->x_minc;
                break;

            case -32768:
                xmin = mesh->x - sinfo->x_maxc;
                xmax = mesh->x - sinfo->x_minc;
                zmin = mesh->z - sinfo->z_maxc;
                zmax = mesh->z - sinfo->z_minc;
                break;

            case -16384:
                xmin = mesh->x - sinfo->z_maxc;
                xmax = mesh->x - sinfo->z_minc;
                zmin = mesh->z + sinfo->x_minc;
                zmax = mesh->z + sinfo->x_maxc;
                break;

            default:
                xmin = mesh->x + sinfo->x_minc;
                xmax = mesh->x + sinfo->x_maxc;
                zmin = mesh->z + sinfo->z_minc;
                zmax = mesh->z + sinfo->z_maxc;
                break;
            }

            if (inxmax <= xmin || inxmin >= xmax || inymax <= ymin
                || inymin >= ymax || inzmax <= zmin || inzmin >= zmax) {
                continue;
            }

            int32_t shl = inxmax - xmin;
            int32_t shr = xmax - inxmin;
            if (shl < shr) {
                shifter.x = -shl;
            } else {
                shifter.x = shr;
            }

            shl = inzmax - zmin;
            shr = zmax - inzmin;
            if (shl < shr) {
                shifter.z = -shl;
            } else {
                shifter.z = shr;
            }

            switch (coll->quadrant) {
            case DIR_NORTH:
                if (shifter.x > coll->radius || shifter.x < -coll->radius) {
                    coll->shift.z = shifter.z;
                    coll->shift.x = coll->old.x - x;
                    coll->coll_type = COLL_FRONT;
                } else if (shifter.x > 0) {
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                    coll->coll_type = COLL_LEFT;
                } else if (shifter.x < 0) {
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                    coll->coll_type = COLL_RIGHT;
                }
                break;

            case DIR_EAST:
                if (shifter.z > coll->radius || shifter.z < -coll->radius) {
                    coll->shift.x = shifter.x;
                    coll->shift.z = coll->old.z - z;
                    coll->coll_type = COLL_FRONT;
                } else if (shifter.z > 0) {
                    coll->shift.z = shifter.z;
                    coll->shift.x = 0;
                    coll->coll_type = COLL_RIGHT;
                } else if (shifter.z < 0) {
                    coll->shift.z = shifter.z;
                    coll->shift.x = 0;
                    coll->coll_type = COLL_LEFT;
                }
                break;

            case DIR_SOUTH:
                if (shifter.x > coll->radius || shifter.x < -coll->radius) {
                    coll->shift.z = shifter.z;
                    coll->shift.x = coll->old.x - x;
                    coll->coll_type = COLL_FRONT;
                } else if (shifter.x > 0) {
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                    coll->coll_type = COLL_RIGHT;
                } else if (shifter.x < 0) {
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                    coll->coll_type = COLL_LEFT;
                }
                break;

            case DIR_WEST:
                if (shifter.z > coll->radius || shifter.z < -coll->radius) {
                    coll->shift.x = shifter.x;
                    coll->shift.z = coll->old.z - z;
                    coll->coll_type = COLL_FRONT;
                } else if (shifter.z > 0) {
                    coll->shift.z = shifter.z;
                    coll->shift.x = 0;
                    coll->coll_type = COLL_LEFT;
                } else if (shifter.z < 0) {
                    coll->shift.z = shifter.z;
                    coll->shift.x = 0;
                    coll->coll_type = COLL_RIGHT;
                }
                break;
            }

            coll->hit_static = 1;
            return 1;
        }
    }

    return 0;
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

void ShiftItem(ITEM_INFO* item, COLL_INFO* coll)
{
    item->pos.x += coll->shift.x;
    item->pos.y += coll->shift.y;
    item->pos.z += coll->shift.z;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
}

void UpdateLaraRoom(ITEM_INFO* item, int32_t height)
{
    int32_t x = item->pos.x;
    int32_t y = item->pos.y + height;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_number;
    FLOOR_INFO* floor = GetFloor(x, y, z, &room_num);
    item->floor = GetHeight(floor, x, y, z);
    if (item->room_number != room_num) {
        ItemNewRoom(Lara.item_number, room_num);
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

void LaraBaddieCollision(ITEM_INFO* lara_item, COLL_INFO* coll)
{
    lara_item->hit_status = 0;
    Lara.hit_direction = -1;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t numroom = 0;
    int16_t roomies[12];

    roomies[numroom++] = lara_item->room_number;

    DOOR_INFOS* door = RoomInfo[lara_item->room_number].doors;
    if (door) {
        for (int i = 0; i < door->count; i++) {
            // NOTE: this access violation check was not present in the original
            // code
            if (numroom >= 12) {
                break;
            }
            roomies[numroom++] = door->door[i].room_num;
        }
    }

    for (int i = 0; i < numroom; i++) {
        int16_t item_num = RoomInfo[roomies[i]].item_number;
        while (item_num != NO_ITEM) {
            ITEM_INFO* item = &Items[item_num];
            if (item->collidable && item->status != IS_INVISIBLE) {
                OBJECT_INFO* object = &Objects[item->object_number];
                if (object->collision) {
                    int32_t x = lara_item->pos.x - item->pos.x;
                    int32_t y = lara_item->pos.y - item->pos.y;
                    int32_t z = lara_item->pos.z - item->pos.z;
                    if (x > -TARGET_DIST && x < TARGET_DIST && y > -TARGET_DIST
                        && y < TARGET_DIST && z > -TARGET_DIST
                        && z < TARGET_DIST) {
                        object->collision(item_num, lara_item, coll);
                    }
                }
            }
            item_num = item->next_item;
        }
    }

    if (Lara.spaz_effect_count) {
        EffectSpaz(lara_item, coll);
    }

    if (Lara.hit_direction == -1) {
        Lara.hit_frame = 0;
    }

    InventoryChosen = -1;
}

void EffectSpaz(ITEM_INFO* lara_item, COLL_INFO* coll)
{
    int32_t x = Lara.spaz_effect->pos.x - lara_item->pos.x;
    int32_t z = Lara.spaz_effect->pos.z - lara_item->pos.z;
    PHD_ANGLE hitang = lara_item->pos.y_rot - (0x8000 + phd_atan(z, x));
    Lara.hit_direction = (hitang + 0x2000) / 0x4000;
    if (!Lara.hit_frame) {
        SoundEffect(27, &lara_item->pos, 0);
    }

    Lara.hit_frame++;
    if (Lara.hit_frame > 34) {
        Lara.hit_frame = 34;
    }

    Lara.spaz_effect_count--;
}

void CreatureCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];

    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        if (item->hit_points <= 0) {
            ItemPushLara(item, lara_item, coll, 0, 0);
        } else {
            ItemPushLara(item, lara_item, coll, coll->enable_spaz, 0);
        }
    }
}

void ObjectCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];

    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        ItemPushLara(item, lara_item, coll, 0, 1);
    }
}

void DoorCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];

    if (!TestBoundsCollide(item, lara_item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        if (item->current_anim_state != item->goal_anim_state) {
            ItemPushLara(item, lara_item, coll, coll->enable_spaz, 1);
        } else {
            ItemPushLara(item, lara_item, coll, 0, 1);
        }
    }
}

void TrapCollision(int16_t item_num, ITEM_INFO* lara_item, COLL_INFO* coll)
{
    ITEM_INFO* item = &Items[item_num];

    if (item->status == IS_ACTIVE) {
        if (TestBoundsCollide(item, lara_item, coll->radius)) {
            TestCollision(item, lara_item);
        }
    } else if (item->status != IS_INVISIBLE) {
        ObjectCollision(item_num, lara_item, coll);
    }
}

void ItemPushLara(
    ITEM_INFO* item, ITEM_INFO* lara_item, COLL_INFO* coll, int32_t spazon,
    int32_t bigpush)
{
    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t z = lara_item->pos.z - item->pos.z;
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);
    int32_t rx = (c * x - s * z) >> W2V_SHIFT;
    int32_t rz = (c * z + s * x) >> W2V_SHIFT;

    int16_t* bounds = GetBestFrame(item);
    int32_t minx = bounds[FRAME_BOUND_MIN_X];
    int32_t maxx = bounds[FRAME_BOUND_MAX_X];
    int32_t minz = bounds[FRAME_BOUND_MIN_Z];
    int32_t maxz = bounds[FRAME_BOUND_MAX_Z];

    if (bigpush) {
        minx -= coll->radius;
        maxx += coll->radius;
        minz -= coll->radius;
        maxz += coll->radius;
    }

    if (rx >= minx && rx <= maxx && rz >= minz && rz <= maxz) {
        int32_t l = rx - minx;
        int32_t r = maxx - rx;
        int32_t t = maxz - rz;
        int32_t b = rz - minz;

        if (l <= r && l <= t && l <= b) {
            rx -= l;
        } else if (r <= l && r <= t && r <= b) {
            rx += r;
        } else if (t <= l && t <= r && t <= b) {
            rz += t;
        } else {
            rz -= b;
        }

        int32_t ax = (c * rx + s * rz) >> W2V_SHIFT;
        int32_t az = (c * rz - s * rx) >> W2V_SHIFT;

        lara_item->pos.x = item->pos.x + ax;
        lara_item->pos.z = item->pos.z + az;

        rx = (bounds[FRAME_BOUND_MIN_X] + bounds[FRAME_BOUND_MAX_X]) / 2;
        rz = (bounds[FRAME_BOUND_MIN_Z] + bounds[FRAME_BOUND_MAX_Z]) / 2;
        x -= (c * rx + s * rz) >> W2V_SHIFT;
        z -= (c * rz - s * rx) >> W2V_SHIFT;

        if (spazon) {
            PHD_ANGLE hitang = lara_item->pos.y_rot - (0x8000 + phd_atan(z, x));
            Lara.hit_direction = (hitang + 0x2000) / 0x4000;
            if (!Lara.hit_frame) {
                SoundEffect(27, &lara_item->pos, 0);
            }

            Lara.hit_frame++;
            if (Lara.hit_frame > 34) {
                Lara.hit_frame = 34;
            }
        }

        coll->bad_pos = NO_BAD_POS;
        coll->bad_neg = -STEPUP_HEIGHT;
        coll->bad_ceiling = 0;

        int16_t old_facing = coll->facing;
        coll->facing = phd_atan(
            lara_item->pos.z - coll->old.z, lara_item->pos.x - coll->old.x);
        GetCollisionInfo(
            coll, lara_item->pos.x, lara_item->pos.y, lara_item->pos.z,
            lara_item->room_number, LARA_HITE);
        coll->facing = old_facing;

        if (coll->coll_type != COLL_NONE) {
            lara_item->pos.x = coll->old.x;
            lara_item->pos.z = coll->old.z;
        } else {
            coll->old.x = lara_item->pos.x;
            coll->old.y = lara_item->pos.y;
            coll->old.z = lara_item->pos.z;
            UpdateLaraRoom(lara_item, -10);
        }
    }
}

int32_t TestBoundsCollide(ITEM_INFO* item, ITEM_INFO* lara_item, int32_t radius)
{
    int16_t* bounds = GetBestFrame(item);
    int16_t* larabounds = GetBestFrame(lara_item);
    if (item->pos.y + bounds[FRAME_BOUND_MAX_Y]
            <= lara_item->pos.y + larabounds[FRAME_BOUND_MIN_Y]
        || item->pos.y + bounds[FRAME_BOUND_MIN_Y]
            >= lara_item->pos.y + larabounds[FRAME_BOUND_MAX_Y]) {
        return 0;
    }

    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);
    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t z = lara_item->pos.z - item->pos.z;
    int32_t rx = (c * x - s * z) >> W2V_SHIFT;
    int32_t rz = (c * z + s * x) >> W2V_SHIFT;
    int32_t minx = bounds[FRAME_BOUND_MIN_X] - radius;
    int32_t maxx = bounds[FRAME_BOUND_MAX_X] + radius;
    int32_t minz = bounds[FRAME_BOUND_MIN_Z] - radius;
    int32_t maxz = bounds[FRAME_BOUND_MAX_Z] + radius;
    if (rx >= minx && rx <= maxx && rz >= minz && rz <= maxz) {
        return 1;
    }

    return 0;
}

int32_t TestLaraPosition(int16_t* bounds, ITEM_INFO* item, ITEM_INFO* lara_item)
{
    PHD_ANGLE xrotrel = lara_item->pos.x_rot - item->pos.x_rot;
    PHD_ANGLE yrotrel = lara_item->pos.y_rot - item->pos.y_rot;
    PHD_ANGLE zrotrel = lara_item->pos.z_rot - item->pos.z_rot;
    if (xrotrel < bounds[6] || xrotrel > bounds[7]) {
        return 0;
    }
    if (yrotrel < bounds[8] || yrotrel > bounds[9]) {
        return 0;
    }
    if (zrotrel < bounds[10] || zrotrel > bounds[11]) {
        return 0;
    }

    int32_t x = lara_item->pos.x - item->pos.x;
    int32_t y = lara_item->pos.y - item->pos.y;
    int32_t z = lara_item->pos.z - item->pos.z;
    phd_PushUnitMatrix();
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    PHD_MATRIX* mptr = PhdMatrixPtr;
    int32_t rx = (mptr->_00 * x + mptr->_10 * y + mptr->_20 * z) >> W2V_SHIFT;
    int32_t ry = (mptr->_01 * x + mptr->_11 * y + mptr->_21 * z) >> W2V_SHIFT;
    int32_t rz = (mptr->_02 * x + mptr->_12 * y + mptr->_22 * z) >> W2V_SHIFT;
    phd_PopMatrix();
    if (rx < bounds[0] || rx > bounds[1]) {
        return 0;
    }
    if (ry < bounds[2] || ry > bounds[3]) {
        return 0;
    }
    if (rz < bounds[4] || rz > bounds[5]) {
        return 0;
    }

    return 1;
}

void AlignLaraPosition(PHD_VECTOR* vec, ITEM_INFO* item, ITEM_INFO* lara_item)
{
    lara_item->pos.x_rot = item->pos.x_rot;
    lara_item->pos.y_rot = item->pos.y_rot;
    lara_item->pos.z_rot = item->pos.z_rot;

    phd_PushUnitMatrix();
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    PHD_MATRIX* mptr = PhdMatrixPtr;
    lara_item->pos.x = item->pos.x
        + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
           >> W2V_SHIFT);
    lara_item->pos.y = item->pos.y
        + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
           >> W2V_SHIFT);
    lara_item->pos.z = item->pos.z
        + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
           >> W2V_SHIFT);
    phd_PopMatrix();
}

int32_t MoveLaraPosition(PHD_VECTOR* vec, ITEM_INFO* item, ITEM_INFO* lara_item)
{
    PHD_3DPOS dest;
    dest.x_rot = item->pos.x_rot;
    dest.y_rot = item->pos.y_rot;
    dest.z_rot = item->pos.z_rot;
    phd_PushUnitMatrix();
    phd_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);
    PHD_MATRIX* mptr = PhdMatrixPtr;
    dest.x = item->pos.x
        + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
           >> W2V_SHIFT);
    dest.y = item->pos.y
        + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
           >> W2V_SHIFT);
    dest.z = item->pos.z
        + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
           >> W2V_SHIFT);
    phd_PopMatrix();
    return Move3DPosTo3DPos(&lara_item->pos, &dest, MOVE_SPEED, MOVE_ANG);
}

int32_t Move3DPosTo3DPos(
    PHD_3DPOS* srcpos, PHD_3DPOS* destpos, int32_t velocity, PHD_ANGLE angadd)
{
    PHD_ANGLE angdif;

    int32_t x = destpos->x - srcpos->x;
    int32_t y = destpos->y - srcpos->y;
    int32_t z = destpos->z - srcpos->z;
    int32_t dist = phd_sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
    if (velocity >= dist) {
        srcpos->x = destpos->x;
        srcpos->y = destpos->y;
        srcpos->z = destpos->z;
    } else {
        srcpos->x += (x * velocity) / dist;
        srcpos->y += (y * velocity) / dist;
        srcpos->z += (z * velocity) / dist;
    }

    angdif = destpos->x_rot - srcpos->x_rot;
    if (angdif > angadd) {
        srcpos->x_rot += angadd;
    } else if (angdif < -angadd) {
        srcpos->x_rot -= angadd;
    } else {
        srcpos->x_rot = destpos->x_rot;
    }

    angdif = destpos->y_rot - srcpos->y_rot;
    if (angdif > angadd) {
        srcpos->y_rot += angadd;
    } else if (angdif < -angadd) {
        srcpos->y_rot -= angadd;
    } else {
        srcpos->y_rot = destpos->y_rot;
    }

    angdif = destpos->z_rot - srcpos->z_rot;
    if (angdif > angadd) {
        srcpos->z_rot += angadd;
    } else if (angdif < -angadd) {
        srcpos->z_rot -= angadd;
    } else {
        srcpos->z_rot = destpos->z_rot;
    }

    return srcpos->x == destpos->x && srcpos->y == destpos->y
        && srcpos->z == destpos->z && srcpos->x_rot == destpos->x_rot
        && srcpos->y_rot == destpos->y_rot && srcpos->z_rot == destpos->z_rot;
}

void T1MInjectGameCollide()
{
    INJECT(0x00411780, GetCollisionInfo);
    INJECT(0x00411FA0, CollideStaticObjects);
    INJECT(0x00412390, GetNearByRooms);
    INJECT(0x00412660, ShiftItem);
    INJECT(0x004126A0, UpdateLaraRoom);
    INJECT(0x00412700, LaraBaddieCollision);
    INJECT(0x00412910, CreatureCollision);
    INJECT(0x00412990, ObjectCollision);
    INJECT(0x004129F0, DoorCollision);
    INJECT(0x00412A70, TrapCollision);
    INJECT(0x00412B10, ItemPushLara);
    INJECT(0x00412E50, TestBoundsCollide);
    INJECT(0x00412F30, TestLaraPosition);
    INJECT(0x00413070, AlignLaraPosition);
    INJECT(0x00413230, MoveLaraPosition);
    INJECT(0x00413230, Move3DPosTo3DPos);
}
