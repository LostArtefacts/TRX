// TR4 flame emitters.
//
// These map two Core Design objects whose behaviour is heavily overloaded on
// the OCB (trigger_flags): a single object slot switches between fire and
// several unrelated jobs. Only the fire is ported here; the rest is left as
// TODOs so the mess stays contained and visible.
//
// O_FLAME_EMITTER_TR4_GROUND (Core FLAME_EMITTER):
//   OCB >= 0 : big static ground/brazier fire (ported). Also drives the library
//              puzzle "is it lit" table in the original (not ported).
//   OCB <  0 : a super-jet flame, steady or pulsing on an item-flag timer (not
//              ported).
//
// O_FLAME_EMITTER_TR4_WALL (Core FLAME_EMITTER2):
//   OCB 0/1/123 : wall flame, size from the OCB, fade from an item flag (fire
//                 ported; the fade animation is not).
//   OCB 2       : a flame that drifts and douses itself underwater (not
//                 ported).
//   OCB <  0    : a flip-map trigger, not a flame at all (not ported).
//
// Both objects are also torch-lighting targets in the original (FireCollision);
// the torch is not ported.

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/game/fx/fire.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output/lights.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>

#define M_IGNITE_DISTANCE 600
#define M_IGNITE_DISTANCE_SQR 0x40000

static int32_t M_GetOCB(const ITEM *item);
static void M_BurnLaraIfNear(const ITEM *item);

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value = {};
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)
        || value.type != TVT_S32) {
        return 0;
    }
    return value.as_int;
}

static void M_BurnLaraIfNear(const ITEM *const item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->burn || !Lara_IsNearItem(&item->pos, M_IGNITE_DISTANCE)) {
        return;
    }

    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    if (SQUARE(dx) + SQUARE(dz) < M_IGNITE_DISTANCE_SQR) {
        Lara_CatchFire();
    }
}

static void M_Light(const XYZ_32 pos, const int32_t falloff)
{
    if (!g_Config.visuals.enable_fire_lighting) {
        return;
    }
    const uint8_t r = (Random_GetControl() & 0x3F) + 192;
    const uint8_t g = (Random_GetControl() & 0x1F) + 96;
    Output_AddDynamicLightRGB(pos, falloff, (RGB_888) { r, g, 0 });
}

static void M_ControlGround(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    if (M_GetOCB(item) < 0) {
        // TODO: super-jet flame (steady + pulsing), not ported.
        return;
    }

    FX_Fire_Add(item->pos, 2, item->room_num, 0);
    M_Light(item->pos, 16 - (Random_GetControl() & 1));
    Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &item->pos, SPM_NORMAL);
    M_BurnLaraIfNear(item);
}

static void M_InitialiseWall(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->pos.y -= 64;

    const int32_t ocb = M_GetOCB(item);
    if (ocb == 123) {
        return;
    }

    // Push the flame off the wall it faces.
    const int32_t offset = ocb == 2 ? 80 : 256;
    switch (item->rot.y) {
    case 0:
        item->pos.z += offset;
        break;
    case DEG_90:
        item->pos.x += offset;
        break;
    case -DEG_180:
        item->pos.z -= offset;
        break;
    case -DEG_90:
        item->pos.x -= offset;
        break;
    default:
        break;
    }
}

static void M_ControlWall(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const int32_t ocb = M_GetOCB(item);
    if (ocb < 0) {
        // TODO: flip-map trigger (not a flame), not ported.
        return;
    }

    if (ocb != 2) {
        int32_t size = ocb == 123 ? 1 : 1 - ocb;
        CLAMP(size, 0, 2);
        // TODO: the original fades the fire via an item flag driven by
        // triggers.
        FX_Fire_Add(item->pos, size, item->room_num, 0);
    } else {
        // TODO: drifting flame that douses itself underwater, not ported.
    }

    if (ocb == 0 || ocb == 2) {
        M_Light(item->pos, 10);
    }
    Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &item->pos, SPM_NORMAL);
}

static void M_SetupGround(OBJECT *const obj)
{
    obj->control_func = M_ControlGround;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupWall(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseWall;
    obj->control_func = M_ControlWall;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

REGISTER_OBJECT(O_FLAME_EMITTER_TR4_GROUND, M_SetupGround)
REGISTER_OBJECT(O_FLAME_EMITTER_TR4_WALL, M_SetupWall)
