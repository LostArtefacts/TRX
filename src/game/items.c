#include "game/items.h"

#include "config.h"
#include "game/anim.h"
#include "game/carrier.h"
#include "game/room.h"
#include "game/shell.h"
#include "game/sound.h"
#include "global/const.h"
#include "global/vars.h"
#include "math/math.h"
#include "math/matrix.h"
#include "util.h"

#include <stddef.h>

#define SFX_MODE_BITS(C) (C & 0xC000)
#define SFX_ID_BITS(C) (C & 0x3FFF)
#define SFX_LAND 0x4000
#define SFX_WATER 0x8000

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

ITEM_INFO *g_Items = NULL;
int16_t g_NextItemActive = NO_ITEM;
static int16_t m_NextItemFree = NO_ITEM;
static int16_t m_InterpolatedBounds[6] = { 0 };

void Item_InitialiseArray(int32_t num_items)
{
    g_NextItemActive = NO_ITEM;
    m_NextItemFree = g_LevelItemCount;
    for (int i = g_LevelItemCount; i < num_items - 1; i++) {
        g_Items[i].next_item = i + 1;
    }
    g_Items[num_items - 1].next_item = NO_ITEM;
}

void Item_Control(void)
{
    int16_t item_num = g_NextItemActive;
    while (item_num != NO_ITEM) {
        ITEM_INFO *item = &g_Items[item_num];
        OBJECT_INFO *obj = &g_Objects[item->object_number];
        if (obj->control) {
            obj->control(item_num);
        }
        item_num = item->next_active;
    }

    Carrier_AnimateDrops();
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

    item->hit_points = -1;
    if (item_num < g_LevelItemCount) {
        item->flags |= IF_KILLED_ITEM;
    } else {
        item->next_item = m_NextItemFree;
        m_NextItemFree = item_num;
    }
}

int16_t Item_Create(void)
{
    int16_t item_num = m_NextItemFree;
    if (item_num != NO_ITEM) {
        g_Items[item_num].flags = 0;
        m_NextItemFree = g_Items[item_num].next_item;
    }
    return item_num;
}

void Item_Initialise(int16_t item_num)
{
    ITEM_INFO *item = &g_Items[item_num];
    OBJECT_INFO *object = &g_Objects[item->object_number];

    Item_SwitchToAnim(item, 0, 0);
    item->current_anim_state = g_Anims[item->anim_number].current_anim_state;
    item->goal_anim_state = item->current_anim_state;
    item->required_anim_state = 0;
    item->rot.x = 0;
    item->rot.z = 0;
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
    item->priv = NULL;
    item->carried_item = NULL;

    if (item->flags & IF_NOT_VISIBLE) {
        item->status = IS_INVISIBLE;
        item->flags &= ~IF_NOT_VISIBLE;
    }

    if ((item->flags & IF_CODE_BITS) == IF_CODE_BITS) {
        item->flags &= ~IF_CODE_BITS;
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

bool Item_Teleport(ITEM_INFO *item, int32_t x, int32_t y, int32_t z)
{
    int16_t room_num = Room_GetIndexFromPos(x, y, z);
    if (room_num == NO_ROOM) {
        return false;
    }
    FLOOR_INFO *const floor = Room_GetFloor(x, y, z, &room_num);
    const int16_t height = Room_GetHeight(floor, x, y, z);
    if (height != NO_HEIGHT) {
        item->pos.x = x;
        item->pos.y = y;
        item->pos.z = z;
        item->floor = height;
        if (item->room_number != room_num) {
            const int16_t item_num = item - g_Items;
            Item_NewRoom(item_num, room_num);
        }
        return true;
    }
    return false;
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

int16_t Item_GetHeight(ITEM_INFO *item)
{
    int16_t room_num = item->room_number;
    FLOOR_INFO *floor =
        Room_GetFloor(item->pos.x, item->pos.y, item->pos.z, &room_num);
    int32_t height =
        Room_GetHeight(floor, item->pos.x, item->pos.y, item->pos.z);

    return height;
}

int16_t Item_GetWaterHeight(ITEM_INFO *item)
{
    int16_t height = Room_GetWaterHeight(
        item->pos.x, item->pos.y, item->pos.z, item->room_number);
    if (height != NO_HEIGHT) {
        height -= item->pos.y;
    }

    return height;
}

int16_t Item_Spawn(ITEM_INFO *item, int16_t object_num)
{
    int16_t spawn_num = Item_Create();
    if (spawn_num != NO_ITEM) {
        ITEM_INFO *spawn = &g_Items[spawn_num];
        spawn->object_number = object_num;
        spawn->room_number = item->room_number;
        spawn->pos = item->pos;
        spawn->rot = item->rot;
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

bool Item_IsNearItem(const ITEM_INFO *item, const XYZ_32 *pos, int32_t distance)
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

bool Item_Test3DRange(int32_t x, int32_t y, int32_t z, int32_t range)
{
    return ABS(x) < range && ABS(y) < range && ABS(z) < range
        && (SQUARE(x) + SQUARE(y) + SQUARE(z) < SQUARE(range));
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

    int32_t c = Math_Cos(dst_item->rot.y);
    int32_t s = Math_Sin(dst_item->rot.y);
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
    PHD_ANGLE xrotrel = src_item->rot.x - dst_item->rot.x;
    PHD_ANGLE yrotrel = src_item->rot.y - dst_item->rot.y;
    PHD_ANGLE zrotrel = src_item->rot.z - dst_item->rot.z;
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
    Matrix_RotYXZ(dst_item->rot.y, dst_item->rot.x, dst_item->rot.z);
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

void Item_AlignPosition(ITEM_INFO *src_item, ITEM_INFO *dst_item, XYZ_32 *vec)
{
    src_item->rot.x = dst_item->rot.x;
    src_item->rot.y = dst_item->rot.y;
    src_item->rot.z = dst_item->rot.z;

    Matrix_PushUnit();
    Matrix_RotYXZ(dst_item->rot.y, dst_item->rot.x, dst_item->rot.z);
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
    ITEM_INFO *item, const ITEM_INFO *ref_item, const XYZ_32 *vec,
    int32_t velocity)
{
    const XYZ_32 *ref_pos = &ref_item->pos;

    Matrix_PushUnit();
    Matrix_RotYXZ(ref_item->rot.y, ref_item->rot.x, ref_item->rot.z);

    MATRIX *mptr = g_MatrixPtr;
    const XYZ_32 dst_pos = {
        .x = ref_pos->x
            + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
               >> W2V_SHIFT),
        .y = ref_pos->y
            + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
               >> W2V_SHIFT),
        .z = ref_pos->z
            + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
               >> W2V_SHIFT),
    };

    const XYZ_16 dst_rot = ref_item->rot;

    Matrix_Pop();

    {
        const int32_t dx = dst_pos.x - item->pos.x;
        const int32_t dy = dst_pos.y - item->pos.y;
        const int32_t dz = dst_pos.z - item->pos.z;
        const int32_t dist = Math_Sqrt(SQUARE(dx) + SQUARE(dy) + SQUARE(dz));
        if (velocity >= dist) {
            item->pos.x = dst_pos.x;
            item->pos.y = dst_pos.y;
            item->pos.z = dst_pos.z;
        } else {
            item->pos.x += (dx * velocity) / dist;
            item->pos.y += (dy * velocity) / dist;
            item->pos.z += (dz * velocity) / dist;
        }
    }

    if (item == g_LaraItem && g_Config.walk_to_items
        && !g_Lara.interact_target.is_moving) {
        if (g_Lara.water_status != LWS_UNDERWATER) {
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

            const int32_t dx = item->pos.x - dst_pos.x;
            const int32_t dz = item->pos.z - dst_pos.z;
            const int32_t angle = (PHD_ONE - Math_Atan(dx, dz)) % PHD_ONE;
            const uint32_t src_quadrant = (uint32_t)(angle + PHD_45) / PHD_90;
            const uint32_t dst_quadrant =
                (uint32_t)(dst_rot.y + PHD_45) / PHD_90;
            const DIRECTION quadrant = (src_quadrant - dst_quadrant) % 4;

            Item_SwitchToAnim(item, step_to_anim_num[quadrant], 0);
            item->goal_anim_state = step_to_anim_state[quadrant];
            item->current_anim_state = step_to_anim_state[quadrant];

            g_Lara.gun_status = LGS_HANDS_BUSY;
        }

        g_Lara.interact_target.is_moving = true;
        g_Lara.interact_target.move_count = 0;
    }

    int16_t rotation = MOVE_ANG;
    ITEM_ADJUST_ROT(item->rot.x, dst_rot.x, rotation);
    ITEM_ADJUST_ROT(item->rot.y, dst_rot.y, rotation);
    ITEM_ADJUST_ROT(item->rot.z, dst_rot.z, rotation);

    // clang-format off
    return item->pos.x == dst_pos.x
        && item->pos.y == dst_pos.y
        && item->pos.z == dst_pos.z
        && item->rot.x == dst_rot.x
        && item->rot.y == dst_rot.y
        && item->rot.z == dst_rot.z;
    // clang-format on
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
    int32_t c = Math_Cos(item->rot.y);
    int32_t s = Math_Sin(item->rot.y);
    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}

bool Item_TestAnimEqual(ITEM_INFO *item, int16_t anim_index)
{
    return item->anim_number
        == g_Objects[item->object_number].anim_index + anim_index;
}

void Item_SwitchToAnim(ITEM_INFO *item, int16_t anim_index, int16_t frame)
{
    Item_SwitchToObjAnim(item, anim_index, frame, item->object_number);
}

void Item_SwitchToObjAnim(
    ITEM_INFO *item, int16_t anim_index, int16_t frame,
    GAME_OBJECT_ID object_number)
{
    item->anim_number = g_Objects[object_number].anim_index + anim_index;
    if (frame < 0) {
        item->frame_number = g_Anims[item->anim_number].frame_end + frame + 1;
    } else {
        item->frame_number = g_Anims[item->anim_number].frame_base + frame;
    }
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
                Item_PlayAnimSFX(
                    item, command, g_RoomInfo[item->room_number].flags);
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

    item->pos.x += (Math_Sin(item->rot.y) * item->speed) >> W2V_SHIFT;
    item->pos.z += (Math_Cos(item->rot.y) * item->speed) >> W2V_SHIFT;
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
                if (Item_TestFrameRange(
                        item,
                        range->start_frame
                            - g_Anims[item->anim_number].frame_base,
                        range->end_frame
                            - g_Anims[item->anim_number].frame_base)) {
                    item->anim_number = range->link_anim_num;
                    item->frame_number = range->link_frame_num;
                    return true;
                }
            }
        }
    }

    return false;
}

void Item_PlayAnimSFX(ITEM_INFO *item, int16_t *command, uint16_t flags)
{
    if (item->frame_number != command[0]) {
        return;
    }

    uint16_t mode = SFX_MODE_BITS(command[1]);
    if (mode) {
        int16_t height = Item_GetWaterHeight(item);
        if ((mode == SFX_WATER && (height >= 0 || height == NO_HEIGHT))
            || (mode == SFX_LAND && height < 0 && height != NO_HEIGHT)) {
            return;
        }
    }

    Sound_Effect(SFX_ID_BITS(command[1]), &item->pos, flags);
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

int16_t *Item_GetBestFrame(const ITEM_INFO *item)
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

int16_t *Item_GetBoundsAccurate(const ITEM_INFO *item)
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

int32_t Item_GetFrames(const ITEM_INFO *item, int16_t *frmptr[], int32_t *rate)
{
    const ANIM_STRUCT *anim = &g_Anims[item->anim_number];
    const int32_t cur_frame_num = item->frame_number - anim->frame_base;
    const int32_t last_frame_num = anim->frame_end - anim->frame_base;
    const int32_t key_frame_span = anim->interpolation;
    const int32_t first_key_frame_num = cur_frame_num / key_frame_span;
    const int32_t second_key_frame_num = first_key_frame_num + 1;
    const int32_t frame_size = g_Objects[item->object_number].nmeshes * 2 + 10;

    frmptr[0] = anim->frame_ptr + first_key_frame_num * frame_size;
    frmptr[1] = anim->frame_ptr + second_key_frame_num * frame_size;

    const int32_t key_frame_shift = cur_frame_num % key_frame_span;
    const int32_t numerator = key_frame_shift;
    int32_t denominator = key_frame_span;
    if (numerator && second_key_frame_num > anim->frame_end) {
        denominator = anim->frame_end + key_frame_span - second_key_frame_num;
    }

    *rate = denominator;
    return numerator;
}

void Item_TakeDamage(ITEM_INFO *item, int16_t damage, bool hit_status)
{
    if (item->hit_points == DONT_TARGET) {
        return;
    }

    item->hit_points -= damage;
    CLAMPL(item->hit_points, 0);

    if (hit_status) {
        item->hit_status = 1;
    }
}

bool Item_TestFrameEqual(ITEM_INFO *item, int16_t frame)
{
    return Anim_TestAbsFrameEqual(
        item->frame_number, g_Anims[item->anim_number].frame_base + frame);
}

bool Item_TestFrameRange(ITEM_INFO *item, int16_t start, int16_t end)
{
    return Anim_TestAbsFrameRange(
        item->frame_number, g_Anims[item->anim_number].frame_base + start,
        g_Anims[item->anim_number].frame_base + end);
}
