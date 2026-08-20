#include <trx/game/lara.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/traps/common.h>
#include <trx/game/objects/traps/movable_block.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

#define M_DEFAULT_DAMAGE 20
#define M_BLOOD_SPEED 1
#define M_DEFAULT_SPEED 5

typedef struct {
    int32_t damage;
    int32_t speed;
} M_PRIV;

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

static void M_Move(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    int16_t room_num = item->room_num;
    const XYZ_32 pos = { item->pos.x, item->pos.y + p->speed, item->pos.z };
    const SECTOR *const sector = Room_GetSector(pos, &room_num);

    if (MovableBlock_TestSquareClaimed(
            (XYZ_32) { item->pos.x, item->pos.y + WALL_L, item->pos.z })) {
        Sound_StopEffect(SFX_SPIKE_WALL);
        return;
    }

    if (Room_GetHeight(sector, pos) < pos.y + WALL_L) {
        Item_SetFinished(item, true);
        Sound_StopEffect(SFX_SPIKE_WALL);
    } else {
        item->pos.y = pos.y;
        Item_UpdateRoom(item_num, room_num);
        Sound_Effect(SFX_SPIKE_WALL, &item->pos, SPM_NORMAL);
    }
}

static void M_HitLara(ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    Lara_TakeDamage(p->damage, true);

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
    obj->initialise_func = Trap_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision_Trap;
    obj->priv_size = sizeof(M_PRIV);
    obj->save_position = true;
    obj->save_flags = true;
    obj->trigger_func = M_Trigger;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, speed, M_DEFAULT_SPEED,
            "Offset applied each frame while the ceiling spikes descend."),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the ceiling spikes."));
}

REGISTER_OBJECT(O_CEILING_SPIKES, M_Setup)
