#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/lara.h>

// clang-format off
#define M_ALL_BITS       (-1)
#define M_CEILING_BITS   0b00000001'11000000
#define M_DEFAULT_DAMAGE LARA_MAX_HITPOINTS
// clang-format on

typedef enum {
    M_ANIM_OFF = 1,
} M_ANIM;

typedef enum {
    M_STATE_NULL,
    M_STATE_ON,
    M_STATE_OFF,
} M_STATE;

typedef struct {
    bool is_looped;
    bool has_activated;
    int32_t delay;
    int32_t timer;
    int32_t damage;
    int32_t deadly_bits;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "deadly_bits", &p->deadly_bits));
    MUST(JSON_READ_OPT(io, "timer", &p->timer));
    MUST(JSON_READ_OPT(io, "has_activated", &p->has_activated));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "deadly_bits", p->deadly_bits);
    JSONW_WRITE(io, "timer", p->timer);
    JSONW_WRITE(io, "has_activated", p->has_activated);
}

static const char *M_CheckWhole(const TRX_VALUE *const in)
{
    return in->as_int < 0 ? "value is below zero" : nullptr;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Item_SwitchToAnim(item, M_ANIM_OFF, 0);
    item->current_anim_state = M_STATE_OFF;
    item->goal_anim_state = M_STATE_OFF;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    const bool is_active = item->current_anim_state == M_STATE_ON;
    const int32_t damage = is_active ? p->damage : 0;
    Object_Collision_SphereTrap(
        item_num, lara_item, coll, damage, p->deadly_bits, is_active);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->deadly_bits = 0;

    if (!Item_IsTriggerActive(item)) {
        p->timer = 0;
        return;
    }

    if (item->current_anim_state == M_STATE_OFF) {
        if (!p->is_looped && p->has_activated) {
            return;
        }

        if (p->timer < p->delay) {
            p->timer++;
        } else {
            item->goal_anim_state = M_STATE_ON;
            p->has_activated = true;
            p->timer = 0;
        }
    } else if (Item_TestFrameRange(item, 1, 6)) {
        p->deadly_bits = M_ALL_BITS;
    } else if (Item_TestFrameRange(item, 7, 15)) {
        p->deadly_bits = M_CEILING_BITS;
    } else {
        p->deadly_bits = 0;
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_anim = true;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, delay, 0, M_CheckWhole,
            "Delay before the blades activate."),
        OBJECT_PROPERTY(
            M_PRIV, is_looped, false,
            "Whether the blades repeat while their trigger remains active."),
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara is struck by the blades."));
}

REGISTER_OBJECT(O_FALLING_BLADE, M_Setup)
