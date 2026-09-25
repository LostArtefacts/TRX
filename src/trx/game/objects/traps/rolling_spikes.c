#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/objects.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>

// clang-format off
#define M_DEFAULT_DAMAGE 50
#define M_RADIUS         (STEP_L * 18) // = 4608
#define M_TURN           (DEG_1 - 12) // = 170
// clang-format on

typedef struct {
    XYZ_32 mid_pos;
    int16_t angle;
    int32_t damage;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "angle", &p->angle));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "angle", p->angle);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->pos = XYZ_32_OffsetYaw(item->pos, item->rot.y + DEG_90, WALL_L / 2);

    M_PRIV *const p = item->priv;
    p->mid_pos = item->pos;
    p->mid_pos.y -= M_RADIUS;
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    Object_Collision_BoxTrap(item_num, lara_item, coll, p->damage);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    Sound_Effect(SFX_METAL_SCRAPE_LOOP_1, &item->pos, SPM_NORMAL);
    Sound_Effect(SFX_METAL_SCRAPE_LOOP_2, &item->pos, SPM_NORMAL);

    const int32_t distance = (M_RADIUS * Math_Cos(p->angle)) >> W2V_SHIFT;
    item->pos = XYZ_32_OffsetYaw(p->mid_pos, item->rot.y, distance);
    item->pos.y -= (M_RADIUS * Math_Sin(p->angle)) >> W2V_SHIFT;
    p->angle += M_TURN;

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    if (item->room_num != room_num) {
        Item_UpdateRoom(item_num, room_num);
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
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara is struck by the spikes."));
}

REGISTER_OBJECT(O_ROLLING_SPIKES, M_Setup)
