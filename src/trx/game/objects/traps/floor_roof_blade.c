#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>

// clang-format off
#define M_DEADLY_BITS  0b00011110
#define M_TOUCH_DAMAGE 20
#define M_SNAP_DAMAGE  200
// clang-format on

typedef struct {
    int32_t touch_damage;
    int32_t snap_damage;
    int32_t applied_damage;
    int32_t deadly_bits;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "applied_damage", &p->applied_damage));
    MUST(JSON_READ_OPT(io, "deadly_bits", &p->deadly_bits));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "applied_damage", p->applied_damage);
    JSONW_WRITE(io, "deadly_bits", p->deadly_bits);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    Object_Collision_SphereTrap(
        item_num, lara_item, coll, p->applied_damage, p->deadly_bits, false);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        p->deadly_bits = 0;
        Item_SwitchToAnim(item, 0, 0);
        return;
    }

    const int16_t frame = Item_GetRelativeFrame(item);
    if (frame <= 5 || frame >= 58 || (frame >= 8 && frame <= 54)) {
        p->deadly_bits = 0;
    } else {
        p->deadly_bits = M_DEADLY_BITS;
        p->applied_damage = frame > 7 ? p->snap_damage : p->touch_damage;
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
            M_PRIV, touch_damage, M_TOUCH_DAMAGE,
            "Damage dealt when Lara touches the blades."),
        OBJECT_PROPERTY(
            M_PRIV, snap_damage, M_SNAP_DAMAGE,
            "Damage dealt when the blades snap shut on Lara."));
}

REGISTER_OBJECT(O_FLOOR_BLADE, M_Setup)
REGISTER_OBJECT(O_ROOF_BLADE, M_Setup)
