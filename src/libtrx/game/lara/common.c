#include "game/lara/common.h"

#include "config.h"
#include "game/const.h"
#include "game/item_actions.h"
#include "game/lara/const.h"
#include "game/matrix.h"
#include "game/rooms.h"

#define M_MOVE_ANIM_VELOCITY 12
#define M_MOVE_SPEED 16
#define M_MOVE_ANGLE (2 * DEG_1) // = 364

void Lara_Animate(ITEM *const item)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    item->frame_num++;

    const ANIM *anim = Item_GetAnim(item);
    if (anim->num_changes > 0 && Item_GetAnimChange(item, anim)) {
        anim = Item_GetAnim(item);
        item->current_anim_state = anim->current_anim_state;
    }

    if (item->frame_num > anim->frame_end) {
        for (int32_t i = 0; i < anim->num_commands; i++) {
            const ANIM_COMMAND *const command = &anim->commands[i];
            switch (command->type) {
            case AC_MOVE_ORIGIN: {
                const XYZ_16 *const pos = (XYZ_16 *)command->data;
                Item_Translate(item, pos->x, pos->y, pos->z);
                break;
            }

            case AC_JUMP_VELOCITY: {
                const ANIM_COMMAND_VELOCITY_DATA *const data =
                    (ANIM_COMMAND_VELOCITY_DATA *)command->data;
                item->fall_speed = data->fall_speed;
                item->speed = data->speed;
                item->gravity = true;
                if (lara->calc_fall_speed != 0) {
                    item->fall_speed = lara->calc_fall_speed;
                    lara->calc_fall_speed = 0;
                }
                break;
            }

            case AC_ATTACK_READY:
                if (lara->gun_status != LGS_SPECIAL) {
                    lara->gun_status = LGS_ARMLESS;
                }
                break;
            default:
                break;
            }
        }

        item->anim_num = anim->jump_anim_num;
        item->frame_num = anim->jump_frame_num;
        anim = Item_GetAnim(item);
        item->current_anim_state = anim->current_anim_state;
    }

    for (int32_t i = 0; i < anim->num_commands; i++) {
        const ANIM_COMMAND *const command = &anim->commands[i];

        switch (command->type) {
        case AC_SOUND_FX: {
            const ANIM_COMMAND_EFFECT_DATA *const data =
                (ANIM_COMMAND_EFFECT_DATA *)command->data;
            Item_PlayAnimSFX(item, data);
            break;
        }

        case AC_EFFECT: {
            const ANIM_COMMAND_EFFECT_DATA *const data =
                (ANIM_COMMAND_EFFECT_DATA *)command->data;
            if (item->frame_num != data->frame_num) {
                break;
            }

            const ANIM_COMMAND_ENVIRONMENT type = data->environment;
            const int32_t height = lara->water_surface_dist;
            if ((type == ACE_WATER && (height >= 0 || height == NO_HEIGHT))
                || (type == ACE_LAND && height < 0 && height != NO_HEIGHT)) {
                break;
            }

            ItemAction_Run(data->effect_num, item);
            break;
        }

        default:
            break;
        }
    }

    if (item->gravity != 0) {
        int32_t speed = anim->velocity
            + anim->acceleration * (item->frame_num - anim->frame_base - 1);
        item->speed -= (int16_t)(speed >> 16);
        speed += anim->acceleration;
        item->speed += (int16_t)(speed >> 16);

        item->fall_speed += item->fall_speed < FAST_FALL_SPEED ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    } else {
        int32_t speed = anim->velocity;
        if (anim->acceleration != 0) {
            speed += anim->acceleration * (item->frame_num - anim->frame_base);
        }
        item->speed = (int16_t)(speed >> 16);
    }

    item->pos.x += (item->speed * Math_Sin(lara->move_angle)) >> W2V_SHIFT;
    item->pos.z += (item->speed * Math_Cos(lara->move_angle)) >> W2V_SHIFT;
}

void Lara_SwapSingleMesh(const LARA_MESH mesh, const GAME_OBJECT_ID obj_id)
{
    const OBJECT *const obj = Object_Get(obj_id);
    Lara_SetMesh(mesh, Object_GetMesh(obj->mesh_idx + mesh));
}

OBJECT_MESH *Lara_GetMesh(const LARA_MESH mesh)
{
    return Lara_GetLaraInfo()->mesh_ptrs[mesh];
}

void Lara_SetMesh(const LARA_MESH mesh, OBJECT_MESH *const mesh_ptr)
{
    Lara_GetLaraInfo()->mesh_ptrs[mesh] = mesh_ptr;
}

const ANIM_FRAME *Lara_GetHitFrame(const ITEM *const item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->hit_direction < 0) {
        return nullptr;
    }

    // clang-format off
    LARA_ANIMATION anim_idx;
    switch (lara->hit_direction) {
    case DIR_EAST:  anim_idx = LA_HIT_LEFT; break;
    case DIR_SOUTH: anim_idx = LA_HIT_BACK; break;
    case DIR_WEST:  anim_idx = LA_HIT_RIGHT; break;
    default:        anim_idx = LA_HIT_FRONT; break;
    }
    // clang-format on

    const OBJECT *const obj = Object_Get(item->object_id);
    const ANIM *const anim = Object_GetAnim(obj, anim_idx);
    return &anim->frame_ptr[lara->hit_frame];
}

void Lara_TakeDamage(const int16_t damage, const bool hit_status)
{
    Item_TakeDamage(Lara_GetItem(), damage, hit_status);
}

bool Lara_TestBoundsCollide(const ITEM *const item, const int32_t radius)
{
    return Item_TestBoundsCollide(item, Lara_GetItem(), radius);
}

bool Lara_TestPosition(
    const ITEM *const item, const OBJECT_BOUNDS *const bounds)
{
    const ITEM *const lara = Lara_GetItem();
    const XYZ_16 rot = {
        .x = lara->rot.x - item->rot.x,
        .y = lara->rot.y - item->rot.y,
        .z = lara->rot.z - item->rot.z,
    };
    const XYZ_32 dist = {
        .x = lara->pos.x - item->pos.x,
        .y = lara->pos.y - item->pos.y,
        .z = lara->pos.z - item->pos.z,
    };

    // clang-format off
    if (rot.x < bounds->rot.min.x ||
        rot.x > bounds->rot.max.x ||
        rot.y < bounds->rot.min.y ||
        rot.y > bounds->rot.max.y ||
        rot.z < bounds->rot.min.z ||
        rot.z > bounds->rot.max.z
    ) {
        return false;
    }
    // clang-format on

    Matrix_PushUnit();
    Matrix_Rot16(item->rot);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (dist.x * m->_00 + dist.y * m->_10 + dist.z * m->_20) >> W2V_SHIFT,
        .y = (dist.x * m->_01 + dist.y * m->_11 + dist.z * m->_21) >> W2V_SHIFT,
        .z = (dist.x * m->_02 + dist.y * m->_12 + dist.z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    // clang-format off
    return (
        shift.x >= bounds->shift.min.x &&
        shift.x <= bounds->shift.max.x &&
        shift.y >= bounds->shift.min.y &&
        shift.y <= bounds->shift.max.y &&
        shift.z >= bounds->shift.min.z &&
        shift.z <= bounds->shift.max.z
    );
    // clang-format on
}

void Lara_AlignPosition(const ITEM *const item, const XYZ_32 *const vec)
{
    ITEM *const lara = Lara_GetItem();
    lara->rot = item->rot;
    Matrix_PushUnit();
    Matrix_Rot16(item->rot);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (vec->x * m->_00 + vec->y * m->_01 + vec->z * m->_02) >> W2V_SHIFT,
        .y = (vec->x * m->_10 + vec->y * m->_11 + vec->z * m->_12) >> W2V_SHIFT,
        .z = (vec->x * m->_20 + vec->y * m->_21 + vec->z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    const XYZ_32 new_pos = {
        .x = item->pos.x + shift.x,
        .y = item->pos.y + shift.y,
        .z = item->pos.z + shift.z,
    };

    int16_t room_num = lara->room_num;
    const SECTOR *const sector =
        Room_GetSector(new_pos.x, new_pos.y, new_pos.z, &room_num);
    const int32_t height =
        Room_GetHeight(sector, new_pos.x, new_pos.y, new_pos.z);
    const int32_t ceiling =
        Room_GetCeiling(sector, new_pos.x, new_pos.y, new_pos.z);

    if (ABS(height - lara->pos.y) > STEP_L
        || ABS(ceiling - lara->pos.y) < LARA_HEIGHT) {
        return;
    }

    lara->pos = new_pos;
}

bool Lara_IsNearItem(const XYZ_32 *const pos, const int32_t distance)
{
    const ITEM *const item = Lara_GetItem();
    const XYZ_32 d = {
        .x = pos->x - item->pos.x,
        .y = pos->y - item->pos.y,
        .z = pos->z - item->pos.z,
    };
    if (ABS(d.x) > distance || ABS(d.z) > distance || ABS(d.y) > WALL_L * 3) {
        return false;
    }

    if (SQUARE(d.x) + SQUARE(d.z) > SQUARE(distance)) {
        return false;
    }

    const BOUNDS_16 *const bounds = Item_GetBoundsAccurate(item);
    return d.y >= bounds->min.y && d.y <= bounds->max.y + 100;
}

bool Lara_MovePosition(const ITEM *const ref_item, const XYZ_32 *const vec)
{
    LARA_INFO *const lara_info = Lara_GetLaraInfo();
#if TR_VERSION == 1
    const int32_t velocity = g_Config.gameplay.enable_walk_to_items
            && lara_info->water_status != LWS_UNDERWATER
        ? M_MOVE_ANIM_VELOCITY
        : M_MOVE_SPEED;
#else
    const int32_t velocity = M_MOVE_SPEED;
#endif

    ITEM *const lara_item = Lara_GetItem();
    const XYZ_16 new_rot = ref_item->rot;

    Matrix_PushUnit();
    Matrix_Rot16(new_rot);
    const MATRIX *const m = g_MatrixPtr;
    const XYZ_32 shift = {
        .x = (vec->y * m->_01 + vec->z * m->_02 + vec->x * m->_00) >> W2V_SHIFT,
        .y = (vec->x * m->_10 + vec->z * m->_12 + vec->y * m->_11) >> W2V_SHIFT,
        .z = (vec->y * m->_21 + vec->x * m->_20 + vec->z * m->_22) >> W2V_SHIFT,
    };
    Matrix_Pop();

    const XYZ_32 new_pos = {
        .x = ref_item->pos.x + shift.x,
        .y = ref_item->pos.y + shift.y,
        .z = ref_item->pos.z + shift.z,
    };

#if TR_VERSION == 2
    if (ref_item->object_id == O_FLARE_ITEM) {
        int16_t room_num = lara_item->room_num;
        const SECTOR *const sector =
            Room_GetSector(new_pos.x, new_pos.y, new_pos.z, &room_num);
        const int32_t height =
            Room_GetHeight(sector, new_pos.x, new_pos.y, new_pos.z);
        if (ABS(height - lara_item->pos.y) > STEP_L * 2) {
            return false;
        }
        if (XYZ_32_GetDistance(&new_pos, &lara_item->pos) < STEP_L) {
            return true;
        }
    }
#endif

    const XYZ_32 dpos = {
        .x = new_pos.x - lara_item->pos.x,
        .y = new_pos.y - lara_item->pos.y,
        .z = new_pos.z - lara_item->pos.z,
    };
    const int32_t dist = XYZ_32_GetDistance0(&dpos);
    if (velocity >= dist) {
        lara_item->pos = new_pos;
    } else {
        lara_item->pos.x += velocity * dpos.x / dist;
        lara_item->pos.y += velocity * dpos.y / dist;
        lara_item->pos.z += velocity * dpos.z / dist;
    }

#if TR_VERSION == 1
    if (g_Config.gameplay.enable_walk_to_items
        && !lara_info->interact_target.is_moving) {
        if (lara_info->water_status != LWS_UNDERWATER) {
            const int16_t step_to_anim_num[4] = {
                LA_SIDE_STEP_LEFT,
                LA_WALK_FORWARD,
                LA_SIDE_STEP_RIGHT,
                LA_WALK_BACK,
            };
            const int16_t step_to_anim_state[4] = {
                LS_STEP_LEFT,
                LS_WALK,
                LS_STEP_RIGHT,
                LS_BACK,
            };

            const int32_t dx = lara_item->pos.x - new_pos.x;
            const int32_t dz = lara_item->pos.z - new_pos.z;
            const int32_t angle = (DEG_360 - Math_Atan(dx, dz)) % DEG_360;
            const uint32_t src_quadrant = (uint32_t)(angle + DEG_45) / DEG_90;
            const uint32_t dst_quadrant =
                (uint32_t)(new_rot.y + DEG_45) / DEG_90;
            const DIRECTION quadrant = (src_quadrant - dst_quadrant) % 4;

            Item_SwitchToAnim(lara_item, step_to_anim_num[quadrant], 0);
            lara_item->goal_anim_state = step_to_anim_state[quadrant];
            lara_item->current_anim_state = step_to_anim_state[quadrant];

            lara_info->gun_status = LGS_HANDS_BUSY;
        }

        lara_info->interact_target.is_moving = true;
        lara_info->interact_target.move_count = 0;
    }
#endif

    const int16_t rotation = M_MOVE_ANGLE;
    ITEM_ADJUST_ROT(lara_item->rot.x, new_rot.x, rotation);
    ITEM_ADJUST_ROT(lara_item->rot.y, new_rot.y, rotation);
    ITEM_ADJUST_ROT(lara_item->rot.z, new_rot.z, rotation);

    // clang-format off
    return lara_item->pos.x == new_pos.x
        && lara_item->pos.y == new_pos.y
        && lara_item->pos.z == new_pos.z
        && lara_item->rot.x == new_rot.x
        && lara_item->rot.y == new_rot.y
        && lara_item->rot.z == new_rot.z;
    // clang-format on
}
