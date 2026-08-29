#include <trx/game/catalog/table.h>
#include <trx/game/items/actions.h>
#include <trx/game/items/const.h>
#include <trx/game/items/manager.h>
#include <trx/game/rooms.h>

typedef void (*M_ACTION_ROUTINE)(ITEM *item);

CATALOG_TABLE_DEFINE(m_Routines, CATALOG_ITEM_ACTIONS, M_ACTION_ROUTINE);
static int16_t m_FXType = 0;
static ITEM_ACTION_INTERCEPTOR m_Interceptor = nullptr;

static void M_RunWithFX(
    const ITEM_ACTION_ID action_id, ITEM *const item, const int16_t fx_type)
{
    m_FXType = fx_type;
    ItemAction_Run(action_id, item);
    m_FXType = 0;
}

// An animation command carries no trigger field, so the timer is 0. The stored
// floor effect reaches here through RunActive with a null item, but a claimed
// number is never stored, so that path never intercepts.
static bool M_InterceptBySlot(
    const ITEM_ACTION_SLOT action_id, ITEM *const item)
{
    const int16_t item_num = item != nullptr ? Item_GetIndex(item) : NO_ITEM;
    return ItemAction_Intercept(action_id, 0, item_num);
}

int16_t ItemAction_GetFXType(void)
{
    return m_FXType;
}

void ItemAction_Register(
    const ITEM_ACTION_ID action, void (*const action_func)(ITEM *item))
{
    *(M_ACTION_ROUTINE *)CatalogTable_Get(&m_Routines, action) = action_func;
}

void ItemAction_Run(const ITEM_ACTION_ID action_id, ITEM *const item)
{
    const M_ACTION_ROUTINE *const routine =
        CatalogTable_Get(&m_Routines, action_id);
    if (routine != nullptr && *routine != nullptr) {
        (*routine)(item);
    }
}

void ItemAction_SetInterceptor(const ITEM_ACTION_INTERCEPTOR interceptor)
{
    m_Interceptor = interceptor;
}

bool ItemAction_Intercept(
    const int32_t effect_num, const int32_t timer, const int16_t item_num)
{
    return m_Interceptor != nullptr
        && m_Interceptor(effect_num, timer, item_num);
}

void ItemAction_RunBySlot(const ITEM_ACTION_SLOT action_id, ITEM *const item)
{
    if (M_InterceptBySlot(action_id, item)) {
        return;
    }
    const ITEM_ACTION_ID trx_id = ItemAction_SlotToID(action_id);
    ItemAction_Run(trx_id, item);
}

void ItemAction_RunWithFXBySlot(
    const ITEM_ACTION_SLOT action_id, ITEM *const item, const int16_t fx_type)
{
    if (M_InterceptBySlot(action_id, item)) {
        return;
    }
    const ITEM_ACTION_ID trx_id = ItemAction_SlotToID(action_id);
    M_RunWithFX(trx_id, item, fx_type);
}

void ItemAction_RunActive(void)
{
    const int32_t flip_effect = Room_GetFlipEffect();
    if (flip_effect != -1) {
        ItemAction_RunBySlot(flip_effect, nullptr);
    }
}
