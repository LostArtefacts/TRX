#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/objects.h>

#define M_DEFAULT_DAMAGE 100

typedef struct {
    int32_t damage;
    int32_t applied_damage;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "applied_damage", &p->applied_damage));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "applied_damage", p->applied_damage);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    Object_Collision_BoxTrap(item_num, lara_item, coll, p->applied_damage);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        Item_SwitchToAnim(item, 0, 0);
        return;
    }

    M_PRIV *const p = item->priv;
    if (Item_TestFrameEqual(item, -1) || Item_GetRelativeFrame(item) < 38) {
        p->applied_damage = 0;
    } else {
        p->applied_damage = p->damage;
    }
    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_anim = true;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara touches the blades."));
}

REGISTER_OBJECT(O_CATWALK_BLADE, M_Setup)
