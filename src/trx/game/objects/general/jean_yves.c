#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/objects.h>
#include <trx/game/random.h>
#include <trx/game/waypoint.h>

#define M_MAX_STAGE 3
#define M_STAGE_ANIMS 4

typedef struct {
    int32_t stage;
} M_PRIV;

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "stage", &p->stage));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "stage", p->stage);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->stage = M_GetOCB(item);
    Item_SwitchToAnim(item, 0, 0);
    item->current_anim_state = 1;
    item->goal_anim_state = 1;
}

// Plays one of the four idle animations of the stage that Lara has reached.
// A new stage starts one of its animations at once.
static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    const int32_t stage = MIN(Waypoint_GetHighest(), M_MAX_STAGE);

    if (p->stage < stage) {
        const int16_t anim = (Random_GetControl() & 3) + M_STAGE_ANIMS * stage;
        Item_SwitchToAnim(item, anim, 0);
        item->current_anim_state = anim + 1;
        item->goal_anim_state = anim + 1;
        p->stage = stage;
    } else {
        int16_t idle;
        if ((Random_GetControl() & 3) != 0) {
            idle = (Random_GetControl() & 1) + 1;
        } else {
            idle = 3 * (Random_GetControl() & 1);
        }
        item->goal_anim_state =
            idle + ((item->current_anim_state - 1) & 0xC) + 1;
    }

    Item_Animate(item);
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_anim = true;
    obj->save_position = true;
}

REGISTER_OBJECT(O_JEAN_YVES, M_Setup)
