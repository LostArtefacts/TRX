#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/objects.h>

// clang-format off
#define M_DEADLY_BITS  0b01111111'11111000'00000000
#define M_TOUCH_DAMAGE 50
#define M_PRONG_DAMAGE 150
// clang-format on

typedef struct {
    int32_t touch_damage;
    int32_t prong_damage;
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
    if ((frame <= 14 || frame >= 24) && (frame < 138 || frame > 140)) {
        if (frame < 141) {
            p->deadly_bits = 0;
        } else {
            p->applied_damage = p->touch_damage;
            p->deadly_bits = M_DEADLY_BITS;
        }
    } else {
        p->applied_damage = p->prong_damage;
        p->deadly_bits = M_DEADLY_BITS;
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
            "Damage dealt when Lara touches the ball."),
        OBJECT_PROPERTY(
            M_PRIV, prong_damage, M_PRONG_DAMAGE,
            "Damage dealt when the prongs hit Lara."));
}

REGISTER_OBJECT(O_SPIKE_BALL, M_Setup)
