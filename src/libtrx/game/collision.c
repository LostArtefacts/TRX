#include "game/collision.h"

#include "config.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/matrix.h"
#include "game/rooms.h"

static bool M_IsOnWalkable(
    const SECTOR *sector, int32_t x, int32_t y, int32_t z, int32_t room_height);

static bool M_IsOnWalkable(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z, const int32_t room_height)
{
#if TR_VERSION == 1
    return g_Config.gameplay.fix_bridge_collision
        && Room_IsOnWalkable(sector, x, y, z, room_height);
#elif TR_VERSION >= 2
    return false;
#endif
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
    spheres[0].pos.x = pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    spheres[0].pos.y = pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    spheres[0].pos.z = pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
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
        sphere->pos.x = pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        sphere->pos.y = pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        sphere->pos.z = pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
        sphere->r = mesh->radius;
        Matrix_Pop();
    }

    Matrix_Pop();
    return obj->mesh_count;
}

void Collide_GetCollisionInfo(
    COLL_INFO *const coll, const int32_t x_pos, const int32_t y_pos,
    const int32_t z_pos, int16_t room_num, const int32_t obj_height)
{
    coll->coll_type = COLL_NONE;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
    coll->quadrant = Math_GetDirection(coll->facing);

    int32_t x = x_pos;
    int32_t z = z_pos;
    const int32_t y = y_pos - obj_height;
    const int32_t y_top = y - 160;

    const SECTOR *sector = Room_GetSector(x, y_top, z, &room_num);
    int32_t height = Room_GetHeight(sector, x, y_top, z);
    int32_t room_height = height;
    if (height != NO_HEIGHT) {
        height -= y_pos;
    }
    int32_t ceiling = Room_GetCeiling(sector, x, y_top, z);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->side_mid.floor = height;
    coll->side_mid.ceiling = ceiling;
    coll->side_mid.type = Room_GetHeightType();

    bool is_on_walkable = M_IsOnWalkable(sector, x, y_top, z, room_height);
    if (is_on_walkable) {
        coll->tilt_z = 0;
        coll->tilt_x = 0;
    } else {
        const ITEM *const lara_item = Lara_GetItem();
        const int16_t tilt = Room_GetTiltType(sector, x, lara_item->pos.y, z);
        coll->tilt_z = tilt >> 8;
        coll->tilt_x = (int8_t)tilt;
    }

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

    // Front.
    x = x_pos + x_front;
    z = z_pos + z_front;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    room_height = height;
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

#if TR_VERSION == 1
    is_on_walkable = M_IsOnWalkable(sector, x, y_top, z, room_height);
#elif TR_VERSION >= 2
    is_on_walkable = false;
#endif
    if (!is_on_walkable) {
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
    }

    // Left.
    x = x_pos + x_left;
    z = z_pos + z_left;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    room_height = height;
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

    is_on_walkable = M_IsOnWalkable(sector, x, y_top, z, room_height);
    if (!is_on_walkable) {
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
    }

    // Right.
    x = x_pos + x_right;
    z = z_pos + z_right;
    sector = Room_GetSector(x, y_top, z, &room_num);
    height = Room_GetHeight(sector, x, y_top, z);
    room_height = height;
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

    is_on_walkable = M_IsOnWalkable(sector, x, y_top, z, room_height);
    if (!is_on_walkable) {
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

        default:
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

        default:
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

        default:
            break;
        }

        coll->coll_type = COLL_RIGHT;
        return;
    }
}
