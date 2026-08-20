#include <trx/game/collision/common.h>

#include <trx/config.h>
#include <trx/core/utils.h>
#include <trx/game/anims/walk.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/items/anim.h>
#include <trx/game/lara/common.h>
#include <trx/game/matrix.h>
#include <trx/game/rooms.h>
#include <trx/version.h>

#define M_HEADROOM 160 // Additional collision space above Lara's head.

static bool M_IsOnWalkable(
    const SECTOR *const sector, const XYZ_32 pos, const int32_t room_height)
{
    return g_Config.gameplay.fix_bridge_collision
        && Room_IsOnWalkable(sector, pos, room_height, NO_ITEM);
}

// Probes the front, left, and right of Lara and fills in the collision info for
// each side. The collision info depends on Lara's state. Her state determines
// how big slope and lava pit sectors are treated. For example, in the walk
// state, Lara won't walk up big slopes or walk down into lava pits.
// The floor data speaks of whole sectors, so an item standing between two of
// them leaves the ground it covers reading as open. A side that cannot be
// reached reads as a wall, and every move that asks the collision info what is
// in front of Lara answers as it would for a real one.
static void M_BlockSideIfUnreachable(
    COLL_SIDE *const side, const XYZ_32 pos, const XZ_32 probe,
    const int32_t obj_height, const int16_t room_num)
{
    if (side->floor == NO_HEIGHT) {
        return;
    }

    const XYZ_32 sample_pos = {
        .x = pos.x + probe.x,
        .y = pos.y,
        .z = pos.z + probe.z,
    };
    // The step of headroom counts something standing on the ledge in front of
    // her, which is as impassable as something at her own height.
    if (Room_IsPathBlocked(pos, sample_pos, room_num, obj_height + STEP_L, 0)) {
        side->floor = NO_HEIGHT;
        side->ceiling = NO_HEIGHT;
    }
}

static void M_FillSide(
    const COLL_INFO *const coll, COLL_SIDE *const side, const XYZ_32 pos,
    const XZ_32 probe, const int32_t obj_height, int16_t *const room_num)
{
    const int32_t y = pos.y - obj_height;
    const int32_t y_top = y - M_HEADROOM;

    int16_t local_room_num = *room_num;
    int16_t *const test_room_num =
        g_Config.gameplay.wall_glitch_mode == WALL_GLITCH_FIXED
        ? &local_room_num
        : room_num;

    const XYZ_32 sample_pos = {
        .x = pos.x + probe.x,
        .y = y_top,
        .z = pos.z + probe.z,
    };
    const SECTOR *sector = Room_GetSector(sample_pos, test_room_num);
    int32_t height = Room_GetHeight(sector, sample_pos);
    int32_t ceiling = Room_GetCeiling(sector, sample_pos);
    const int32_t room_height = height;
    const int32_t room_ceiling = ceiling;
    const bool sim_wall = room_height == ceiling && room_height != NO_HEIGHT
        && !sector->ceiling.is_split && !sector->floor.is_split
        && sector->ceiling.tilt.x == 0 && sector->ceiling.tilt.z == 0
        && sector->floor.tilt.x == 0 && sector->floor.tilt.z == 0;
    if (height != NO_HEIGHT) {
        height -= pos.y;
    }
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    side->floor = height;
    side->ceiling = ceiling;
    side->type = Room_GetHeightType();

    const bool is_on_walkable = M_IsOnWalkable(sector, sample_pos, room_height);

    const bool retest_front = side == &coll->side_front && g_TRVersion >= 3;
    if (retest_front) {
        XYZ_32 front_probe_pos = sample_pos;
        front_probe_pos.x += probe.x;
        front_probe_pos.z += probe.z;
        sector = Room_GetSector(front_probe_pos, room_num);
        height = Room_GetHeight(sector, front_probe_pos);
        if (height != NO_HEIGHT) {
            height -= pos.y;
        }
    }

    if (!is_on_walkable) {
        if (coll->slopes_are_walls
            && (side->type == HT_BIG_SLOPE || side->type == HT_DIAGONAL)
            && side->floor < 0
            && (!retest_front
                || (side->floor < coll->side_mid.floor
                    && height < side->floor))) {
            side->floor = UNDEFINED_HEIGHT;
        } else if (
            coll->slopes_are_pits
            && (side->type == HT_BIG_SLOPE || side->type == HT_DIAGONAL)
            && side->floor > (retest_front ? coll->side_mid.floor : 0)) {
            side->floor = STEP_L * 2;
        } else if (
            coll->lava_is_pit && side->floor > 0
            && Room_GetPitSector(sector, pos.x, pos.z)->is_death_sector) {
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
    const OBJECT *const obj = Object_Get(item->object_id);

    ANIM_WALK walk;
    Anim_Walk_Begin(
        &walk,
        &(ANIM_WALK_DESC) {
            .obj = obj,
            .pose = Anim_Pose_FromFrame(frame),
            .extra_rotations = item->extra_rotations,
        });
    while (Anim_Walk_Next(&walk)) {
        const OBJECT_MESH *const mesh =
            Object_GetMesh(obj->mesh_idx + walk.joint);
        const XYZ_32 center =
            Anim_Walk_GetPos(&walk, XYZ_32_From16(mesh->center));
        SPHERE *const sphere = &spheres[walk.joint];
        sphere->pos.x = pos.x + center.x;
        sphere->pos.y = pos.y + center.y;
        sphere->pos.z = pos.z + center.z;
        sphere->r = mesh->radius;
    }
    Anim_Walk_End(&walk);

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
    ANIM_FRAME *frames[2] = { nullptr, nullptr };
    int32_t rate = 0;
    const int32_t frac = Item_GetFrames(item, frames, &rate);
    const bool use_item_interp =
        Interpolation_IsActive() && item->enable_interpolation;
    const XYZ_32 item_pos =
        use_item_interp ? item->interp.result.pos : item->pos;
    const XYZ_16 item_rot =
        use_item_interp ? item->interp.result.rot : item->rot;

    if (frames[0] == nullptr) {
        Matrix_PushUnit();
        Matrix_Rot16(item_rot);
        Matrix_TranslateRel32(*out_vec);
        out_vec->x = item_pos.x + (g_MatrixPtr->_03 >> W2V_SHIFT);
        out_vec->y = item_pos.y + (g_MatrixPtr->_13 >> W2V_SHIFT);
        out_vec->z = item_pos.z + (g_MatrixPtr->_23 >> W2V_SHIFT);
        Matrix_Pop();
        return;
    }

    const ANIM_FRAME *const frame_a = frames[0];
    const ANIM_FRAME *const frame_b = frames[1];

    Matrix_PushUnit();
    Matrix_Rot16(item_rot);

    ANIM_WALK walk;
    Anim_Walk_BeginToJoint(
        &walk,
        &(ANIM_WALK_DESC) {
            .obj = obj,
            .pose = Anim_Pose_FromFrames(frame_a, frame_b, frac, rate),
            .extra_rotations = item->extra_rotations,
        },
        MIN(joint, MAX(obj->mesh_count - 1, 0)));
    while (Anim_Walk_Next(&walk)) {}

    *out_vec = XYZ_32_Add(item_pos, Anim_Walk_GetPos(&walk, *out_vec));
    Anim_Walk_End(&walk);
    Matrix_Pop();
}

void Collide_GetCollisionInfo(
    COLL_INFO *const coll, const XYZ_32 pos, int16_t room_num,
    int32_t obj_height)
{
    coll->coll_type = COLL_NONE;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
    coll->quadrant = Math_GetDirection(coll->facing);

    bool reset_room = false;
    int16_t prev_room_num = room_num;
    if (obj_height < 0) {
        reset_room = true;
        obj_height = -obj_height;
    }

    const int32_t y = pos.y - obj_height;
    const int32_t y_top = y - M_HEADROOM;

    const XYZ_32 sample_pos = { .x = pos.x, .y = y_top, .z = pos.z };
    const SECTOR *sector = Room_GetSector(sample_pos, &room_num);
    int32_t height = Room_GetHeight(sector, sample_pos);
    const int32_t room_height = height;
    if (height != NO_HEIGHT) {
        height -= pos.y;
    }
    int32_t ceiling = Room_GetCeiling(sector, sample_pos);
    if (ceiling != NO_HEIGHT) {
        ceiling -= y;
    }

    coll->side_mid.floor = height;
    coll->side_mid.ceiling = ceiling;
    coll->side_mid.type = Room_GetHeightType();

    bool is_on_walkable = M_IsOnWalkable(sector, sample_pos, room_height);
    if (is_on_walkable) {
        coll->tilt = (XZ_16) {};
    } else {
        const ITEM *const lara_item = Lara_GetItem();
        coll->tilt = Room_GetTiltType(
            sector, (XYZ_32) { pos.x, lara_item->pos.y, pos.z });
    }

    XZ_32 probe_left = {};
    XZ_32 probe_right = {};
    XZ_32 probe_front = {};
    switch (coll->quadrant) {
    case DIR_NORTH:
        probe_front.x = (coll->radius * Math_Sin(coll->facing)) >> W2V_SHIFT;
        probe_front.z = coll->radius;
        probe_left.x = -coll->radius;
        probe_left.z = coll->radius;
        probe_right.x = coll->radius;
        probe_right.z = coll->radius;
        break;

    case DIR_EAST:
        probe_front.x = coll->radius;
        probe_front.z = (coll->radius * Math_Cos(coll->facing)) >> W2V_SHIFT;
        probe_left.x = coll->radius;
        probe_left.z = coll->radius;
        probe_right.x = coll->radius;
        probe_right.z = -coll->radius;
        break;

    case DIR_SOUTH:
        probe_front.x = (coll->radius * Math_Sin(coll->facing)) >> W2V_SHIFT;
        probe_front.z = -coll->radius;
        probe_left.x = coll->radius;
        probe_left.z = -coll->radius;
        probe_right.x = -coll->radius;
        probe_right.z = -coll->radius;
        break;

    case DIR_WEST:
        probe_front.x = -coll->radius;
        probe_front.z = (coll->radius * Math_Cos(coll->facing)) >> W2V_SHIFT;
        probe_left.x = -coll->radius;
        probe_left.z = -coll->radius;
        probe_right.x = -coll->radius;
        probe_right.z = coll->radius;
        break;

    default:
        break;
    }

    if (reset_room) {
        room_num = prev_room_num;
    }

    M_FillSide(
        coll, &coll->side_front, pos, probe_front, obj_height, &room_num);

    int16_t room_num2;
    room_num2 = prev_room_num;
    M_FillSide(coll, &coll->side_left, pos, probe_left, obj_height, &room_num2);
    room_num2 = prev_room_num;
    M_FillSide(
        coll, &coll->side_right, pos, probe_right, obj_height, &room_num2);

    M_FillSide(coll, &coll->side_left2, pos, probe_left, obj_height, &room_num);
    M_FillSide(
        coll, &coll->side_right2, pos, probe_right, obj_height, &room_num);

    COLL_SIDE *const sides[] = {
        &coll->side_front, &coll->side_left,   &coll->side_right,
        &coll->side_left2, &coll->side_right2,
    };
    const XZ_32 probes[] = {
        probe_front, probe_left, probe_right, probe_left, probe_right,
    };
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(sides); i++) {
        M_BlockSideIfUnreachable(
            sides[i], pos, probes[i], obj_height, prev_room_num);
    }

    const int16_t static_room_num = g_TRVersion >= 3 ? prev_room_num : room_num;
    if (Collide_CollideStaticObjects(coll, pos, static_room_num, obj_height)) {
        const XYZ_32 test_pos = {
            .x = pos.x + coll->shift.x,
            .y = pos.y,
            .z = pos.z + coll->shift.z,
        };
        sector = Room_GetSector(test_pos, &room_num);
        if (Room_GetHeight(sector, test_pos) < test_pos.y - WALL_L / 2
            || Room_GetCeiling(sector, test_pos) > y) {
            coll->shift.x = -coll->shift.x;
            coll->shift.z = -coll->shift.z;
        }
    }

    if (coll->side_mid.floor == NO_HEIGHT) {
        coll->shift.x = coll->old_pos.x - pos.x;
        coll->shift.y = coll->old_pos.y - pos.y;
        coll->shift.z = coll->old_pos.z - pos.z;
        coll->coll_type = COLL_FRONT;
        return;
    }

    if (coll->side_mid.floor - coll->side_mid.ceiling <= 0) {
        coll->shift.x = coll->old_pos.x - pos.x;
        coll->shift.y = coll->old_pos.y - pos.y;
        coll->shift.z = coll->old_pos.z - pos.z;
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
            coll->shift.x = coll->old_pos.x - pos.x;
            coll->shift.z = coll->old_pos.z - pos.z;
        } else {
            switch (coll->quadrant) {
            case DIR_NORTH:
            case DIR_SOUTH:
                coll->shift.x = coll->old_pos.x - pos.x;
                coll->shift.z =
                    Room_FindGridShift(pos.z + probe_front.z, pos.z);
                break;

            case DIR_EAST:
            case DIR_WEST:
                coll->shift.x =
                    Room_FindGridShift(pos.x + probe_front.x, pos.x);
                coll->shift.z = coll->old_pos.z - pos.z;
                break;

            default:
                break;
            }
        }

        coll->coll_type = COLL_FRONT;
        return;
    }

    if (coll->side_front.ceiling >= coll->bad_ceiling) {
        coll->shift.x = coll->old_pos.x - pos.x;
        coll->shift.y = coll->old_pos.y - pos.y;
        coll->shift.z = coll->old_pos.z - pos.z;
        coll->coll_type = COLL_TOP_FRONT;
        return;
    }

    if (coll->side_left.floor > coll->bad_pos
        || coll->side_left.floor < coll->bad_neg) {
        if (coll->side_left.type == HT_SPLIT_TRI) {
            coll->shift.x = coll->old_pos.x - pos.x;
            coll->shift.z = coll->old_pos.z - pos.z;
        } else {
            switch (coll->quadrant) {
            case DIR_NORTH:
            case DIR_SOUTH:
                coll->shift.x = Room_FindGridShift(
                    pos.x + probe_left.x, pos.x + probe_front.x);
                break;

            case DIR_EAST:
            case DIR_WEST:
                coll->shift.z = Room_FindGridShift(
                    pos.z + probe_left.z, pos.z + probe_front.z);
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
            coll->shift.x = coll->old_pos.x - pos.x;
            coll->shift.z = coll->old_pos.z - pos.z;
        } else {
            switch (coll->quadrant) {
            case DIR_NORTH:
            case DIR_SOUTH:
                coll->shift.x = Room_FindGridShift(
                    pos.x + probe_right.x, pos.x + probe_front.x);
                break;

            case DIR_EAST:
            case DIR_WEST:
                coll->shift.z = Room_FindGridShift(
                    pos.z + probe_right.z, pos.z + probe_front.z);
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
    COLL_INFO *const coll, const XYZ_32 pos, const int16_t room_num,
    const int32_t height)
{
    coll->hit_static = 0;

    const int32_t in_x_min = pos.x - coll->radius;
    const int32_t in_x_max = pos.x + coll->radius;
    const int32_t in_y_min = pos.y - height;
    const int32_t in_y_max = pos.y;
    const int32_t in_z_min = pos.z - coll->radius;
    const int32_t in_z_max = pos.z + coll->radius;
    XYZ_32 shifter = { .x = 0, .z = 0 };

    Room_GetNearbyRooms(pos, coll->radius + 50, height + 50, room_num);

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

            coll->hit_static = 1;
            if (g_Config.gameplay.enable_soft_statics) {
                return true;
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
                    coll->shift.x = coll->old_pos.x - pos.x;
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
                    coll->shift.z = coll->old_pos.z - pos.z;
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
                    coll->shift.x = coll->old_pos.x - pos.x;
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
                    coll->shift.z = coll->old_pos.z - pos.z;
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

            return true;
        }
    }

    return false;
}

bool Collide_TestBoundsCollide(
    const COLL_ITEM *const src_item, const COLL_ITEM *const dst_item,
    const int32_t radius)
{
    const BOUNDS_16 *const src_bounds = &src_item->bounds;
    const BOUNDS_16 *const dst_bounds = &dst_item->bounds;

    if (src_item->pos.y + src_bounds->min.y
            >= dst_item->pos.y + dst_bounds->max.y
        || src_item->pos.y + src_bounds->max.y
            <= dst_item->pos.y + dst_bounds->min.y) {
        return false;
    }

    const XYZ_32 delta = XYZ_32_Subtract(dst_item->pos, src_item->pos);
    const XYZ_32 local = XYZ_32_UnrotateYaw(delta, src_item->rot.y);

    // clang-format off
    return (
        local.x >= src_bounds->min.x - radius &&
        local.x <= src_bounds->max.x + radius &&
        local.z >= src_bounds->min.z - radius &&
        local.z <= src_bounds->max.z + radius);
    // clang-format on
}

void Collide_DoProperDetection(ITEM *const item, const XYZ_32 old_pos)
{
    int16_t room_num = item->room_num;
    const SECTOR *sector = Room_GetSector(old_pos, &room_num);
    const int32_t old_height = Room_GetHeight(sector, old_pos);
    const HEIGHT_TYPE old_type = Room_GetHeightType();

    room_num = item->room_num;
    sector = Room_GetSector(item->pos, &room_num);
    int32_t height = Room_GetHeight(sector, item->pos);

    bool bs;
    if (item->pos.y >= height) {
        bs = false;

        if ((old_type == HT_BIG_SLOPE || old_type == HT_DIAGONAL)
            && old_height < height) {
            int32_t y_ang = (uint16_t)item->rot.y;

            const XZ_16 tilt = Room_GetTiltType(sector, item->pos);
            if (tilt.x < 0) {
                if (y_ang >= DEG_180) {
                    bs = true;
                }
            } else if (tilt.x > 0) {
                if (y_ang <= DEG_180) {
                    bs = true;
                }
            }

            if (tilt.z < 0) {
                if (y_ang >= DEG_90 && y_ang <= DEG_270) {
                    bs = true;
                }
            } else if (tilt.z > 0) {
                if (y_ang <= DEG_90 || y_ang >= DEG_270) {
                    bs = true;
                }
            }
        }

        const bool x_cross = ROUND_TO_SECTOR(item->pos.x ^ old_pos.x) != 0;
        const bool z_cross = ROUND_TO_SECTOR(item->pos.z ^ old_pos.z) != 0;
        if (old_pos.y > height + 32 && !bs && (x_cross || z_cross)) {
            const bool xs = x_cross && z_cross
                ? ABS(old_pos.x - item->pos.x) < ABS(old_pos.z - item->pos.z)
                : true;
            item->rot.y = x_cross && xs ? -item->rot.y : -DEG_180 - item->rot.y;
            item->pos = old_pos;
            item->speed >>= 1;
        } else if (old_type != HT_BIG_SLOPE && old_type != HT_DIAGONAL) {
            if (item->fall_speed > 0) {
                if (item->fall_speed > 16) {
                    if (item->object_id == O_GRENADE) {
                        item->fall_speed =
                            (item->fall_speed >> 1) - item->fall_speed;
                    } else {
                        item->fall_speed = -(item->fall_speed >> 2);

                        if (item->fall_speed < -100) {
                            item->fall_speed = -100;
                        }
                    }
                } else {
                    item->fall_speed = 0;

                    if (item->object_id == O_GRENADE) {
                        item->speed--;
                        item->required_anim_state = 1;
                        item->rot.x = 0;
                    } else {
                        item->speed -= 3;
                    }

                    if (item->speed < 0) {
                        item->speed = 0;
                    }
                }
            }

            item->pos.y = height;
        } else {
            item->speed -= item->speed >> 2;

            const XZ_16 tilt = Room_GetTiltType(sector, item->pos);
            if (tilt.x < 0 && ABS(tilt.x) - ABS(tilt.z) >= MAX_SLOPE) {
                if ((uint16_t)item->rot.y > DEG_180) {
                    item->rot.y = -1 - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed -= tilt.x << 1;

                        if ((uint16_t)item->rot.y > DEG_90
                            && (uint16_t)item->rot.y < DEG_270) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_90) {
                                item->rot.y = DEG_90;
                            }
                        } else if ((uint16_t)item->rot.y < DEG_90) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_90) {
                                item->rot.y = DEG_90;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.x > 0 && ABS(tilt.x) - ABS(tilt.z) >= MAX_SLOPE) {
                if ((uint16_t)item->rot.y < DEG_180) {
                    item->rot.y = -1 - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed += tilt.x << 1;

                        if ((uint16_t)item->rot.y > DEG_270
                            || (uint16_t)item->rot.y < DEG_90) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_270) {
                                item->rot.y = -DEG_90;
                            }
                        } else if ((uint16_t)item->rot.y < DEG_270) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_270) {
                                item->rot.y = -DEG_90;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.z < 0 && ABS(tilt.z) - ABS(tilt.x) >= MAX_SLOPE) {
                if ((uint16_t)item->rot.y > DEG_90
                    && (uint16_t)item->rot.y < DEG_270) {
                    item->rot.y = 0x7FFF - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed -= tilt.z << 1;

                        if ((uint16_t)item->rot.y < DEG_180) {
                            item->rot.y -= DEG_90;

                            if ((uint16_t)item->rot.y > 61440) {
                                item->rot.y = 0;
                            }
                        } else {
                            item->rot.y += DEG_90;

                            if ((uint16_t)item->rot.y < DEG_90) {
                                item->rot.y = 0;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.z > 0 && ABS(tilt.z) - ABS(tilt.x) >= MAX_SLOPE) {
                if ((uint16_t)item->rot.y > DEG_270
                    || (uint16_t)item->rot.y < DEG_90) {
                    item->rot.y = 0x7FFF - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed += tilt.z << 1;

                        if ((uint16_t)item->rot.y > DEG_180) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_180) {
                                item->rot.y = -DEG_180;
                            }
                        } else if ((uint16_t)item->rot.y < DEG_180) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_180) {
                                item->rot.y = -DEG_180;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.x < 0 && tilt.z < 0) {
                if ((uint16_t)item->rot.y > DEG_135
                    && (uint16_t)item->rot.y < DEG_315) {
                    item->rot.y = -(DEG_90 + 1) - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed -= tilt.z + tilt.x;

                        if ((uint16_t)item->rot.y > DEG_45
                            && (uint16_t)item->rot.y < DEG_225) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_45) {
                                item->rot.y = DEG_45;
                            }
                        } else if (item->rot.y != DEG_45) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_45) {
                                item->rot.y = DEG_45;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.x < 0 && tilt.z > 0) {
                if ((uint16_t)item->rot.y > DEG_225
                    || (uint16_t)item->rot.y < DEG_45) {
                    item->rot.y = DEG_90 - 1 - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed += tilt.z - tilt.x;

                        if ((uint16_t)item->rot.y < DEG_315
                            && (uint16_t)item->rot.y > DEG_135) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_135) {
                                item->rot.y = DEG_135;
                            }
                        } else if (item->rot.y != DEG_135) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_135) {
                                item->rot.y = DEG_135;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.x > 0 && tilt.z > 0) {
                if ((uint16_t)item->rot.y > DEG_315
                    || (uint16_t)item->rot.y < DEG_135) {
                    item->rot.y = -(DEG_90 + 1) - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed += tilt.z + tilt.x;

                        if ((uint16_t)item->rot.y < DEG_45
                            || (uint16_t)item->rot.y > DEG_225) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_225) {
                                item->rot.y = -DEG_135;
                            }
                        } else if ((uint16_t)item->rot.y != DEG_225) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_225) {
                                item->rot.y = -DEG_135;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            } else if (tilt.x > 0 && tilt.z < 0) {
                if ((uint16_t)item->rot.y > DEG_45
                    && (uint16_t)item->rot.y < DEG_225) {
                    item->rot.y = DEG_90 - 1 - item->rot.y;

                    if (item->fall_speed > 0) {
                        item->fall_speed = -(item->fall_speed >> 1);
                    }
                } else {
                    if (item->speed < 32) {
                        item->speed += tilt.x - tilt.z;

                        if ((uint16_t)item->rot.y < DEG_135
                            || (uint16_t)item->rot.y > DEG_315) {
                            item->rot.y -= 0x1000;

                            if ((uint16_t)item->rot.y < DEG_315) {
                                item->rot.y = -DEG_45;
                            }
                        } else if ((uint16_t)item->rot.y != DEG_315) {
                            item->rot.y += 0x1000;

                            if ((uint16_t)item->rot.y > DEG_315) {
                                item->rot.y = -DEG_45;
                            }
                        }
                    }

                    item->fall_speed =
                        item->fall_speed > 0 ? -(item->fall_speed >> 1) : 0;
                }
            }

            item->pos = old_pos;
        }
    } else {
        if (item->fall_speed >= 0) {
            const XYZ_32 test_pos = {
                item->pos.x,
                old_pos.y,
                item->pos.z,
            };
            room_num = item->room_num;
            sector = Room_GetSector(test_pos, &room_num);
            height = Room_GetHeight(sector, test_pos);
            const bool on_walkable =
                Room_IsOnWalkable(sector, test_pos, height, NO_ITEM);

            if (item->pos.y >= height && on_walkable) {
                if (item->fall_speed > 16) {
                    if (item->object_id == O_GRENADE) {
                        item->fall_speed = -(item->fall_speed / 2);
                    } else {
                        item->fall_speed = -(item->fall_speed / 4);
                        CLAMPL(item->fall_speed, -100);
                    }
                } else if (item->fall_speed > 0) {
                    item->fall_speed = 0;
                    if (item->object_id == O_GRENADE) {
                        item->speed--;
                        item->required_anim_state = 1;
                        item->rot.x = 0;
                    } else {
                        item->speed -= 3;
                    }
                    CLAMPL(item->speed, 0);
                }
                item->pos.y = height;
            }
        }

        room_num = item->room_num;
        sector = Room_GetSector(item->pos, &room_num);
        int32_t ceiling = Room_GetCeiling(sector, item->pos);

        if (item->pos.y < ceiling) {
            const bool x_cross = ROUND_TO_SECTOR(item->pos.x ^ old_pos.x) != 0;
            const bool z_cross = ROUND_TO_SECTOR(item->pos.z ^ old_pos.z) != 0;
            if (old_pos.y < ceiling && (x_cross || z_cross)) {
                item->rot.y = x_cross ? -item->rot.y : -DEG_180 - item->rot.y;

                if (item->object_id == O_GRENADE) {
                    item->speed -= item->speed >> 3;
                } else {
                    item->speed >>= 1;
                }

                item->pos = old_pos;
            } else {
                item->pos.y = ceiling;
            }

            if (item->fall_speed < 0) {
                item->fall_speed = -item->fall_speed;
            }
        }
    }

    room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);

    if (item->room_num != room_num) {
        Item_UpdateRoom(Item_GetIndex(item), room_num);
    }
}

void Collide_ShiftItem(ITEM *const item, COLL_INFO *const coll)
{
    item->pos.x += coll->shift.x;
    item->pos.y += coll->shift.y;
    item->pos.z += coll->shift.z;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
}
