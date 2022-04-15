#include "game/items.h"

#include "3dsystem/matrix.h"
#include "3dsystem/phd_math.h"
#include "game/collide.h"
#include "game/draw.h"
#include "game/room.h"
#include "game/shell.h"
#include "global/const.h"
#include "global/vars.h"

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

static bool Item_Move3DPosTo3DPos(
    PHD_3DPOS *src_pos, PHD_3DPOS *dst_pos, int32_t velocity, int16_t rotation);

static bool Item_Move3DPosTo3DPos(
    PHD_3DPOS *src_pos, PHD_3DPOS *dst_pos, int32_t velocity, int16_t rotation)
{
    int32_t x = dst_pos->x - src_pos->x;
    int32_t y = dst_pos->y - src_pos->y;
    int32_t z = dst_pos->z - src_pos->z;
    int32_t dist = phd_sqrt(SQUARE(x) + SQUARE(y) + SQUARE(z));
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

void InitialiseItemArray(int32_t num_items)
{
    g_NextItemActive = NO_ITEM;
    g_NextItemFree = g_LevelItemCount;
    for (int i = g_LevelItemCount; i < num_items - 1; i++) {
        g_Items[i].next_item = i + 1;
    }
    g_Items[num_items - 1].next_item = NO_ITEM;
}

void KillItem(int16_t item_num)
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

int16_t CreateItem(void)
{
    int16_t item_num = g_NextItemFree;
    if (item_num != NO_ITEM) {
        g_Items[item_num].flags = 0;
        g_NextItemFree = g_Items[item_num].next_item;
    }
    return item_num;
}

void InitialiseItem(int16_t item_num)
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
        AddActiveItem(item_num);
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

void RemoveActiveItem(int16_t item_num)
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

void RemoveDrawnItem(int16_t item_num)
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

void AddActiveItem(int16_t item_num)
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

void ItemNewRoom(int16_t item_num, int16_t room_num)
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
        ItemNewRoom(g_Lara.item_number, room_num);
    }
}

int16_t SpawnItem(ITEM_INFO *item, int16_t object_num)
{
    int16_t spawn_num = CreateItem();
    if (spawn_num != NO_ITEM) {
        ITEM_INFO *spawn = &g_Items[spawn_num];
        spawn->object_number = object_num;
        spawn->room_number = item->room_number;
        spawn->pos = item->pos;
        InitialiseItem(spawn_num);
        spawn->status = IS_NOT_ACTIVE;
        spawn->shade = 4096;
    }
    return spawn_num;
}

int32_t GlobalItemReplace(int32_t src_object_num, int32_t dst_object_num)
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

void InitialiseFXArray(void)
{
    g_NextFxActive = NO_ITEM;
    g_NextFxFree = 0;
    for (int i = 0; i < NUM_EFFECTS - 1; i++) {
        g_Effects[i].next_fx = i + 1;
    }
    g_Effects[NUM_EFFECTS - 1].next_fx = NO_ITEM;
}

int16_t CreateEffect(int16_t room_num)
{
    int16_t fx_num = g_NextFxFree;
    if (fx_num == NO_ITEM) {
        return fx_num;
    }

    FX_INFO *fx = &g_Effects[fx_num];
    g_NextFxFree = fx->next_fx;

    ROOM_INFO *r = &g_RoomInfo[room_num];
    fx->room_number = room_num;
    fx->next_fx = r->fx_number;
    r->fx_number = fx_num;

    fx->next_active = g_NextFxActive;
    g_NextFxActive = fx_num;

    return fx_num;
}

void KillEffect(int16_t fx_num)
{
    FX_INFO *fx = &g_Effects[fx_num];

    int16_t linknum = g_NextFxActive;
    if (linknum == fx_num) {
        g_NextFxActive = fx->next_active;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_active) {
            if (g_Effects[linknum].next_active == fx_num) {
                g_Effects[linknum].next_active = fx->next_active;
                break;
            }
        }
    }

    ROOM_INFO *r = &g_RoomInfo[fx->room_number];
    linknum = r->fx_number;
    if (linknum == fx_num) {
        r->fx_number = fx->next_fx;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_fx) {
            if (g_Effects[linknum].next_fx == fx_num) {
                g_Effects[linknum].next_fx = fx->next_fx;
                break;
            }
        }
    }

    fx->next_fx = g_NextFxFree;
    g_NextFxFree = fx_num;
}

void EffectNewRoom(int16_t fx_num, int16_t room_num)
{
    FX_INFO *fx = &g_Effects[fx_num];
    ROOM_INFO *r = &g_RoomInfo[fx->room_number];

    int16_t linknum = r->fx_number;
    if (linknum == fx_num) {
        r->fx_number = fx->next_fx;
    } else {
        for (; linknum != NO_ITEM; linknum = g_Effects[linknum].next_fx) {
            if (g_Effects[linknum].next_fx == fx_num) {
                g_Effects[linknum].next_fx = fx->next_fx;
                break;
            }
        }
    }

    r = &g_RoomInfo[room_num];
    fx->room_number = room_num;
    fx->next_fx = r->fx_number;
    r->fx_number = fx_num;
}

bool Item_IsNearItem(ITEM_INFO *item, PHD_3DPOS *pos, int32_t distance)
{
    int32_t x = pos->x - item->pos.x;
    int32_t y = pos->y - item->pos.y;
    int32_t z = pos->z - item->pos.z;

    if (x >= -distance && x <= distance && z >= -distance && z <= distance
        && y >= -WALL_L * 3 && y <= WALL_L * 3
        && SQUARE(x) + SQUARE(z) <= SQUARE(distance)) {
        int16_t *bounds = GetBoundsAccurate(item);
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
    int16_t *src_bounds = GetBestFrame(src_item);
    int16_t *dst_bounds = GetBestFrame(dst_item);
    if (dst_item->pos.y + dst_bounds[FRAME_BOUND_MAX_Y]
            <= src_item->pos.y + src_bounds[FRAME_BOUND_MIN_Y]
        || dst_item->pos.y + dst_bounds[FRAME_BOUND_MIN_Y]
            >= src_item->pos.y + src_bounds[FRAME_BOUND_MAX_Y]) {
        return false;
    }

    int32_t c = phd_cos(dst_item->pos.y_rot);
    int32_t s = phd_sin(dst_item->pos.y_rot);
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
    phd_PushUnitMatrix();
    phd_RotYXZ(dst_item->pos.y_rot, dst_item->pos.x_rot, dst_item->pos.z_rot);
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    int32_t rx = (mptr->_00 * x + mptr->_10 * y + mptr->_20 * z) >> W2V_SHIFT;
    int32_t ry = (mptr->_01 * x + mptr->_11 * y + mptr->_21 * z) >> W2V_SHIFT;
    int32_t rz = (mptr->_02 * x + mptr->_12 * y + mptr->_22 * z) >> W2V_SHIFT;
    phd_PopMatrix();
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

    phd_PushUnitMatrix();
    phd_RotYXZ(dst_item->pos.y_rot, dst_item->pos.x_rot, dst_item->pos.z_rot);
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    src_item->pos.x = dst_item->pos.x
        + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
           >> W2V_SHIFT);
    src_item->pos.y = dst_item->pos.y
        + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
           >> W2V_SHIFT);
    src_item->pos.z = dst_item->pos.z
        + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
           >> W2V_SHIFT);
    phd_PopMatrix();
}

bool Item_MovePosition(
    ITEM_INFO *src_item, ITEM_INFO *dst_item, PHD_VECTOR *vec, int32_t velocity)
{
    PHD_3DPOS dst_pos;
    dst_pos.x_rot = dst_item->pos.x_rot;
    dst_pos.y_rot = dst_item->pos.y_rot;
    dst_pos.z_rot = dst_item->pos.z_rot;
    phd_PushUnitMatrix();
    phd_RotYXZ(dst_item->pos.y_rot, dst_item->pos.x_rot, dst_item->pos.z_rot);
    PHD_MATRIX *mptr = g_PhdMatrixPtr;
    dst_pos.x = dst_item->pos.x
        + ((mptr->_00 * vec->x + mptr->_01 * vec->y + mptr->_02 * vec->z)
           >> W2V_SHIFT);
    dst_pos.y = dst_item->pos.y
        + ((mptr->_10 * vec->x + mptr->_11 * vec->y + mptr->_12 * vec->z)
           >> W2V_SHIFT);
    dst_pos.z = dst_item->pos.z
        + ((mptr->_20 * vec->x + mptr->_21 * vec->y + mptr->_22 * vec->z)
           >> W2V_SHIFT);
    phd_PopMatrix();

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
    int32_t c = phd_cos(item->pos.y_rot);
    int32_t s = phd_sin(item->pos.y_rot);
    item->pos.x += (c * x + s * z) >> W2V_SHIFT;
    item->pos.y += y;
    item->pos.z += (c * z - s * x) >> W2V_SHIFT;
}
