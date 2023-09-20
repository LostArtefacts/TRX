#include "game/collide.h"

#include "game/items.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"
#include "util.h"

void Collide_GetCollisionInfo(
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

    FLOOR_INFO *floor = Room_GetFloor(x, ytop, z, &room_num);
    int32_t height = Room_GetHeight(floor, x, ytop, z);
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
        xfront = (Math_Sin(coll->facing) * coll->radius) >> W2V_SHIFT;
        zfront = coll->radius;
        xleft = -coll->radius;
        zleft = coll->radius;
        xright = coll->radius;
        zright = coll->radius;
        break;

    case DIR_EAST:
        xfront = coll->radius;
        zfront = (Math_Cos(coll->facing) * coll->radius) >> W2V_SHIFT;
        xleft = coll->radius;
        zleft = coll->radius;
        xright = coll->radius;
        zright = -coll->radius;
        break;

    case DIR_SOUTH:
        xfront = (Math_Sin(coll->facing) * coll->radius) >> W2V_SHIFT;
        zfront = -coll->radius;
        xleft = coll->radius;
        zleft = -coll->radius;
        xright = -coll->radius;
        zright = -coll->radius;
        break;

    case DIR_WEST:
        xfront = -coll->radius;
        zfront = (Math_Cos(coll->facing) * coll->radius) >> W2V_SHIFT;
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
    floor = Room_GetFloor(x, ytop, z, &room_num);
    height = Room_GetHeight(floor, x, ytop, z);
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
    floor = Room_GetFloor(x, ytop, z, &room_num);
    height = Room_GetHeight(floor, x, ytop, z);
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
    floor = Room_GetFloor(x, ytop, z, &room_num);
    height = Room_GetHeight(floor, x, ytop, z);
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

    if (Collide_CollideStaticObjects(
            coll, xpos, ypos, zpos, room_num, obj_height)) {
        floor = Room_GetFloor(
            xpos + coll->shift.x, ypos, zpos + coll->shift.z, &room_num);
        if (Room_GetHeight(
                floor, xpos + coll->shift.x, ypos, zpos + coll->shift.z)
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

bool Collide_CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_number,
    int32_t height)
{
    PHD_VECTOR shifter;

    coll->hit_static = 0;
    int32_t inxmin = x - coll->radius;
    int32_t inxmax = x + coll->radius;
    int32_t inymin = y - height;
    int32_t inymax = y;
    int32_t inzmin = z - coll->radius;
    int32_t inzmax = z + coll->radius;

    shifter.x = 0;
    shifter.y = 0;
    shifter.z = 0;

    Room_GetNearByRooms(x, y, z, coll->radius + 50, height + 50, room_number);

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
            return true;
        }
    }

    return false;
}

int32_t Collide_GetSpheres(ITEM_INFO *item, SPHERE *ptr, int32_t world_space)
{
    static int16_t null_rotation[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

    if (!item) {
        return 0;
    }

    int32_t x;
    int32_t y;
    int32_t z;
    if (world_space) {
        x = item->pos.x;
        y = item->pos.y;
        z = item->pos.z;
        Matrix_PushUnit();
    } else {
        x = 0;
        y = 0;
        z = 0;
        Matrix_Push();
        Matrix_TranslateAbs(item->pos.x, item->pos.y, item->pos.z);
    }

    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int16_t *frame = Item_GetBestFrame(item);
    Matrix_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
    Matrix_RotYXZpack(*packed_rotation++);

    OBJECT_INFO *object = &g_Objects[item->object_number];
    int16_t **meshpp = &g_Meshes[object->mesh_index];
    int32_t *bone = &g_AnimBones[object->bone_index];

    int16_t *objptr = *meshpp++;
    Matrix_Push();
    Matrix_TranslateRel(objptr[0], objptr[1], objptr[2]);
    ptr->x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    ptr->y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    ptr->z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    ptr->r = objptr[3];
    ptr++;
    Matrix_Pop();

    int16_t *extra_rotation = item->data ? item->data : &null_rotation;
    for (int i = 1; i < object->nmeshes; i++) {
        int32_t bone_extra_flags = bone[0];
        if (bone_extra_flags & BEB_POP) {
            Matrix_Pop();
        }
        if (bone_extra_flags & BEB_PUSH) {
            Matrix_Push();
        }

        Matrix_TranslateRel(bone[1], bone[2], bone[3]);
        Matrix_RotYXZpack(*packed_rotation++);

        if (bone_extra_flags & BEB_ROT_Y) {
            Matrix_RotY(*extra_rotation++);
        }
        if (bone_extra_flags & BEB_ROT_X) {
            Matrix_RotX(*extra_rotation++);
        }
        if (bone_extra_flags & BEB_ROT_Z) {
            Matrix_RotZ(*extra_rotation++);
        }

        objptr = *meshpp++;
        Matrix_Push();
        Matrix_TranslateRel(objptr[0], objptr[1], objptr[2]);
        ptr->x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        ptr->y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        ptr->z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
        ptr->r = objptr[3];
        Matrix_Pop();

        ptr++;
        bone += 4;
    }

    Matrix_Pop();
    return object->nmeshes;
}

int32_t Collide_TestCollision(ITEM_INFO *item, ITEM_INFO *lara_item)
{
    SPHERE slist_baddie[34];
    SPHERE slist_lara[34];

    uint32_t flags = 0;
    int32_t num1 = Collide_GetSpheres(item, slist_baddie, 1);
    int32_t num2 = Collide_GetSpheres(lara_item, slist_lara, 1);

    for (int i = 0; i < num1; i++) {
        SPHERE *ptr1 = &slist_baddie[i];
        if (ptr1->r <= 0) {
            continue;
        }
        for (int j = 0; j < num2; j++) {
            SPHERE *ptr2 = &slist_lara[j];
            if (ptr2->r <= 0) {
                continue;
            }
            int32_t x = ptr2->x - ptr1->x;
            int32_t y = ptr2->y - ptr1->y;
            int32_t z = ptr2->z - ptr1->z;
            int32_t r = ptr2->r + ptr1->r;
            int32_t d = SQUARE(x) + SQUARE(y) + SQUARE(z);
            int32_t r2 = SQUARE(r);
            if (d < r2) {
                flags |= 1 << i;
                break;
            }
        }
    }

    item->touch_bits = flags;
    return flags;
}

void Collide_GetJointAbsPosition(
    ITEM_INFO *item, PHD_VECTOR *vec, int32_t joint)
{
    OBJECT_INFO *object = &g_Objects[item->object_number];

    Matrix_PushUnit();
    Matrix_RotYXZ(item->pos.y_rot, item->pos.x_rot, item->pos.z_rot);

    int16_t *frame = Item_GetBestFrame(item);
    Matrix_TranslateRel(
        frame[FRAME_POS_X], frame[FRAME_POS_Y], frame[FRAME_POS_Z]);

    int32_t *packed_rotation = (int32_t *)(frame + FRAME_ROT);
    Matrix_RotYXZpack(*packed_rotation++);

    int32_t *bone = &g_AnimBones[object->bone_index];

    int16_t *extra_rotation = (int16_t *)item->data;
    for (int i = 0; i < joint; i++) {
        int32_t bone_extra_flags = bone[0];
        if (bone_extra_flags & BEB_POP) {
            Matrix_Pop();
        }
        if (bone_extra_flags & BEB_PUSH) {
            Matrix_Push();
        }

        Matrix_TranslateRel(bone[1], bone[2], bone[3]);
        Matrix_RotYXZpack(*packed_rotation++);

        if (bone_extra_flags & BEB_ROT_Y) {
            Matrix_RotY(*extra_rotation++);
        }
        if (bone_extra_flags & BEB_ROT_X) {
            Matrix_RotX(*extra_rotation++);
        }
        if (bone_extra_flags & BEB_ROT_Z) {
            Matrix_RotZ(*extra_rotation++);
        }

        bone += 4;
    }

    Matrix_TranslateRel(vec->x, vec->y, vec->z);
    vec->x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    vec->y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    vec->z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
    Matrix_Pop();
}
