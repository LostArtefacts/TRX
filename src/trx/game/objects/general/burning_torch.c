#include <trx/game/objects/general/burning_torch.h>

#include <trx/config.h>
#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/collision.h>
#include <trx/game/const.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/pickup.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sparks.h>

#define M_SPIN (DEG_1 * 5)

typedef struct {
    bool is_lit;
    bool is_dropped;
    PICKUP_MODE pickup_mode;
    int32_t flame_count;
    int32_t aid_timer;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "is_lit", &p->is_lit));
    MUST(JSON_READ_OPT(io, "is_dropped", &p->is_dropped));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "is_lit", p->is_lit);
    JSONW_WRITE(io, "is_dropped", p->is_dropped);
}

static void M_Move(ITEM *const item)
{
    if (item->fall_speed != 0) {
        item->rot.z += M_SPIN;
    } else if (item->speed == 0) {
        item->rot.x = 0;
        item->rot.z = 0;
    }

    const XYZ_32 old_pos = item->pos;
    item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y, item->speed);

    M_PRIV *const p = item->priv;
    if (Room_Get(item->room_num)->flags.underwater) {
        item->fall_speed += (5 - item->fall_speed) / 2;
        item->speed += (5 - item->speed) / 2;
        p->is_lit = false;
    } else {
        item->fall_speed += GRAVITY;
    }
    item->pos.y += item->fall_speed;

    Collide_DoProperDetection(item, old_pos);

    COLLIDED_OBJECTS found;
    Collide_GetCollidedObjects(item, 0, true, false, &found);
    if (found.item_count > 0 || found.static_count > 0) {
        item->speed >>= 1;
    }
}

static void M_Burn(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (!p->is_lit) {
        return;
    }

    if (g_Config.visuals.enable_fire_lighting) {
        const RGB_888 color = {
            .r = (Random_GetControl() & 0x3F) + 192,
            .g = (Random_GetControl() & 0x1F) + 96,
            .b = 0,
        };
        Output_AddDynamicLightRGB(
            item->pos, 12 - (Random_GetControl() & 1), color);
    }

    p->flame_count++;
    if ((p->flame_count & 1) == 0) {
        Sparks_TriggerTorchFlame(Item_GetIndex(item), SPARK_NODE_TORCH_ITEM);
    }
}

// Returns whether Lara can stand where the torch lies, as she cannot pick it up
// while she swims.
static bool M_IsWithinReach(const ITEM *const item)
{
    if (!Room_Get(item->room_num)->flags.underwater) {
        return true;
    }
    return g_Config.gameplay.enable_wading
        && Lara_GetWaterDepth(item->pos, item->room_num)
        <= LARA_SWIM_DEPTH - STEP_L;
}

static const char *M_CheckPickupMode(const TRX_VALUE *const in)
{
    switch (in->as_int) {
    case PICKUP_MODE_NORMAL:
    case PICKUP_MODE_PLINTH_LOW:
    case PICKUP_MODE_PLINTH_HIGH:
    case PICKUP_MODE_HIDDEN:
        return nullptr;
    default:
        return "no such pickup mode for the torch";
    }
}

static void M_Initialise(const int16_t item_num)
{
    const ITEM *const item = Item_Get(item_num);
    if (item->is_visible) {
        Item_AddSimulated(item_num);
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->is_dropped) {
        M_Move(item);
    }
    M_Burn(item);

    if (!p->is_lit && p->pickup_mode != PICKUP_MODE_HIDDEN
        && M_IsWithinReach(item)) {
        Pickup_ControlAids(item, &p->aid_timer);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Pickup_Collision;
    obj->bounds_func = Pickup_Bounds;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
    obj->save_flags = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, pickup_mode, PICKUP_MODE_NORMAL, M_CheckPickupMode,
            "Pickup animation mode - 0: normal; 1: low pedestal; 2: high "
            "pedestal; 3: hidden reach-in."));
}

bool BurningTorch_IsLit(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->is_lit;
}

void BurningTorch_SetLit(ITEM *const item, const bool is_lit)
{
    M_PRIV *const p = item->priv;
    p->is_lit = is_lit;
}

void BurningTorch_Drop(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    p->is_dropped = true;
}

PICKUP_MODE BurningTorch_GetPickupMode(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    return p->pickup_mode;
}

REGISTER_OBJECT(O_BURNING_TORCH_ITEM, M_Setup)
