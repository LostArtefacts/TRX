#include "game/collide.h"

#include "game/items.h"
#include "game/room.h"
#include "global/vars.h"

#include <libtrx/game/math.h>
#include <libtrx/game/matrix.h>
#include <libtrx/utils.h>

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
