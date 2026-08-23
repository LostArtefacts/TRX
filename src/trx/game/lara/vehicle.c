#include <trx/game/lara/vehicle.h>

#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/lara/skin/common.h>

static const GAME_OBJECT_PAIR m_AnimMap[] = {
    { O_BOAT, O_LARA_BOAT },
    { O_SKIDOO_FAST, O_LARA_SKIDOO },
    { O_QUAD_BIKE, O_LARA_QUAD_BIKE },
    { O_MOUNTED_GUN, O_LARA_MOUNTED_GUN },
    { O_KAYAK, O_LARA_KAYAK },
    { O_UPV, O_LARA_UPV },
    { O_RIB, O_LARA_RIB },
    { O_MINE_CART, O_LARA_MINE_CART },
    { NO_OBJECT, NO_OBJECT },
};

// The weapon a vehicle shoots with. Lara's own is put away while she rides,
// so what her ammunition counter shows comes from here.
static const struct {
    OBJECT_ID object_id;
    LARA_GUN_TYPE gun_type;
} m_GunMap[] = {
    { O_UPV, LGT_HARPOON },
    { NO_OBJECT, LGT_UNARMED },
};

static int16_t m_VehicleItemNum = NO_ITEM;
static OBJECT_ID m_AnimationObject = NO_OBJECT;

static void M_UpdateAnimationObject(void)
{
    if (!Lara_Vehicle_IsMounted()) {
        m_AnimationObject = NO_OBJECT;
        return;
    }

    const ITEM *const vehicle = Lara_Vehicle_GetItem();
    const OBJECT_ID obj_id = Object_GetCognate(vehicle->object_id, m_AnimMap);
    m_AnimationObject = obj_id != NO_OBJECT && Object_Get(obj_id)->loaded
        ? obj_id
        : O_LARA_VEHICLE_ANIM;
}

LARA_GUN_TYPE Lara_Vehicle_GetGunType(void)
{
    if (!Lara_Vehicle_IsMounted()) {
        return LGT_UNARMED;
    }
    const ITEM *const vehicle = Lara_Vehicle_GetItem();
    for (int32_t i = 0; m_GunMap[i].object_id != NO_OBJECT; i++) {
        if (m_GunMap[i].object_id == vehicle->object_id) {
            return m_GunMap[i].gun_type;
        }
    }
    return LGT_UNARMED;
}

bool Lara_Vehicle_IsMounted(void)
{
    return m_VehicleItemNum != NO_ITEM;
}

bool Lara_Vehicle_IsOnType(const OBJECT_ID obj_id)
{
    if (!Lara_Vehicle_IsMounted()) {
        return false;
    }

    const ITEM *const vehicle = Lara_Vehicle_GetItem();
    return vehicle->object_id == obj_id;
}

void Lara_Vehicle_SetIndex(const int16_t item_num)
{
    m_VehicleItemNum = item_num;
    M_UpdateAnimationObject();
}

int16_t Lara_Vehicle_GetIndex(void)
{
    return m_VehicleItemNum;
}

ITEM *Lara_Vehicle_GetItem(void)
{
    return m_VehicleItemNum == NO_ITEM ? nullptr : Item_Get(m_VehicleItemNum);
}

OBJECT_ID Lara_Vehicle_GetAnimationObject(void)
{
    return m_AnimationObject;
}

void Lara_Vehicle_SwitchToAnim(const int16_t anim_idx, const int16_t frame_idx)
{
    ITEM *const lara_item = Lara_GetItem();
    Item_SwitchToObjAnim(lara_item, anim_idx, frame_idx, m_AnimationObject);
}

int16_t Lara_Vehicle_GetRelativeAnim(void)
{
    const ITEM *const lara_item = Lara_GetItem();
    return Item_GetRelativeObjAnim(lara_item, m_AnimationObject);
}

bool Lara_Vehicle_TestAnimEqual(const int16_t anim_idx)
{
    const ITEM *const lara_item = Lara_GetItem();
    return Item_TestObjAnimEqual(lara_item, anim_idx, m_AnimationObject);
}

void Lara_Vehicle_SyncItemAnim(void)
{
    if (!Lara_Vehicle_IsMounted()) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int16_t anim_idx = Lara_Vehicle_GetRelativeAnim();
    const int16_t frame_idx = Item_GetRelativeFrame(lara_item);

    ITEM *const vehicle_item = Lara_Vehicle_GetItem();
    Item_SwitchToAnim(vehicle_item, anim_idx, frame_idx);
}

void Lara_Vehicle_Dismount(void)
{
    if (!Lara_Vehicle_IsMounted()) {
        return;
    }

    ITEM *const lara_item = Lara_GetItem();
    ITEM *const vehicle = Lara_Vehicle_GetItem();
    Item_SwitchToAnim(vehicle, 0, 0);
    Lara_Vehicle_SetIndex(NO_ITEM);

    lara_item->current_anim_state = LS(LS_STOP);
    lara_item->goal_anim_state = LS(LS_STOP);
    Item_SwitchToAnim(lara_item, LA(LA_STAND_STILL), 0);

    lara_item->rot.x = 0;
    lara_item->rot.z = 0;

    const LARA_SKIN_EQUIPMENT *const hand_r_equipment =
        Lara_Skin_GetEquipment(LM_HAND_R);
    if (hand_r_equipment->type == EQUIPMENT_TYPE_EXTRA
        && hand_r_equipment->data == EXTRA_MESH_OAR) {
        Lara_Skin_ClearEquipment(LM_HAND_R);
        Item_SetMeshVisibleMask(lara_item, INT32_MAX, true);
    }
}
