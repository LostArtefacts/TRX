#include "game/collide.h"

#include "game/items.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

void Collide_GetCollisionInfo(
    COLL_INFO *const coll, const int32_t x_pos, const int32_t y_pos,
    const int32_t z_pos, int16_t room_num, const int32_t obj_height)
{
    coll->coll_type = COLL_NONE;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
    coll->quadrant = Math_GetDirection(coll->facing);

    const SECTOR *sector;
    int32_t height;
    int32_t ceiling;
    int32_t x;
    int32_t z;
    const int32_t y = y_pos - obj_height;
    const int32_t y_top = y - 160;

    x = x_pos;
    z = z_pos;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    if (height != NO_HEIGHT) {
        height -= y_pos;
    }
    ceiling = Room_GetCeiling(sector, x, y_top, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->side_mid.floor = height;
    coll->side_mid.ceiling = ceiling;
    coll->side_mid.type = Room_GetHeightType();

    const int16_t tilt = Room_GetTiltType(sector, x, g_LaraItem->pos.y, z);
    coll->z_tilt = tilt >> 8;
    coll->x_tilt = (int8_t)tilt;

    int32_t x_left;
    int32_t z_left;
    int32_t x_right;
    int32_t z_right;
    int32_t x_front;
    int32_t z_front;
    switch (coll->quadrant) {
    case DIR_NORTH:
        x_front = (coll->radius * Math_Sin(coll->facing)) >> W2V_SHIFT;
        z_front = coll->radius;
        x_left = -coll->radius;
        z_left = coll->radius;
        x_right = coll->radius;
        z_right = coll->radius;
        break;

    case DIR_EAST:
        x_front = coll->radius;
        z_front = (coll->radius * Math_Cos(coll->facing)) >> W2V_SHIFT;
        x_left = coll->radius;
        z_left = coll->radius;
        x_right = coll->radius;
        z_right = -coll->radius;
        break;

    case DIR_SOUTH:
        x_front = (coll->radius * Math_Sin(coll->facing)) >> W2V_SHIFT;
        z_front = -coll->radius;
        x_left = coll->radius;
        z_left = -coll->radius;
        x_right = -coll->radius;
        z_right = -coll->radius;
        break;

    case DIR_WEST:
        x_front = -coll->radius;
        z_front = (coll->radius * Math_Cos(coll->facing)) >> W2V_SHIFT;
        x_left = -coll->radius;
        z_left = -coll->radius;
        x_right = -coll->radius;
        z_right = coll->radius;
        break;

    default:
        x_front = 0;
        z_front = 0;
        x_left = 0;
        z_left = 0;
        x_right = 0;
        z_right = 0;
        break;
    }

    x = x_pos + x_front;
    z = z_pos + z_front;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    if (height != NO_HEIGHT) {
        height -= y_pos;
    }
    ceiling = Room_GetCeiling(sector, x, y_top, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->side_front.floor = height;
    coll->side_front.ceiling = ceiling;
    coll->side_front.type = Room_GetHeightType();
    if (coll->slopes_are_walls && coll->side_front.type == HT_BIG_SLOPE
        && coll->side_front.floor < 0) {
        coll->side_front.floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->side_front.type == HT_BIG_SLOPE
        && coll->side_front.floor > 0) {
        coll->side_front.floor = 512;
    } else if (
        coll->lava_is_pit && coll->side_front.floor > 0
        && Room_GetPitSector(sector, x, z)->is_death_sector) {
        coll->side_front.floor = 512;
    }

    x = x_pos + x_left;
    z = z_pos + z_left;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    if (height != NO_HEIGHT) {
        height -= y_pos;
    }
    ceiling = Room_GetCeiling(sector, x, y_top, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->side_left.floor = height;
    coll->side_left.ceiling = ceiling;
    coll->side_left.type = Room_GetHeightType();
    if (coll->slopes_are_walls && coll->side_left.type == HT_BIG_SLOPE
        && coll->side_left.floor < 0) {
        coll->side_left.floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->side_left.type == HT_BIG_SLOPE
        && coll->side_left.floor > 0) {
        coll->side_left.floor = 512;
    } else if (
        coll->lava_is_pit && coll->side_left.floor > 0
        && Room_GetPitSector(sector, x, z)->is_death_sector) {
        coll->side_left.floor = 512;
    }

    x = x_pos + x_right;
    z = z_pos + z_right;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    if (height != NO_HEIGHT) {
        height -= y_pos;
    }
    ceiling = Room_GetCeiling(sector, x, y_top, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->side_right.floor = height;
    coll->side_right.ceiling = ceiling;
    coll->side_right.type = Room_GetHeightType();
    if (coll->slopes_are_walls && coll->side_right.type == HT_BIG_SLOPE
        && coll->side_right.floor < 0) {
        coll->side_right.floor = -32767;
    } else if (
        coll->slopes_are_pits && coll->side_right.type == HT_BIG_SLOPE
        && coll->side_right.floor > 0) {
        coll->side_right.floor = 512;
    } else if (
        coll->lava_is_pit && coll->side_right.floor > 0
        && Room_GetPitSector(sector, x, z)->is_death_sector) {
        coll->side_right.floor = 512;
    }

    if (Collide_CollideStaticObjects(
            coll, x_pos, y_pos, z_pos, room_num, obj_height)) {
        const XYZ_32 test_pos = {
            .x = x_pos + coll->shift.x,
            .y = y_pos,
            .z = z_pos + coll->shift.z,
        };
        sector = Room_GetSector(test_pos.x, test_pos.y, test_pos.z, &room_num);
        if (Room_GetHeight(sector, test_pos.x, test_pos.y, test_pos.z)
                < test_pos.y - WALL_L / 2
            || Room_GetCeiling(sector, test_pos.x, test_pos.y, test_pos.z)
                > y) {
            coll->shift.x = -coll->shift.x;
            coll->shift.z = -coll->shift.z;
        }
    }

    if (coll->side_mid.floor == NO_HEIGHT) {
        coll->shift.x = coll->old.x - x_pos;
        coll->shift.y = coll->old.y - y_pos;
        coll->shift.z = coll->old.z - z_pos;
        coll->coll_type = COLL_FRONT;
        return;
    }

    if (coll->side_mid.floor - coll->side_mid.ceiling <= 0) {
        coll->shift.x = coll->old.x - x_pos;
        coll->shift.y = coll->old.y - y_pos;
        coll->shift.z = coll->old.z - z_pos;
        coll->coll_type = COLL_CLAMP;
        return;
    }

    if (coll->side_mid.ceiling >= 0) {
        coll->shift.y = coll->side_mid.ceiling;
        coll->coll_type = COLL_TOP;
    }

    if (coll->side_front.floor > coll->bad_pos
        || coll->side_front.floor < coll->bad_neg
        || coll->side_front.ceiling > coll->bad_ceiling) {
        switch (coll->quadrant) {
        case DIR_NORTH:
        case DIR_SOUTH:
            coll->shift.x = coll->old.x - x_pos;
            coll->shift.z = Room_FindGridShift(z_pos + z_front, z_pos);
            break;

        case DIR_EAST:
        case DIR_WEST:
            coll->shift.x = Room_FindGridShift(x_pos + x_front, x_pos);
            coll->shift.z = coll->old.z - z_pos;
            break;
        }

        coll->coll_type = COLL_FRONT;
        return;
    }

    if (coll->side_front.ceiling >= coll->bad_ceiling) {
        coll->shift.x = coll->old.x - x_pos;
        coll->shift.y = coll->old.y - y_pos;
        coll->shift.z = coll->old.z - z_pos;
        coll->coll_type = COLL_TOP_FRONT;
        return;
    }

    if (coll->side_left.floor > coll->bad_pos
        || coll->side_left.floor < coll->bad_neg) {
        switch (coll->quadrant) {
        case DIR_NORTH:
        case DIR_SOUTH:
            coll->shift.x = Room_FindGridShift(x_pos + x_left, x_pos + x_front);
            break;

        case DIR_EAST:
        case DIR_WEST:
            coll->shift.z = Room_FindGridShift(z_pos + z_left, z_pos + z_front);
            break;
        }

        coll->coll_type = COLL_LEFT;
        return;
    }

    if (coll->side_right.floor > coll->bad_pos
        || coll->side_right.floor < coll->bad_neg) {
        switch (coll->quadrant) {
        case DIR_NORTH:
        case DIR_SOUTH:
            coll->shift.x =
                Room_FindGridShift(x_pos + x_right, x_pos + x_front);
            break;

        case DIR_EAST:
        case DIR_WEST:
            coll->shift.z =
                Room_FindGridShift(z_pos + z_right, z_pos + z_front);
            break;
        }

        coll->coll_type = COLL_RIGHT;
        return;
    }
}

int32_t Collide_CollideStaticObjects(
    COLL_INFO *const coll, const int32_t x, const int32_t y, const int32_t z,
    const int16_t room_num, const int32_t height)
{
    coll->hit_static = 0;

    const int32_t in_x_min = x - coll->radius;
    const int32_t in_x_max = x + coll->radius;
    const int32_t in_y_min = y - height;
    const int32_t in_y_max = y;
    const int32_t in_z_min = z - coll->radius;
    const int32_t in_z_max = z + coll->radius;
    XYZ_32 shifter = { .x = 0, .z = 0 };

    Room_GetNearbyRooms(x, y, z, coll->radius + 50, height + 50, room_num);

    for (int32_t i = 0; i < Room_DrawGetCount(); i++) {
        const ROOM *const room = Room_Get(Room_DrawGetRoom(i));

        for (int32_t j = 0; j < room->num_static_meshes; j++) {
            const STATIC_MESH *const mesh = &room->static_meshes[j];
            const STATIC_OBJECT_3D *const obj =
                Object_Get3DStatic(mesh->static_num);

            if (!obj->collidable) {
                continue;
            }

            int32_t x_min;
            int32_t x_max;
            int32_t z_min;
            int32_t z_max;
            const int32_t y_min = mesh->pos.y + obj->collision_bounds.min.y;
            const int32_t y_max = mesh->pos.y + obj->collision_bounds.max.y;
            switch (mesh->rot.y) {
            case DEG_90:
                x_min = mesh->pos.x + obj->collision_bounds.min.z;
                x_max = mesh->pos.x + obj->collision_bounds.max.z;
                z_min = mesh->pos.z - obj->collision_bounds.max.x;
                z_max = mesh->pos.z - obj->collision_bounds.min.x;
                break;

            case -DEG_180:
                x_min = mesh->pos.x - obj->collision_bounds.max.x;
                x_max = mesh->pos.x - obj->collision_bounds.min.x;
                z_min = mesh->pos.z - obj->collision_bounds.max.z;
                z_max = mesh->pos.z - obj->collision_bounds.min.z;
                break;

            case -DEG_90:
                x_min = mesh->pos.x - obj->collision_bounds.max.z;
                x_max = mesh->pos.x - obj->collision_bounds.min.z;
                z_min = mesh->pos.z + obj->collision_bounds.min.x;
                z_max = mesh->pos.z + obj->collision_bounds.max.x;
                break;

            default:
                x_min = mesh->pos.x + obj->collision_bounds.min.x;
                x_max = mesh->pos.x + obj->collision_bounds.max.x;
                z_min = mesh->pos.z + obj->collision_bounds.min.z;
                z_max = mesh->pos.z + obj->collision_bounds.max.z;
                break;
            }

            if (in_x_max <= x_min || in_x_min >= x_max || in_y_max <= y_min
                || in_y_min >= y_max || in_z_max <= z_min
                || in_z_min >= z_max) {
                continue;
            }

            int32_t shl = in_x_max - x_min;
            int32_t shr = x_max - in_x_min;
            if (shl < shr) {
                shifter.x = -shl;
            } else {
                shifter.x = shr;
            }

            shl = in_z_max - z_min;
            shr = z_max - in_z_min;
            if (shl < shr) {
                shifter.z = -shl;
            } else {
                shifter.z = shr;
            }

            switch (coll->quadrant) {
            case DIR_NORTH:
                if (shifter.x > coll->radius || shifter.x < -coll->radius) {
                    coll->coll_type = COLL_FRONT;
                    coll->shift.x = coll->old.x - x;
                    coll->shift.z = shifter.z;
                } else if (shifter.x > 0) {
                    coll->coll_type = COLL_LEFT;
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                } else if (shifter.x < 0) {
                    coll->coll_type = COLL_RIGHT;
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                }
                break;

            case DIR_EAST:
                if (shifter.z > coll->radius || shifter.z < -coll->radius) {
                    coll->coll_type = COLL_FRONT;
                    coll->shift.x = shifter.x;
                    coll->shift.z = coll->old.z - z;
                } else if (shifter.z > 0) {
                    coll->coll_type = COLL_RIGHT;
                    coll->shift.x = 0;
                    coll->shift.z = shifter.z;
                } else if (shifter.z < 0) {
                    coll->coll_type = COLL_LEFT;
                    coll->shift.x = 0;
                    coll->shift.z = shifter.z;
                }
                break;

            case DIR_SOUTH:
                if (shifter.x > coll->radius || shifter.x < -coll->radius) {
                    coll->coll_type = COLL_FRONT;
                    coll->shift.x = coll->old.x - x;
                    coll->shift.z = shifter.z;
                } else if (shifter.x > 0) {
                    coll->coll_type = COLL_RIGHT;
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                } else if (shifter.x < 0) {
                    coll->coll_type = COLL_LEFT;
                    coll->shift.x = shifter.x;
                    coll->shift.z = 0;
                }
                break;

            case DIR_WEST:
                if (shifter.z > coll->radius || shifter.z < -coll->radius) {
                    coll->coll_type = COLL_FRONT;
                    coll->shift.x = shifter.x;
                    coll->shift.z = coll->old.z - z;
                } else if (shifter.z > 0) {
                    coll->coll_type = COLL_LEFT;
                    coll->shift.x = 0;
                    coll->shift.z = shifter.z;
                } else if (shifter.z < 0) {
                    coll->coll_type = COLL_RIGHT;
                    coll->shift.x = 0;
                    coll->shift.z = shifter.z;
                }
                break;
            }

            coll->hit_static = 1;
            return 1;
        }
    }

    return 0;
}

int32_t Collide_TestCollision(ITEM *const item, const ITEM *const lara_item)
{
    SPHERE slist_baddie[34];
    SPHERE slist_lara[34];

    uint32_t touch_bits = 0;
    int32_t num1 = Collide_GetSpheres(item, slist_baddie, true);
    int32_t num2 = Collide_GetSpheres(lara_item, slist_lara, true);

    for (int32_t i = 0; i < num1; i++) {
        const SPHERE *const ptr1 = &slist_baddie[i];
        if (ptr1->r <= 0) {
            continue;
        }

        for (int32_t j = 0; j < num2; j++) {
            const SPHERE *const ptr2 = &slist_lara[j];
            if (ptr2->r <= 0) {
                continue;
            }

            const int32_t dx = ptr2->x - ptr1->x;
            const int32_t dy = ptr2->y - ptr1->y;
            const int32_t dz = ptr2->z - ptr1->z;
            const int32_t d1 = SQUARE(dx) + SQUARE(dy) + SQUARE(dz);
            const int32_t d2 = SQUARE(ptr1->r + ptr2->r);
            if (d1 < d2) {
                touch_bits |= 1 << i;
                break;
            }
        }
    }

    item->touch_bits = touch_bits;
    return touch_bits;
}

int32_t Collide_GetSpheres(
    const ITEM *const item, SPHERE *const spheres, const bool world_space)
{
    if (item == nullptr) {
        return 0;
    }

    XYZ_32 pos;
    if (world_space) {
        pos = item->pos;
        Matrix_PushUnit();
        Matrix_TranslateSet(0, 0, 0);
    } else {
        pos.x = 0;
        pos.y = 0;
        pos.z = 0;
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
    spheres[0].x = pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    spheres[0].y = pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    spheres[0].z = pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    spheres[0].r = mesh->radius;
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
        SPHERE *const sphere = &spheres[i];
        sphere->x = pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        sphere->y = pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        sphere->z = pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
        sphere->r = mesh->radius;
        Matrix_Pop();
    }

    Matrix_Pop();
    return obj->mesh_count;
}

void Collide_GetJointAbsPosition(
    const ITEM *const item, XYZ_32 *const out_vec, const int32_t joint)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM_FRAME *const frame = Item_GetBestFrame(item);

    Matrix_PushUnit();
    Matrix_TranslateSet(0, 0, 0);
    Matrix_Rot16(item->rot);
    Matrix_TranslateRel16(frame->offset);
    Matrix_Rot16(frame->mesh_rots[0]);

    const int16_t *extra_rotation = item->data;
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
    }

    Matrix_TranslateRel32(*out_vec);
    out_vec->x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    out_vec->y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    out_vec->z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
}
