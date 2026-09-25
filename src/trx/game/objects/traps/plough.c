#include <trx/game/objects.h>

// clang-format off
#define M_DEADLY_BITS    0b00000011'11110000'00000000
#define M_DEFAULT_DAMAGE 50
// clang-format on

typedef struct {
    int32_t damage;
} M_PRIV;

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    const bool is_active = Item_IsTriggerActiveRO(item);
    const int32_t damage = is_active ? p->damage : 0;
    const int32_t deadly_bits = is_active ? M_DEADLY_BITS : 0;
    Object_Collision_SphereTrap(
        item_num, lara_item, coll, damage, deadly_bits, false);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        Item_Animate(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->save_flags = true;
    obj->save_anim = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara is struck by the blades."));
}

REGISTER_OBJECT(O_PLOUGH, M_Setup)
