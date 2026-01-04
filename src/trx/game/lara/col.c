#include <trx/game/lara/col.h>

#include <trx/debug.h>
#include <trx/game/lara.h>
#include <trx/game/rooms.h>

#define M_MONKEY_CEILING_SNAP 704

static void (*m_CollisionRoutines[LS_NUMBER_OF])(
    ITEM *item, COLL_INFO *coll) = {};

void Lara_Col_Register(
    const LARA_TRX_STATE state,
    void (*const handle_func)(ITEM *item, COLL_INFO *coll))
{
    ASSERT(state >= 0 && state < LS_NUMBER_OF);
    m_CollisionRoutines[state] = handle_func;
}

void Lara_Col_Update(ITEM *const item, COLL_INFO *const coll)
{
    const LARA_TRX_STATE state = LS_U(item->current_anim_state);
    if (state >= 0 && state < LS_NUMBER_OF
        && m_CollisionRoutines[state] != nullptr) {
        m_CollisionRoutines[state](item, coll);
    }
}

void Lara_Col_GetInfo(const ITEM *const item, COLL_INFO *const coll)
{
    LARA_INFO *const lara = Lara_GetLaraInfo();
    coll->facing = lara->move_angle;
    Collide_GetCollisionInfo(
        coll, item->pos.x, item->pos.y, item->pos.z, item->room_num,
        LARA_HEIGHT);
}

void Lara_Col_Shift(COLL_INFO *const coll)
{
    ITEM *const lara_item = Lara_GetItem();
    lara_item->pos.x += coll->shift.x;
    lara_item->pos.y += coll->shift.y;
    lara_item->pos.z += coll->shift.z;
    coll->shift.x = 0;
    coll->shift.y = 0;
    coll->shift.z = 0;
}

void Lara_Col_MonkeySwingSnap(ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector =
        Room_GetSector(item->pos.x, item->pos.y, item->pos.z, &room_num);
    const int32_t ceiling =
        Room_GetCeiling(sector, item->pos.x, item->pos.y, item->pos.z);
    if (ceiling != NO_HEIGHT) {
        item->pos.y = ceiling + M_MONKEY_CEILING_SNAP;
    }
}
