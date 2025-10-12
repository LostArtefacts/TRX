#include "game/collision.h"

#include "config.h"
#include "game/items.h"
#include "game/lara/common.h"
#include "game/matrix.h"
#include "game/rooms.h"
#include "utils.h"

#define M_HEADROOM 160 // Additional collision space above Lara's head.

static bool M_IsOnWalkable(
    const SECTOR *const sector, const int32_t x, const int32_t y,
    const int32_t z, const int32_t room_height)
{
    return g_Config.gameplay.fix_bridge_collision
        && Room_IsOnWalkable(sector, x, y, z, room_height, NO_ITEM);
}

// Probes the front, left, and right of Lara and fills in the collision info for
// each side. The collision info depends on Lara's state. Her state determines
// how big slope and lava pit sectors are treated. For example, in the walk
// state, Lara won't walk up big slopes or walk down into lava pits.
static void M_FillSide(
    const COLL_INFO *const coll, COLL_SIDE *const side, const int32_t x_pos,
    const int32_t z_pos, const int32_t y_pos, const int32_t obj_height,
    int16_t *const room_num)
{
    const int32_t y = y_pos - obj_height;
    const int32_t y_top = y - M_HEADROOM;

    int16_t local_room_num = *room_num;
    int16_t *const test_room_num =
        g_Config.gameplay.wall_glitch_mode == WALL_GLITCH_FIXED
        ? &local_room_num
        : room_num;

    const SECTOR *const sector =
        Room_GetSector(x_pos, y_top, z_pos, test_room_num);
    int32_t height = Room_GetHeight(sector, x_pos, y_top, z_pos);
    int32_t ceiling = Room_GetCeiling(sector, x_pos, y_top, z_pos);
    const int32_t room_height = height;
    const int32_t room_ceiling = ceiling;
    const bool sim_wall = room_height == ceiling && room_height != NO_HEIGHT
        && sector->ceiling.tilt == 0 && sector->floor.tilt == 0;
    if (height != NO_HEIGHT) {
        height -= y_pos;
    }
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    side->floor = height;
    side->ceiling = ceiling;
    side->type = Room_GetHeightType();

    const bool is_on_walkable =
        M_IsOnWalkable(sector, x_pos, y_top, z_pos, room_height);
    if (!is_on_walkable) {
        if (coll->slopes_are_walls
            && (side->type == HT_BIG_SLOPE || side->type == HT_DIAGONAL)
            && side->floor < 0) {
            side->floor = -32767;
        } else if (
            coll->slopes_are_pits
            && (side->type == HT_BIG_SLOPE || side->type == HT_DIAGONAL)
            && side->floor > 0) {
            side->floor = STEP_L * 2;
        } else if (
            coll->lava_is_pit && side->floor > 0
            && Room_GetPitSector(sector, x_pos, z_pos)->is_death_sector) {
            side->floor = STEP_L * 2;
        }
    } else if (sim_wall) {
        side->floor = NO_HEIGHT;
        side->ceiling = NO_HEIGHT;
    }
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
        Object_ApplyExtraRotation(&extra_rotation, bone->rot, false);

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

            const int32_t dx = ptr2->pos.x - ptr1->pos.x;
            const int32_t dy = ptr2->pos.y - ptr1->pos.y;
            const int32_t dz = ptr2->pos.z - ptr1->pos.z;
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

void Collide_GetJointAbsPosition(
    const ITEM *const item, XYZ_32 *const out_vec, const int32_t joint)
{
    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM_FRAME *const frame = Item_GetBestFrame(item);

    Matrix_PushUnit();
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
        Object_ApplyExtraRotation(&extra_rotation, bone->rot, false);
    }

    Matrix_TranslateRel32(*out_vec);
    out_vec->x = item->pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    out_vec->y = item->pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    out_vec->z = item->pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
    Matrix_Pop();
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
    const int32_t y_top = y - M_HEADROOM;

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

    M_FillSide(
        coll, &coll->side_front, x_pos + x_front, z_pos + z_front, y_pos,
        obj_height, &room_num);
    M_FillSide(
        coll, &coll->side_left, x_pos + x_left, z_pos + z_left, y_pos,
        obj_height, &room_num);
    M_FillSide(
        coll, &coll->side_right, x_pos + x_right, z_pos + z_right, y_pos,
        obj_height, &room_num);
    // TODO: TR3 uses an extra left and right side, purpose to be investigated.

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
        if (coll->side_front.type == HT_DIAGONAL
            || coll->side_front.type == HT_SPLIT_TRI) {
            coll->shift.x = coll->old.x - x;
            coll->shift.z = coll->old.z - z;
        } else {
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
        if (coll->side_left.type == HT_SPLIT_TRI) {
            coll->shift.x = coll->old.x - x;
            coll->shift.z = coll->old.z - z;
        } else {
            switch (coll->quadrant) {
            case DIR_NORTH:
            case DIR_SOUTH:
                coll->shift.x =
                    Room_FindGridShift(x_pos + x_left, x_pos + x_front);
                break;

            case DIR_EAST:
            case DIR_WEST:
                coll->shift.z =
                    Room_FindGridShift(z_pos + z_left, z_pos + z_front);
                break;

            default:
                break;
            }
        }

        coll->coll_type = COLL_LEFT;
        return;
    }

    if (coll->side_right.floor > coll->bad_pos
        || coll->side_right.floor < coll->bad_neg) {
        if (coll->side_right.type == HT_SPLIT_TRI) {
            coll->shift.x = coll->old.x - x;
            coll->shift.z = coll->old.z - z;
        } else {
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
        }

        coll->coll_type = COLL_RIGHT;
        return;
    }
}

bool Collide_CollideStaticObjects(
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

            default:
                break;
            }

            coll->hit_static = 1;
            return true;
        }
    }

    return false;
}
