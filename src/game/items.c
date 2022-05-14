#include "game/items.h"

#include "game/draw.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"
#include "util.h"

#include <stddef.h>

#define ITEM_ADJUST_ROT(source, target, rot)                                   \
    do {                                                                       \
        if ((int16_t)(target - source) > rot) {                                \
            source += rot;                                                     \
        } else if ((int16_t)(target - source) < -rot) {                        \
            source -= rot;                                                     \
        } else {                                                               \
            source = target;                                                   \
        }                                                                      \
    } while (0)

static int16_t m_InterpolatedBounds[6] = { 0 };

static bool Item_Move3DPosTo3DPos(
    PHD_3DPOS *src_pos, PHD_3DPOS *dst_pos, int32_t velocity, int16_t rotation);

static bool Item_Move3DPosTo3DPos(
    PHD_3DPOS *src_pos, PHD_3DPOS *dst_pos, int32_t velocity, int16_t rotation)
{
    int32_t x = dst_pos->x - src_pos->x;
    int32_t y = dst_pos->y - src_pos->y;
    int32_t z = dst_pos->z - src_pos->z;
    int32_t dist = Math_Sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
    if (velocity >= dist) {
        src_pos->x = dst_pos->x;
        src_pos->y = dst_pos->y;
        src_pos->z = dst_pos->z;
    } else {
        src_pos->x += (x * velocity) / dist;
        src_pos->y += (y * velocity) / dist;
        src_pos->z += (z * velocity) / dist;
    }

    ITEM_ADJUST_ROT(src_pos->x_rot, dst_pos->x_rot, rotation);
    ITEM_ADJUST_ROT(src_pos->y_rot, dst_pos->y_rot, rotation);
    ITEM_ADJUST_ROT(src_pos->z_rot, dst_pos->z_rot, rotation);

    return src_pos->x == dst_pos->x && src_pos->y == dst_pos->y
        && src_pos->z == dst_pos->z && src_pos->x_rot == dst_pos->x_rot
        && src_pos->y_rot == dst_pos->y_rot && src_pos->z_rot == dst_pos->z_rot;
}

void Item_InitialiseArray(int32_t num_items)
{
    g_NextItemActive = NO_ITEM;
    g_NextItemFree = g_LevelItemCount;
    for (int i = g_LevelItemCount; i < num_items - 1; i++) {
        g_Items[i].next_item = i + 1;
    }
    g_Items[num_items - 1].next_item = NO_ITEM;
}

void Item_Kill(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    ROOM_INFO *r = &g_RoomInfo[item->room_number];

    int16_t linknum = g_NextItemActive;
    if (linknum == item_num) {
        g_NextItemActive = item->next_active;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_active) {
            if (g_Items[linknum].next_active == item_num) {
                g_Items[linknum].next_active = item->next_active;
                break;
            }
        }
    }

    linknum = r->item_number;
    if (linknum == item_num) {
        r->item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_item) {
            if (g_Items[linknum].next_item == item_num) {
                g_Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }

    if (item == g_Lara.target) {
        g_Lara.target = NULL;
    }

    if (item_num < g_LevelItemCount) {
        item->flags |= IF_KILLED_ITEM;
    } else {
        item->next_item = g_NextItemFree;
        g_NextItemFree = item_num;
    }
}

int16_t Item_Create(void)
{
    int16_t item_num = g_NextItemFree;
    if (item_num != NO_ITEM) {
        g_Items[item_num].flags = 0;
        g_NextItemFree = g_Items[item_num].next_item;
    }
    return item_num;
}

void Item_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    OBJECT_INFO *object = &g_Objects[item->object_number];

    item->anim_number = object->anim_index;
    item->frame_number = g_Anims[item->anim_number].frame_base;
    item->current_anim_state = g_Anims[item->anim_number].current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->required_anim_state = 0;
    item->pos.x_rot = 0;
    item->pos.z_rot = 0;
    item->speed = 0;
    item->fall_speed = 0;
    item->status = IS_NOT_ACTIVE;
    item->active = 0;
    item->gravity_status = 0;
    item->hit_status = 0;
    item->looked_at = 0;
    item->collidable = 1;
    item->hit_points = object->hit_points;
    item->timer = 0;
    item->mesh_bits = -1;
    item->touch_bits = 0;
    item->data = NULL;

    if (item->flags & IF_NOT_VISIBLE) {
        item->status = IS_INVISIBLE;
        item->flags -= IF_NOT_VISIBLE;
    }

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        item->flags -= IF_CODE_BITS;
        item->flags |= IF_REVERSE;
        Item_AddActive(item_num);
        item->status = IS_ACTIVE;
    }

    ROOM_INFO *r = &g_RoomInfo[item->room_number];
    item->next_item = r->item_number;
    r->item_number = item_num;
    int32_t x_floor = (item->pos.z - r->z) >> WALL_SHIFT;
    int32_t y_floor = (item->pos.x - r->x) >> WALL_SHIFT;
    FLOOR_INFO *floor = &r->floor[x_floor + y_floor * r->x_size];
    item->floor = floor->floor << 8;

    if (g_GameInfo.bonus_flag & GBF_NGPLUS) {
        item->hit_points *= 2;
    }
    if (object->initialise) {
        object->initialise(item_num);
    }
}

void Item_RemoveActive(int16_t item_num)
{
    if (!g_Items[item_num].active) {
        Shell_ExitSystem("Item already deactive");
    }

    g_Items[item_num].active = 0;

    int16_t linknum = g_NextItemActive;
    if (linknum == item_num) {
        g_NextItemActive = g_Items[item_num].next_active;
        return;
    }

    for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_active) {
        if (g_Items[linknum].next_active == item_num) {
            g_Items[linknum].next_active = g_Items[item_num].next_active;
            break;
        }
    }
}

void Item_RemoveDrawn(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    ROOM_INFO *r = &g_RoomInfo[item->room_number];

    int16_t linknum = r->item_number;
    if (linknum == item_num) {
        r->item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_item) {
            if (g_Items[linknum].next_item == item_num) {
                g_Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }
}

void Item_AddActive(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];

    if (!g_Objects[item->object_number].control) {
        item->status = IS_NOT_ACTIVE;
        return;
    }

    if (item->active) {
        return;
    }

    item->active = 1;
    item->next_active = g_NextItemActive;
    g_NextItemActive = item_num;
}

void Item_NewRoom(int16_t item_num, int16_t room_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    ROOM_INFO *r = &g_RoomInfo[item->room_number];

    int16_t linknum = r->item_number;
    if (linknum == item_num) {
        r->item_number = item->next_item;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Items[linknum].next_item) {
            if (g_Items[linknum].next_item == item_num) {
                g_Items[linknum].next_item = item->next_item;
                break;
            }
        }
    }

    r = &g_RoomInfo[room_num];
    item->room_number = room_num;
    item->next_item = r->item_number;
    r->item_number = item_num;
}

void Item_UpdateRoom(ITEM_INFO *item, int32_t height)
{
    int32_t x = item->pos.x;
    int32_t y = item->pos.y + height;
    int32_t z = item->pos.z;
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor = Room_GetFloor(x, y, z, &room_num);
    item->floor = Room_GetHeight(floor, x, y, z);
    if (item->room_number != room_num) {
        Item_NewRoom(g_Lara.item_number, room_num);
    }
}

int16_t Item_Spawn(ITEM_INFO *item, int16_t object_num)
{
    int16_t spawn_num = Item_Create();
    if (spawn_num != NO_ITEM) {
        ITEM_INFO *spawn = &g_Items[spawn_num];
        spawn->object_number = object_num;
        spawn->room_number = item->room_number;
        spawn->pos = item->pos;
        Item_Initialise(spawn_num);
        spawn->status = IS_NOT_ACTIVE;
        spawn->shade = 4096;
    }
    return spawn_num;
}

int32_t Item_GlobalReplace(int32_t src_object_num, int32_t dst_object_num)
{
    int32_t changed = 0;
    for (int i = 0; i < g_RoomCount; i++) {
        ROOM_INFO *r = &g_RoomInfo[i];
        for (int16_t item_num = r->item_number; item_num != NO_ITEM;
             item_num = g_Items[item_num].next_item) {
            if (g_Items[item_num].object_number == src_object_num) {
                g_Items[item_num].object_number = dst_object_num;
                changed++;
            }
        }
    }
    return changed;
}

bool Item_IsNearItem(ITEM_INFO *item, PHD_3DPOS *pos, int32_t distance)
{
    int32_t x = pos->x - item->pos.x;
    int32_t y = pos->y - item->pos.y;
    int32_t z = pos->z - item->pos.z;

    if (x >= -distance && x <= distance && z >= -distance && z <= distance
        && y >= -WALL_L * 3 && y <= WALL_L * 3
        && SQUARE(x) + SQUARE(z) <= SQUARE(distance)) {
        int16_t *bounds = Item_GetBoundsAccurate(item);
        if (y >= bounds[FRAME_BOUND_MIN_Y]
            && y <= bounds[FRAME_BOUND_MAX_Y] + 100) {
            return true;
        }
    }

    return false;
}

bool Item_TestBoundsCollide(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, int32_t radius)
{
    int16_t *src_bounds = Item_GetBestFrame(src_item);
    int16_t *dst_bounds = Item_GetBestFrame(dst_item);
    if (dst_item->pos.y + dst_bounds[FRAME_BOUND_MAX_Y]
            <= src_item->pos.y + src_bounds[FRAME_BOUND_MIN_Y]
        || dst_item->pos.y + dst_bounds[FRAME_BOUND_MIN_Y]
            >= src_item->pos.y + src_bounds[FRAME_BOUND_MAX_Y]) {
        return false;
    }

    int32_t c = Math_Cos(dst_item->pos.y_rot);
    int32_t s = Math_Sin(dst_item->pos.y_rot);
    int32_t x = src_item->pos.x - dst_item->pos.x;
    int32_t z = src_item->pos.z - dst_item->pos.z;
    int32_t rx = (c * x - s * z) >> W2V_SHIFT;
    int32_t rz = (c * z + s * x) >> W2V_SHIFT;
    int32_t minx = dst_bounds[FRAME_BOUND_MIN_X] - radius;
    int32_t maxx = dst_bounds[FRAME_BOUND_MAX_X] + radius;
    int32_t minz = dst_bounds[FRAME_BOUND_MIN_Z] - radius;
    int32_t maxz = dst_bounds[FRAME_BOUND_MAX_Z] + radius;
    return rx >= minx && rx <= maxx && rz >= minz && rz <= maxz;
}

bool Item_TestPosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, int16_t *bounds)
{
    PHD_ANGLE xrotrel = src_item->pos.x_rot - dst_item->pos.x_rot;
    PHD_ANGLE yrotrel = src_item->pos.y_rot - dst_item->pos.y_rot;
    PHD_ANGLE zrotrel = src_item->pos.z_rot - dst_item->pos.z_rot;
    if (xrotrel < bounds[6] || xrotrel > bounds[7]) {
        return false;
    }
    if (yrotrel < bounds[8] || yrotrel > bounds[9]) {
        return false;
    }
    if (zrotrel < bounds[10] || zrotrel > bounds[11]) {
        return false;
    }

    int32_t x = src_item->pos.x - dst_item->pos.x;
    int32_t y = src_item->pos.y - dst_item->pos.y;
    int32_t z = src_item->pos.z - dst_item->pos.z;
    Matrix_PushUnit();
    Matrix_RotYXZ(
        dst_item->pos.y_rot, dst_item->pos.x_rot, dst_item->pos.z_rot);
    MATRIX *mptr = g_MatrixPtr;
    int32_t rx = (mptr->_00 * x + mptr->_10 * y + mptr->_20 * z) >> W2V_SHIFT;
    int32_t ry = (mptr->_01 * x + mptr->_11 * y + mptr->_21 * z) >> W2V_SHIFT;
    int32_t rz = (mptr->_02 * x + mptr->_12 * y + mptr->_22 * z) >> W2V_SHIFT;
    Matrix_Pop();
    if (rx < bounds[0] || rx > bounds[1]) {
        return false;
    }
    if (ry < bounds[2] || ry > bounds[3]) {
        return false;
    }
    if (rz < bounds[4] || rz > bounds[5]) {
        return false;
    }

    return true;
}

void Item_AlignPosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, PHD_VECTOR *vec)
{
    src_item->pos.x_rot = dst_item->pos.x_rot;
    src_item->pos.y_rot = dst_item->pos.y_rot;
    src_item->pos.z_rot = dst_item->pos.z_rot;

    Matrix_PushUnit();
    Matrix_RotYXZ(
        dst_item->pos.y_rot, dst_item->pos.x_rot, dst_item->pos.z_rot);
    MATRIX *mptr = g_MatrixPtr;
    src_item->pos.x = dst_item->pos.x
        + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
           >> W2V_SHIFT);
    src_item->pos.y = dst_item->pos.y
        + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
           >> W2V_SHIFT);
    src_item->pos.z = dst_item->pos.z
        + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
           >> W2V_SHIFT);
    Matrix_Pop();
}

bool Item_MovePosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, PHD_VECTOR *vec, int32_t velocity)
{
    PHD_3DPOS dst_pos;
    dst_pos.x_rot = dst_item->pos.x_rot;
    dst_pos.y_rot = dst_item->pos.y_rot;
    dst_pos.z_rot = dst_item->pos.z_rot;
    Matrix_PushUnit();
    Matrix_RotYXZ(
        dst_item->pos.y_rot, dst_item->pos.x_rot, dst_item->pos.z_rot);
    MATRIX *mptr = g_MatrixPtr;
    dst_pos.x = dst_item->pos.x
        + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
           >> W2V_SHIFT);
    dst_pos.y = dst_item->pos.y
        + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
           >> W2V_SHIFT);
    dst_pos.z = dst_item->pos.z
        + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
           >> W2V_SHIFT);
    Matrix_Pop();

    return Item_Move3DPosTo3DPos(&src_item->pos, &dst_pos, velocity, MOVE_ANG);
}

void Item_ShiftCol(ITEM_INFO *item, COLL_INFO *coll)
{
    item->pos.x += coll->shift.x;
    item->pos.y += coll->shift.y;
    item->pos.z += coll->shift.z;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
}

void Item_Translate(ITEM_INFO *item, int32_t x, int32_t y, int32_t z)
{
    int32_t c = Math_Cos(item->pos.y_rot);
    int32_t s = Math_Sin(item->pos.y_rot);
    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}

void Item_Animate(ITEM_INFO *item)
{
    item->touch_bits = 0;
    item->hit_status = 0;

    ANIM_STRUCT *anim = &g_Anims[item->anim_number];

    item->frame_number++;

    if (anim->number_changes > 0) {
        if (Item_GetAnimChange(item, anim)) {
            anim = &g_Anims[item->anim_number];
            item->current_anim_state = anim->current_anim_state;

            if (item->required_anim_state == item->current_anim_state) {
                item->required_anim_state = 0;
            }
        }
    }

    if (item->frame_number > anim->frame_end) {
        if (anim->number_commands > 0) {
            int16_t *command = &g_AnimCommands[anim->command_index];
            for (int i = 0; i < anim->number_commands; i++) {
                switch (*command++) {
                case AC_MOVE_ORIGIN:
                    Item_Translate(item, command[0], command[1], command[2]);
                    command += 3;
                    break;

                case AC_JUMP_VELOCITY:
                    item->fall_speed = command[0];
                    item->speed = command[1];
                    item->gravity_status = 1;
                    command += 2;
                    break;

                case AC_DEACTIVATE:
                    item->status = IS_DEACTIVATED;
                    break;

                case AC_SOUND_FX:
                case AC_EFFECT:
                    command += 2;
                    break;
                }
            }
        }

        item->anim_number = anim->jump_anim_num;
        item->frame_number = anim->jump_frame_num;

        anim = &g_Anims[item->anim_number];
        item->current_anim_state = anim->current_anim_state;
        item->goal_anim_state = item->current_anim_state;
        if (item->required_anim_state == item->current_anim_state) {
            item->required_anim_state = 0;
        }
    }

    if (anim->number_commands > 0) {
        int16_t *command = &g_AnimCommands[anim->command_index];
        for (int i = 0; i < anim->number_commands; i++) {
            switch (*command++) {
            case AC_MOVE_ORIGIN:
                command += 3;
                break;

            case AC_JUMP_VELOCITY:
                command += 2;
                break;

            case AC_SOUND_FX:
                if (item->frame_number == command[0]) {
                    Sound_Effect(
                        command[1], &item->pos,
                        g_RoomInfo[item->room_number].flags);
                }
                command += 2;
                break;

            case AC_EFFECT:
                if (item->frame_number == command[0]) {
                    g_EffectRoutines[command[1]](item);
                }
                command += 2;
                break;
            }
        }
    }

    if (!item->gravity_status) {
        int32_t speed = anim->velocity;
        if (anim->acceleration) {
            speed +=
                anim->acceleration * (item->frame_number - anim->frame_base);
        }
        item->speed = speed >> 16;
    } else {
        item->fall_speed += (item->fall_speed < FASTFALL_SPEED) ? GRAVITY : 1;
        item->pos.y += item->fall_speed;
    }

    item->pos.x += (Math_Sin(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
    item->pos.z += (Math_Cos(item->pos.y_rot) * item->speed) >> W2V_SHIFT;
}

bool Item_GetAnimChange(ITEM_INFO *item, ANIM_STRUCT *anim)
{
    if (item->current_anim_state == item->goal_anim_state) {
        return false;
    }

    ANIM_CHANGE_STRUCT *change = &g_AnimChanges[anim->change_index];
    for (int i = 0; i < anim->number_changes; i++, change++) {
        if (change->goal_anim_state == item->goal_anim_state) {
            ANIM_RANGE_STRUCT *range = &g_AnimRanges[change->range_index];
            for (int j = 0; j < change->number_ranges; j++, range++) {
                if (item->frame_number >= range->start_frame
                    && item->frame_number <= range->end_frame) {
                    item->anim_number = range->link_anim_num;
                    item->frame_number = range->link_frame_num;
                    return true;
                }
            }
        }
    }

    return false;
}

bool Item_IsTriggerActive(ITEM_INFO *item)
{
    bool ok = !(item->flags & IF_REVERSE);

    if ((item->flags & IF_CODE_BITS) != IF_CODE_BITS) {
        return !ok;
    }

    if (!item->timer) {
        return ok;
    }

    if (item->timer == -1) {
        return !ok;
    }

    item->timer--;

    if (!item->timer) {
        item->timer = -1;
    }

    return ok;
}

int16_t *Item_GetBestFrame(ITEM_INFO *item)
{
    int16_t *frmptr[2];
    int32_t rate;
    int32_t frac = Item_GetFrames(item, frmptr, &rate);
    if (frac <= rate / 2) {
        return frmptr[0];
    } else {
        return frmptr[1];
    }
}

int16_t *Item_GetBoundsAccurate(ITEM_INFO *item)
{
    int32_t rate;
    int16_t *frmptr[2];

    int32_t frac = Item_GetFrames(item, frmptr, &rate);
    if (!frac) {
        return frmptr[0];
    }

    for (int i = 0; i < 6; i++) {
        int16_t a = frmptr[0][i];
        int16_t b = frmptr[1][i];
        m_InterpolatedBounds[i] = a + (((b - a) * frac) / rate);
    }
    return m_InterpolatedBounds;
}

int32_t Item_GetFrames(ITEM_INFO *item, int16_t *frmptr[], int32_t *rate)
{
    ANIM_STRUCT *anim = &g_Anims[item->anim_number];
    frmptr[0] = anim->frame_ptr;
    frmptr[1] = anim->frame_ptr;

    *rate = anim->interpolation;

    int32_t frm = item->frame_number - anim->frame_base;
    int32_t first = frm / anim->interpolation;
    int32_t frame_size = g_Objects[item->object_number].nmeshes * 2 + 10;

    frmptr[0] += first * frame_size;
    frmptr[1] = frmptr[0] + frame_size;

    int32_t interp = frm % anim->interpolation;
    if (!interp) {
        return 0;
    }

    int32_t second = anim->interpolation * (first + 1);
    if (second > anim->frame_end) {
        *rate = anim->frame_end + anim->interpolation - second;
    }

    return interp;
}
