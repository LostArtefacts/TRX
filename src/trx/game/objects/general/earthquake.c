#include <trx/game/objects/general/earthquake.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/const.h>
#include <trx/game/objects.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/version.h>

#define M_DEFAULT_MODE                                                         \
    (g_TRVersion == 1 ? EARTHQUAKE_MODE_RANDOM_1                               \
                      : (g_TRVersion == 2 ? EARTHQUAKE_MODE_RANDOM_2           \
                                          : EARTHQUAKE_MODE_RAMPED))

typedef struct {
    EARTHQUAKE_MODE mode;
    bool shake_camera;
    bool trigger_items;
    int32_t lifetime;
    int32_t shake_intensity;
    int32_t target_intensity;
    int32_t target_timer;
    int32_t active_timer;
} M_PRIV;

static const char *M_CheckMode(const TRX_VALUE *const in)
{
    return in->as_int < 0 || in->as_int >= EARTHQUAKE_MODE_NUMBER_OF
        ? "no such earthquake mode"
        : nullptr;
}

static const char *M_CheckWhole(const TRX_VALUE *const in)
{
    return in->as_int < 0 ? "value is below zero" : nullptr;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    SHOULD(JSON_READ_OPT(io, "shake_intensity", &p->shake_intensity));
    SHOULD(JSON_READ_OPT(io, "target_intensity", &p->target_intensity));
    SHOULD(JSON_READ_OPT(io, "target_timer", &p->target_timer));
    SHOULD(JSON_READ_OPT(io, "active_timer", &p->active_timer));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "shake_intensity", p->shake_intensity);
    JSONW_WRITE(io, "target_intensity", p->target_intensity);
    JSONW_WRITE(io, "target_timer", p->target_timer);
    JSONW_WRITE(io, "active_timer", p->active_timer);
}

static void M_ActivateRelatedItem(ITEM *const earth_item)
{
    // The related item may have been authored hidden; the quake reveals it as
    // it starts simulating.
    Item_SetVisible(earth_item, true);
    Item_AddSimulated(Item_GetIndex(earth_item));
    earth_item->trigger = (ITEM_TRIGGER_STATE) { .mask = TRIGGER_MASK_ALL };
    earth_item->timer = 0;
}

static void M_FindAndActivateRelatedItems(const ITEM *const item)
{
    OBJECT_ID object_id_to_activate = NO_OBJECT;
    const int32_t random = Random_GetControl();
    if (random < 512) {
        object_id_to_activate = O_FLAME_EMITTER;
    } else if (random < 1024) {
        object_id_to_activate = O_FALLING_CEILING_1;
    }
    if (object_id_to_activate == NO_OBJECT
        || !Object_Get(object_id_to_activate)->loaded) {
        return;
    }

    int16_t related_item_num = Room_Get(item->room_num)->item_num;
    while (related_item_num != NO_ITEM) {
        ITEM *const earth_item = Item_Get(related_item_num);
        if (earth_item->object_id == object_id_to_activate
            && !Item_IsInPlay(earth_item) && !earth_item->is_finished) {
            M_ActivateRelatedItem(earth_item);
            break;
        }
        related_item_num = earth_item->next_item;
    }
}

static void M_Reset(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->shake_intensity = 0;
    p->target_intensity = 0;
    p->target_timer = 0;
    p->active_timer = 0;
    Item_RemoveSimulated(item_num);
}

static void M_ShakeCamera(const M_PRIV *const p, const int32_t intensity)
{
    if (p->shake_camera) {
        g_Camera.bounce = intensity;
    }
}

static void M_ControlRandom1(const M_PRIV *const p)
{
    if (Random_GetDraw() < 256) {
        M_ShakeCamera(p, -150);
        Sound_Effect(SFX_EARTHQUAKE_1, nullptr, SPM_NORMAL);
    } else if (Random_GetControl() < 1024) {
        M_ShakeCamera(p, 50);
        Sound_Effect(SFX_EARTHQUAKE_2, nullptr, SPM_NORMAL);
    }
}

static void M_ControlRandom2(const M_PRIV *const p)
{
    if (Random_GetDraw() < 512) {
        M_ShakeCamera(p, -200);
        Sound_Effect(SFX_EARTHQUAKE_1, nullptr, SPM_NORMAL);
    }
}

static void M_ControlRamped(M_PRIV *const p)
{
    if (p->target_intensity == 0) {
        p->target_intensity = 100;
    }

    if (p->target_timer == 0
        && ABS(p->shake_intensity - p->target_intensity) < 16) {
        if (p->target_intensity == 20) {
            p->target_intensity = 100;
            p->target_timer = (Random_GetControl() & 0x7F) + 90;
        } else {
            p->target_intensity = 20;
            p->target_timer = (Random_GetControl() & 0x7F) + 30;
        }
    }

    if (p->target_timer != 0) {
        p->target_timer--;
    }

    if (p->shake_intensity > p->target_intensity) {
        p->shake_intensity -= (Random_GetControl() & 7) + 2;
    } else {
        p->shake_intensity += (Random_GetControl() & 7) + 2;
    }

    M_ShakeCamera(p, -p->shake_intensity);
    Sound_Effect(
        SFX_EARTHQUAKE_LOOP, nullptr,
        ((p->shake_intensity << 16) + 0x1000000) | SPM_PITCH);
}

static void M_ControlBasic(const M_PRIV *const p)
{
    M_ShakeCamera(p, -64 - (Random_GetControl() & 0x1F));
    Sound_Effect(SFX_EARTHQUAKE_LOOP, nullptr, SPM_NORMAL);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        M_Reset(item_num);
        return;
    }

    switch (p->mode) {
    case EARTHQUAKE_MODE_RANDOM_1:
        M_ControlRandom1(p);
        break;
    case EARTHQUAKE_MODE_RANDOM_2:
        M_ControlRandom2(p);
        break;
    case EARTHQUAKE_MODE_RAMPED:
        M_ControlRamped(p);
        break;
    case EARTHQUAKE_MODE_BASIC:
        M_ControlBasic(p);
        break;
    default:
        break;
    }

    if (p->trigger_items) {
        M_FindAndActivateRelatedItems(item);
    }

    p->active_timer++;
    if (p->lifetime != 0 && p->active_timer > p->lifetime) {
        Sound_Effect(SFX_EARTHQUAKE_2, nullptr, SPM_NORMAL);
        Item_Destroy(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->control_func = M_Control;
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->draw_func = nullptr;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, mode, M_DEFAULT_MODE, M_CheckMode,
            "Control mode - 0: random (TR1); 1: random (TR2); 2: "
            "ramped (TR3); 3: basic (TR4)"),
        OBJECT_PROPERTY(
            M_PRIV, shake_camera, true,
            "Whether or not the earthquake shakes the camera while it is "
            "active."),
        OBJECT_PROPERTY_CHECKED(
            M_PRIV, lifetime, 0, M_CheckWhole,
            "The lifetime of the earthquake to remain active. Zero implies "
            "active until anti-triggered."),
        OBJECT_PROPERTY(
            M_PRIV, trigger_items, true,
            "Whether or not the earthquake triggers falling ceiling and flame "
            "emitters placed in the same room at random."));
}

REGISTER_OBJECT(O_EARTHQUAKE, M_Setup)
