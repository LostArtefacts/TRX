#include "game/collide.h"

#include "3dsystem/matrix.h"
#include "3dsystem/phd_math.h"
#include "config.h"
#include "game/control.h"
#include "game/draw.h"
#include "game/items.h"
#include "game/lara.h"
#include "game/objects/door.h"
#include "game/room.h"
#include "game/sound.h"
#include "game/sphere.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#define MAX_BADDIE_COLLISION 12

void GetCollisionInfo(
    COLL_INFO *coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t obj_height)
{
    coll->coll_type = COLL_NONE;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
    coll->quadrant = (uint16_t)(coll->facing + PHD_45) / PHD_90;

    int32_t x = xpos;
    int32_t y = ypos - obj_height;
    int32_t z = zpos;
    int32_t ytop = y - 160;

    FLOOR_INFO *floor = GetFloor(x, ytop, z, &room_num);
    int32_t height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    int32_t ceiling = Room_GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->mid_floor = height;
    coll->mid_ceiling = ceiling;
    coll->mid_type = g_HeightType;
    coll->trigger = g_TriggerIndex;

    int16_t tilt = Room_GetTiltType(floor, x, g_LaraItem->pos.y, z);
    coll->tilt_z = tilt >> 8;
    coll->tilt_x = (int8_t)tilt;

    int32_t xleft;
    int32_t zleft;
    int32_t xright;
    int32_t zright;
    int32_t xfront;
    int32_t zfront;
    switch (coll->quadrant) {
    case DIR_NORTH:
        xfront = (phd_sin(coll->facing) * coll->radius) >> W2V_SHIFT;
        zfront = coll->radius;
        xleft = -coll->radius;
        zleft = coll->radius;
        xright = coll->radius;
        zright = coll->radius;
        break;

    case DIR_EAST:
        xfront = coll->radius;
        zfront = (phd_cos(coll->facing) * coll->radius) >> W2V_SHIFT;
        xleft = coll->radius;
        zleft = coll->radius;
        xright = coll->radius;
        zright = -coll->radius;
        break;

    case DIR_SOUTH:
        xfront = (phd_sin(coll->facing) * coll->radius) >> W2V_SHIFT;
        zfront = -coll->radius;
        xleft = coll->radius;
        zleft = -coll->radius;
        xright = -coll->radius;
        zright = -coll->radius;
        break;

    case DIR_WEST:
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

    ceiling = Room_GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->front_floor = height;
    coll->front_ceiling = ceiling;
    coll->front_type = g_HeightType;
    if (coll->slopes_are_walls && coll->front_type == HT_BIG_SLOPE
        && coll->front_floor < 0) {
        coll->front_floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->front_type == HT_BIG_SLOPE
        && coll->front_floor > 0) {
        coll->front_floor = 512;
    } else if (
        coll->lava_is_pit && coll->front_floor > 0 && g_TriggerIndex
        && (g_TriggerIndex[0] & DATA_TYPE) == FT_LAVA) {
        coll->front_floor = 512;
    }

    x = xpos + xleft;
    z = zpos + zleft;
    floor = GetFloor(x, ytop, z, &room_num);
    height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = Room_GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->left_floor = height;
    coll->left_ceiling = ceiling;
    coll->left_type = g_HeightType;
    if (coll->slopes_are_walls && coll->left_type == HT_BIG_SLOPE
        && coll->left_floor < 0) {
        coll->left_floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->left_type == HT_BIG_SLOPE
        && coll->left_floor > 0) {
        coll->left_floor = 512;
    } else if (
        coll->lava_is_pit && coll->left_floor > 0 && g_TriggerIndex
        && (g_TriggerIndex[0] & DATA_TYPE) == FT_LAVA) {
        coll->left_floor = 512;
    }

    x = xpos + xright;
    z = zpos + zright;
    floor = GetFloor(x, ytop, z, &room_num);
    height = GetHeight(floor, x, ytop, z);
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = Room_GetCeiling(floor, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->right_floor = height;
    coll->right_ceiling = ceiling;
    coll->right_type = g_HeightType;
    if (coll->slopes_are_walls && coll->right_type == HT_BIG_SLOPE
        && coll->right_floor < 0) {
        coll->right_floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->right_type == HT_BIG_SLOPE
        && coll->right_floor > 0) {
        coll->right_floor = 512;
    } else if (
        coll->lava_is_pit && coll->right_floor > 0 && g_TriggerIndex
        && (g_TriggerIndex[0] & DATA_TYPE) == FT_LAVA) {
        coll->right_floor = 512;
    }

    if (CollideStaticObjects(coll, xpos, ypos, zpos, room_num, obj_height)) {
        floor = GetFloor(
            xpos + coll->shift.x, ypos, zpos + coll->shift.z, &room_num);
        if (GetHeight(floor, xpos + coll->shift.x, ypos, zpos + coll->shift.z)
                < ypos - 512
            || Room_GetCeiling(
                   floor, xpos + coll->shift.x, ypos, zpos + coll->shift.z)
                > y) {
            coll->shift.x = -coll->shift.x;
            coll->shift.z = -coll->shift.z;
        }
    }

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
        case DIR_NORTH:
        case DIR_SOUTH:
            coll->shift.x = coll->old.x - xpos;
            coll->shift.z = Room_FindGridShift(zpos + zfront, zpos);
            break;

        case DIR_EAST:
        case DIR_WEST:
            coll->shift.x = Room_FindGridShift(xpos + xfront, xpos);
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
        case DIR_NORTH:
        case DIR_SOUTH:
            coll->shift.x = Room_FindGridShift(xpos + xleft, xpos + xfront);
            break;

        case DIR_EAST:
        case DIR_WEST:
            coll->shift.z = Room_FindGridShift(zpos + zleft, zpos + zfront);
            break;
        }

        coll->coll_type = COLL_LEFT;
        return;
    }

    if (coll->right_floor > coll->bad_pos
        || coll->right_floor < coll->bad_neg) {
        switch (coll->quadrant) {
        case DIR_NORTH:
        case DIR_SOUTH:
            coll->shift.x = Room_FindGridShift(xpos + xright, xpos + xfront);
            break;

        case DIR_EAST:
        case DIR_WEST:
            coll->shift.z = Room_FindGridShift(zpos + zright, zpos + zfront);
            break;
        }

        coll->coll_type = COLL_RIGHT;
        return;
    }
}

int32_t CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_number,
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

    Room_GetNearByRooms(x, y, z, coll->radius + 50, hite + 50, room_number);

    for (int i = 0; i < g_RoomsToDrawCount; i++) {
        int16_t room_num = g_RoomsToDraw[i];
        ROOM_INFO *r = &g_RoomInfo[room_num];
        MESH_INFO *mesh = r->mesh;

        for (int j = 0; j < r->num_meshes; j++, mesh++) {
            STATIC_INFO *sinfo = &g_StaticObjects[mesh->static_number];
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
            case PHD_90:
                xmin = mesh->x + sinfo->z_minc;
                xmax = mesh->x + sinfo->z_maxc;
                zmin = mesh->z - sinfo->x_maxc;
                zmax = mesh->z - sinfo->x_minc;
                break;

            case -PHD_180:
                xmin = mesh->x - sinfo->x_maxc;
                xmax = mesh->x - sinfo->x_minc;
                zmin = mesh->z - sinfo->z_maxc;
                zmax = mesh->z - sinfo->z_minc;
                break;

            case -PHD_90:
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

void LaraBaddieCollision(ITEM_INFO *lara_item, COLL_INFO *coll)
{
    lara_item->hit_status = 0;
    g_Lara.hit_direction = -1;
    if (lara_item->hit_points <= 0) {
        return;
    }

    int16_t numroom = 0;
    int16_t roomies[MAX_BADDIE_COLLISION];

    roomies[numroom++] = lara_item->room_number;

    DOOR_INFOS *door = g_RoomInfo[lara_item->room_number].doors;
    if (door) {
        for (int i = 0; i < door->count; i++) {
            if (numroom >= MAX_BADDIE_COLLISION) {
                break;
            }
            roomies[numroom++] = door->door[i].room_num;
        }
    }

    for (int i = 0; i < numroom; i++) {
        int16_t item_num = g_RoomInfo[roomies[i]].item_number;
        while (item_num != NO_ITEM) {
            ITEM_INFO *item = &g_Items[item_num];
            if (item->collidable && item->status != IS_INVISIBLE) {
                OBJECT_INFO *object = &g_Objects[item->object_number];
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

    if (g_Lara.spaz_effect_count) {
        EffectSpaz(lara_item, coll);
    }

    if (g_Lara.hit_direction == -1) {
        g_Lara.hit_frame = 0;
    }

    g_InvChosen = -1;
}

void EffectSpaz(ITEM_INFO *lara_item, COLL_INFO *coll)
{
    int32_t x = g_Lara.spaz_effect->pos.x - lara_item->pos.x;
    int32_t z = g_Lara.spaz_effect->pos.z - lara_item->pos.z;
    PHD_ANGLE hitang = lara_item->pos.y_rot - (PHD_180 + phd_atan(z, x));
    g_Lara.hit_direction = (hitang + PHD_45) / PHD_90;
    if (!g_Lara.hit_frame) {
        Sound_Effect(SFX_LARA_BODYSL, &lara_item->pos, SPM_NORMAL);
    }

    g_Lara.hit_frame++;
    if (g_Lara.hit_frame > 34) {
        g_Lara.hit_frame = 34;
    }

    g_Lara.spaz_effect_count--;
}

void CreatureCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        if (item->hit_points <= 0) {
            Lara_Push(item, coll, 0, 0);
        } else {
            Lara_Push(item, coll, coll->enable_spaz, 0);
        }
    }
}

void ObjectCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }
    if (!TestCollision(item, lara_item)) {
        return;
    }

    if (coll->enable_baddie_push) {
        Lara_Push(item, coll, 0, 1);
    }
}

void TrapCollision(int16_t item_num, ITEM_INFO *lara_item, COLL_INFO *coll)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (item->status == IS_ACTIVE) {
        if (Lara_TestBoundsCollide(item, coll->radius)) {
            TestCollision(item, lara_item);
        }
    } else if (item->status != IS_INVISIBLE) {
        ObjectCollision(item_num, lara_item, coll);
    }
}
