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
    ptr->pos.x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
    ptr->pos.y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
    ptr->pos.z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
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
        ptr->pos.x = x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        ptr->pos.y = y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        ptr->pos.z = z + (g_MatrixPtr->_23 >> W2V_SHIFT);
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
            int32_t x = ptr2->pos.x - ptr1->pos.x;
            int32_t y = ptr2->pos.y - ptr1->pos.y;
            int32_t z = ptr2->pos.z - ptr1->pos.z;
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
