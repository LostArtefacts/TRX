#include <trx/game/creature.h>
#include <trx/game/lara.h>
#include <trx/game/lara/common.h>
#include <trx/game/objects/common.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/traps/common.h>
#include <trx/game/spawn.h>

#define M_DEFAULT_DAMAGE 50

typedef struct {
    int32_t damage;
} M_PRIV;

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item) && Item_TestFrameEqual(item, -1)) {
        Item_SwitchToAnim(item, 0, 0);
        Item_RemoveSimulated(item_num);
        item->enable_interpolation = false;
        return;
    }

    item->enable_interpolation = true;
    if (item->touch_bits != 0) {
        const ITEM *const lara_item = Lara_GetItem();
        Lara_TakeDamage(p->damage, true);
        Spawn_BloodBath(
            lara_item->pos.x, lara_item->pos.y - WALL_L / 2, lara_item->pos.z,
            lara_item->speed, lara_item->rot.y, lara_item->room_num, 3);
    }

    Item_Animate(item);
}

static void M_HandleSave(ITEM *const item, const SAVEGAME_STAGE stage)
{
    if (stage == SAVEGAME_STAGE_AFTER_LOAD) {
        item->enable_interpolation = Item_IsInPlay(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->handle_save_func = M_HandleSave;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the hook."));
}

REGISTER_OBJECT(O_HOOK, M_Setup)
