#include <trx/game/objects/general/waterfall.h>

#include <trx/game/items.h>
#include <trx/game/objects.h>
#include <trx/game/sound.h>

typedef struct {
    WATERFALL_SOUND loop_sound;
    bool hide_when_inactive;
} M_PRIV;

static const char *M_CheckLoopSound(const TRX_VALUE *const in)
{
    return in->as_int < 0 || in->as_int >= WATERFALL_SOUND_NUMBER_OF
        ? "no such waterfall sound"
        : nullptr;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        if (p->hide_when_inactive) {
            item->is_visible = false;
        }
        return;
    }

    item->is_visible = true;
    switch (p->loop_sound) {
    case WATERFALL_SOUND_SAND:
        Sound_Effect(SFX_SAND_LOOP, &item->pos, SPM_NORMAL);
        break;
    case WATERFALL_SOUND_WATER:
        Sound_Effect(SFX_WATERFALL_LOOP, &item->pos, SPM_NORMAL);
        break;
    default:
        break;
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->save_flags = true;
    obj->priv_size = sizeof(M_PRIV);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, loop_sound, WATERFALL_SOUND_NONE, M_CheckLoopSound,
            "The sound the waterfall loops while it runs - 0: none; 1: sand; "
            "2: water"),
        OBJECT_PROPERTY(
            M_PRIV, hide_when_inactive, false,
            "Hide the waterfall while its trigger is inactive."));
}

REGISTER_OBJECT(O_WATERFALL_1, M_Setup)
REGISTER_OBJECT(O_WATERFALL_2, M_Setup)
REGISTER_OBJECT(O_WATERFALL_3, M_Setup)
