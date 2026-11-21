#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/game_flow.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/version.h>

// clang-format off
#define M_PULSE_OCB_QUAKE  1
#define M_PULSE_OCB_RUMBLE 2
#define M_PULSE_STEP       2048
#define M_PULSE_FALLOFF    18
// clang-format on

typedef struct {
    RGB_888 color;
} M_PRIV;

typedef struct {
    int32_t ocb;
    int16_t phase;
} M_PULSE_PRIV;

static void M_InitialiseGeneric(const int16_t item_num, const RGB_888 color)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->color = color;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;

    if (!Item_IsTriggerActive(item)) {
        return;
    }

    Output_AddDynamicLightRGB(item->pos, 24, p->color);
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PRIV);
    obj->control_func = M_Control;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value = {};
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)
        || value.type != TVT_S32) {
        return 0;
    }
    return value.as_int;
}

static void M_InitialisePulse(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PULSE_PRIV *const p = item->priv;
    p->ocb = M_GetOCB(item);
}

static void M_ControlPulse(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PULSE_PRIV *const p = item->priv;
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const int32_t flip_group = GF_GetCurrentLevel()->pulse_flip_group;
    if (flip_group != NO_FLIP_GROUP && !Room_GetFlipGroupStatus(flip_group)) {
        return;
    }

    if (p->ocb == M_PULSE_OCB_QUAKE) {
        p->ocb = 0;
        Output_Flash_Trigger((RGB_888) { 128, 255, 255 });
        g_Camera.bounce = -128;
        Sound_Effect(SFX_ROLLING_BALL_FALL, nullptr, SPM_NORMAL);
        Sound_Effect(SFX_EXPLOSION_2, nullptr, SPM_NORMAL);
    } else if (p->ocb == M_PULSE_OCB_RUMBLE) {
        Sound_Effect(SFX_MAPPER_PYRAMID_OPEN, &item->pos, SPM_NORMAL);
        if (Room_Get(g_Camera.pos.room_num)->flags.wind) {
            Output_Flash_Trigger((RGB_888) { 64, 128, 128 });
        }
    }

    p->phase -= M_PULSE_STEP;
    int32_t shade =
        ABS(Math_Sin(p->phase + ((item->pos.y & 0x3FFF) << 2)) >> 6);
    CLAMPG(shade, 255);
    const RGB_888 color = {
        .r = (shade * 64) >> 9,
        .g = (shade * 255) >> 9,
        .b = (shade * 255) >> 9,
    };
    Output_AddDynamicLightRGB(item->pos, M_PULSE_FALLOFF, color);
}

static RESULT M_LoadPulsePriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PULSE_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "ocb", &p->ocb));
    MUST(JSON_READ_OPT(io, "phase", &p->phase));
    return OK;
}

static void M_SavePulsePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PULSE_PRIV *const p = item->priv;
    JSONW_WRITE(io, "ocb", p->ocb);
    JSONW_WRITE(io, "phase", p->phase);
}

static void M_SetupPulse(OBJECT *const obj)
{
    obj->priv_size = sizeof(M_PULSE_PRIV);
    obj->priv_load_func = M_LoadPulsePriv;
    obj->priv_save_func = M_SavePulsePriv;
    obj->initialise_func = M_InitialisePulse;
    obj->control_func = M_ControlPulse;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

#define M_DEFINE_LIGHT(name, r, g, b)                                          \
    static void M_Initialise##name(const int16_t num)                          \
    {                                                                          \
        M_InitialiseGeneric(num, (RGB_888) { r, g, b });                       \
    }                                                                          \
    static void M_Setup##name(OBJECT *const obj)                               \
    {                                                                          \
        M_SetupCommon(obj);                                                    \
        obj->initialise_func = M_Initialise##name;                             \
    }                                                                          \
    REGISTER_OBJECT(O_##name##_LIGHT, M_Setup##name)

M_DEFINE_LIGHT(RED, 255, 0, 0)
M_DEFINE_LIGHT(GREEN, 0, 255, 0)
M_DEFINE_LIGHT(BLUE, 0, 0, 255)
M_DEFINE_LIGHT(WHITE, 224, 224, 255)

#undef M_DEFINE_LIGHT

static void M_InitialiseAmber(const int16_t item_num)
{
    M_InitialiseGeneric(item_num, (RGB_888) { 255, 192, 0 });
}

static void M_SetupAmber(OBJECT *const obj)
{
    if (g_TRVersion == 4) {
        M_SetupPulse(obj);
        return;
    }
    M_SetupCommon(obj);
    obj->initialise_func = M_InitialiseAmber;
}

REGISTER_OBJECT(O_AMBER_LIGHT, M_SetupAmber)
