#include "game/collide.h"

#include "game/items.h"
#include "game/output.h"
#include "game/room.h"
#include "global/const.h"
#include "global/types.h"
#include "global/vars.h"

#include <libtrx/config.h>
#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

void Collide_GetCollisionInfo(
    COLL_INFO *coll, int32_t xpos, int32_t ypos, int32_t zpos, int16_t room_num,
    int32_t obj_height)
{
    coll->coll_type = COLL_NONE;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
    coll->quadrant = (uint16_t)(coll->facing + DEG_45) / DEG_90;

    int32_t x = xpos;
    int32_t y = ypos - obj_height;
    int32_t z = zpos;
    int32_t ytop = y - 160;

    const SECTOR *sector = Room_GetSector(x, ytop, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, ytop, z);
    int32_t room_height = height;
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    int32_t ceiling = Room_GetCeiling(sector, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->mid_floor = height;
    coll->mid_ceiling = ceiling;
    coll->mid_type = Room_GetHeightType();

    if (!g_Config.gameplay.fix_bridge_collision
        || !Room_IsOnWalkable(sector, x, ytop, z, room_height)) {
        const int16_t tilt = Room_GetTiltType(sector, x, g_LaraItem->pos.y, z);
        coll->tilt_z = tilt >> 8;
        coll->tilt_x = (int8_t)tilt;
    } else {
        coll->tilt_z = 0;
        coll->tilt_x = 0;
    }

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

    // Front.
    x = xpos + xfront;
    z = zpos + zfront;
    sector = Room_GetSector(x, ytop, z, &room_num);
    height = Room_GetHeight(sector, x, ytop, z);
    room_height = height;
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = Room_GetCeiling(sector, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->front_floor = height;
    coll->front_ceiling = ceiling;
    coll->front_type = Room_GetHeightType();

    if (!g_Config.gameplay.fix_bridge_collision
        || !Room_IsOnWalkable(sector, x, ytop, z, room_height)) {
        if (coll->slopes_are_walls && coll->front_type == HT_BIG_SLOPE
            && coll->front_floor < 0) {
            coll->front_floor = -32767;
        } else if (
            coll->slopes_are_pits && coll->front_type == HT_BIG_SLOPE
            && coll->front_floor > 0) {
            coll->front_floor = 512;
        } else if (
            coll->lava_is_pit && coll->front_floor > 0
            && Room_GetPitSector(sector, x, z)->is_death_sector) {
            coll->front_floor = 512;
        }
    }

    // Left.
    x = xpos + xleft;
    z = zpos + zleft;
    sector = Room_GetSector(x, ytop, z, &room_num);
    height = Room_GetHeight(sector, x, ytop, z);
    room_height = height;
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = Room_GetCeiling(sector, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->left_floor = height;
    coll->left_ceiling = ceiling;
    coll->left_type = Room_GetHeightType();

    if (!g_Config.gameplay.fix_bridge_collision
        || !Room_IsOnWalkable(sector, x, ytop, z, room_height)) {
        if (coll->slopes_are_walls && coll->left_type == HT_BIG_SLOPE
            && coll->left_floor < 0) {
            coll->left_floor = -32767;
        } else if (
            coll->slopes_are_pits && coll->left_type == HT_BIG_SLOPE
            && coll->left_floor > 0) {
            coll->left_floor = 512;
        } else if (
            coll->lava_is_pit && coll->left_floor > 0
            && Room_GetPitSector(sector, x, z)->is_death_sector) {
            coll->left_floor = 512;
        }
    }

    // Right.
    x = xpos + xright;
    z = zpos + zright;
    sector = Room_GetSector(x, ytop, z, &room_num);
    height = Room_GetHeight(sector, x, ytop, z);
    room_height = height;
    if (height != NO_HEIGHT) {
        height -= ypos;
    }

    ceiling = Room_GetCeiling(sector, x, ytop, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->right_floor = height;
    coll->right_ceiling = ceiling;
    coll->right_type = Room_GetHeightType();

    if (!g_Config.gameplay.fix_bridge_collision
        || !Room_IsOnWalkable(sector, x, ytop, z, room_height)) {
        if (coll->slopes_are_walls && coll->right_type == HT_BIG_SLOPE
            && coll->right_floor < 0) {
            coll->right_floor = -32767;
        } else if (
            coll->slopes_are_pits && coll->right_type == HT_BIG_SLOPE
            && coll->right_floor > 0) {
            coll->right_floor = 512;
        } else if (
            coll->lava_is_pit && coll->right_floor > 0
            && Room_GetPitSector(sector, x, z)->is_death_sector) {
            coll->right_floor = 512;
        }
    }

    if (Collide_CollideStaticObjects(
            coll, xpos, ypos, zpos, room_num, obj_height)) {
        sector = Room_GetSector(
            xpos + coll->shift.x, ypos, zpos + coll->shift.z, &room_num);
        if (Room_GetHeight(
                sector, xpos + coll->shift.x, ypos, zpos + coll->shift.z)
                < ypos - 512
            || Room_GetCeiling(
                   sector, xpos + coll->shift.x, ypos, zpos + coll->shift.z)
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

        default:
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

        default:
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

        default:
            break;
        }

        coll->coll_type = COLL_RIGHT;
        return;
    }
}

bool Collide_CollideStaticObjects(
    COLL_INFO *coll, int32_t x, int32_t y, int32_t z, int16_t room_num,
    int32_t height)
{
    XYZ_32 shifter;

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

    Room_GetNearByRooms(x, y, z, coll->radius + 50, height + 50, room_num);

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        const ROOM *const room = Room_Get(Room_DrawGetRoom(i));

        for (int32_t j = 0; j < room->num_static_meshes; j++) {
            const STATIC_MESH *const mesh = &room->static_meshes[j];
            const STATIC_OBJECT_3D *const obj =
                Object_Get3DStatic(mesh->static_num);
            if (!obj->collidable) {
                continue;
            }

            int32_t ymin = mesh->pos.y + obj->collision_bounds.min.y;
            int32_t ymax = mesh->pos.y + obj->collision_bounds.max.y;
            int32_t xmin;
            int32_t xmax;
            int32_t zmin;
            int32_t zmax;
            switch (mesh->rot.y) {
            case DEG_90:
                xmin = mesh->pos.x + obj->collision_bounds.min.z;
                xmax = mesh->pos.x + obj->collision_bounds.max.z;
                zmin = mesh->pos.z - obj->collision_bounds.max.x;
                zmax = mesh->pos.z - obj->collision_bounds.min.x;
                break;

            case -DEG_180:
                xmin = mesh->pos.x - obj->collision_bounds.max.x;
                xmax = mesh->pos.x - obj->collision_bounds.min.x;
                zmin = mesh->pos.z - obj->collision_bounds.max.z;
                zmax = mesh->pos.z - obj->collision_bounds.min.z;
                break;

            case -DEG_90:
                xmin = mesh->pos.x - obj->collision_bounds.max.z;
                xmax = mesh->pos.x - obj->collision_bounds.min.z;
                zmin = mesh->pos.z + obj->collision_bounds.min.x;
                zmax = mesh->pos.z + obj->collision_bounds.max.x;
                break;

            default:
                xmin = mesh->pos.x + obj->collision_bounds.min.x;
                xmax = mesh->pos.x + obj->collision_bounds.max.x;
                zmin = mesh->pos.z + obj->collision_bounds.min.z;
                zmax = mesh->pos.z + obj->collision_bounds.max.z;
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

            default:
                break;
            }

            coll->hit_static = 1;
            return true;
        }
    }

    return false;
}

int32_t Collide_GetSpheres(ITEM *item, SPHERE *ptr, int32_t world_space)
{
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
        Matrix_TranslateAbs32(item->pos);
    }

    Matrix_Rot16(item->rot);

    const ANIM_FRAME *const frame = Item_GetBestFrame(item);
    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(frame->mesh_rots[0]);

    const OBJECT *const obj = Object_Get(item->object_id);
    const OBJECT_MESH *mesh = Object_GetMesh(obj->mesh_idx);

    Matrix_Push();
    Matrix_TranslateRel16(mesh->center);
    ptr->x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    ptr->y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    ptr->z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    ptr->r = mesh->radius;
    ptr++;
    Matrix_Pop();

    const int16_t *extra_rotation = (int16_t *)item->data;
    for (int32_t i = 1; i < obj->mesh_count; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i - 1);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }
        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(frame->mesh_rots[i]);

        if (extra_rotation != nullptr) {
            if (bone->rot_y) {
                Matrix_RotY(*extra_rotation++);
            }
            if (bone->rot_x) {
                Matrix_RotX(*extra_rotation++);
            }
            if (bone->rot_z) {
                Matrix_RotZ(*extra_rotation++);
            }
        }

        mesh = Object_GetMesh(obj->mesh_idx + i);
        Matrix_Push();
        Matrix_TranslateRel16(mesh->center);
        ptr->x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        ptr->y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        ptr->z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
        ptr->r = mesh->radius;
        Matrix_Pop();

        ptr++;
    }

    Matrix_Pop();
    return obj->mesh_count;
}

int32_t Collide_TestCollision(ITEM *item, ITEM *lara_item)
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

void Collide_GetJointAbsPosition(ITEM *item, XYZ_32 *vec, int32_t joint)
{
    const OBJECT *const obj = Object_Get(item->object_id);

    Matrix_PushUnit();
    Matrix_Rot16(item->rot);

    const ANIM_FRAME *const frame = Item_GetBestFrame(item);
    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(frame->mesh_rots[0]);

    int16_t *extra_rotation = (int16_t *)item->data;
    const int32_t abs_joint = MIN(obj->mesh_count, joint);
    for (int32_t i = 0; i < abs_joint; i++) {
        const ANIM_BONE *const bone = Object_GetBone(obj, i);
        if (bone->matrix_pop) {
            Matrix_Pop();
        }
        if (bone->matrix_push) {
            Matrix_Push();
        }

        Matrix_TranslateRel32(bone->pos);
        Matrix_Rot16(frame->mesh_rots[i + 1]);

        if (bone->rot_y) {
            Matrix_RotY(*extra_rotation++);
        }
        if (bone->rot_x) {
            Matrix_RotX(*extra_rotation++);
        }
        if (bone->rot_z) {
            Matrix_RotZ(*extra_rotation++);
        }
    }

    Matrix_TranslateRel32(*vec);
    vec->x = (g_MatrixPtr->_03 >> W2V_SHIFT) + item->pos.x;
    vec->y = (g_MatrixPtr->_13 >> W2V_SHIFT) + item->pos.y;
    vec->z = (g_MatrixPtr->_23 >> W2V_SHIFT) + item->pos.z;
    Matrix_Pop();
}
