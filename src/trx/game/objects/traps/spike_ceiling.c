#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/traps/common.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

#define M_DEFAULT_DAMAGE 20
#define M_BLOOD_SPEED 1
#define M_DEFAULT_SPEED 5

typedef struct {
    int32_t speed;
} M_PRIV;

static int32_t M_GetSpeed(const ITEM *const item)
{
    TRX_VALUE speed = {};
    if (ObjectProperty_GetItemValue(item, "speed", &speed)) {
        return speed.as_int;
    }

    return M_DEFAULT_SPEED;
}

static bool M_Trigger(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    M_PRIV *const p = item->priv;
    if (p == nullptr) {
        return true;
    }

    if (trigger->kind == ITEM_TRIGGER_ANTI) {
        return true;
    }

    item->timer = 0;
    return true;
}

static void M_Initialise(const int16_t item_num)
{
    Trap_Initialise(item_num);
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->speed = M_GetSpeed(item);
}

static void M_Move(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    int16_t room_num = item->room_num;
    const XYZ_32 pos = { item->pos.x, item->pos.y + p->speed, item->pos.z };
    const SECTOR *const sector = Room_GetSector(pos, &room_num);
    if (Room_GetHeight(sector, pos) < pos.y + WALL_L) {
        Item_SetFinished(item, true);
        Sound_StopEffect(SFX_SPIKE_WALL);
    } else {
        item->pos.y = pos.y;
        Item_UpdateRoom(item_num, room_num);
        Sound_Effect(SFX_SPIKE_WALL, &item->pos, SPM_NORMAL);
    }
}

static int32_t M_GetDamage(const ITEM *const item)
{
    TRX_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, "damage", &damage)) {
        return damage.as_int;
    }

    return M_DEFAULT_DAMAGE;
}

static void M_HitLara(ITEM *const item)
{
    Lara_TakeDamage(M_GetDamage(item), true);

    const ITEM *const lara_item = Lara_GetItem();
    Spawn_BloodBath(
        lara_item->pos.x, item->pos.y + LARA_HEIGHT, lara_item->pos.z,
        M_BLOOD_SPEED, item->rot.y, lara_item->room_num, 3);
    item->touch_bits = 0;

    Sound_Effect(SFX_LARA_FLESH_WOUND, &item->pos, SPM_NORMAL);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!Item_IsTriggerActive(item)) {
        Trap_Reset(item);
    } else if (!item->is_finished) {
        M_Move(item_num);
    }

    if (item->touch_bits) {
        M_HitLara(item);
    }

    if (Item_IsTriggerActive(item) && !item->is_finished) {
        Item_Animate(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_position = true;
    obj->save_flags = true;
    obj->trigger_func = M_Trigger;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "speed", M_DEFAULT_SPEED,
            "Offset applied each frame while the ceiling spikes descend."),
        OBJECT_PROPERTY_INT(
            "damage", M_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the ceiling spikes."));
}

REGISTER_OBJECT(O_CEILING_SPIKES, M_Setup)
