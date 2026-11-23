// TR4 flame emitters.
//
// These map two Core Design objects whose behaviour is heavily overloaded on
// the OCB (trigger_flags): a single object slot switches between fire and
// several unrelated jobs. Only the fire and the electric arcs are ported
// here; the rest is left as TODOs so the mess stays contained and visible.
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
// O_FLAME_EMITTER_TR4_SPREAD (Core FLAME_EMITTER3):
//   OCB 0   : small flames that flicker across one sector (ported).
//   OCB != 0: electric arcs (ported). Every one crackles with short sparks.
//             OCB 2 and 4 also strike a point two sectors behind the emitter.
//             OCB 3 and above also strike the ANIMATING3 items that share the
//             OCB, and the one with OCB 0, in turn.
//
// The ground and wall emitters are also torch-lighting targets: Lara lights
// her torch from a burning one, and lights an unlit one with her burning torch.

#include <trx/config.h>
#include <trx/core/math.h>
#include <trx/game/collision.h>
#include <trx/game/fx/fire.h>
#include <trx/game/fx/lightning.h>
#include <trx/game/gun.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/lights.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

#define M_IGNITE_DISTANCE 600
#define M_IGNITE_DISTANCE_SQR 0x40000
#define M_SPREAD_BURN_DISTANCE_SQR 202500
#define M_SPREAD_BURN_DAMAGE 5
#define M_ARC_REACH (WALL_L * 2)
#define M_ARC_SPARK_SPREAD 0x1FF
#define M_ARC_SIZE 32
#define M_ARC_GLOW 64
#define M_ARC_BEHIND_SEGMENTS 3
#define M_ARC_TARGET_SEGMENTS 5
#define M_ARC_SPARK_SEGMENTS 3
#define M_ARC_LIFE 24
#define M_ARC_LONG_LIFE 32
#define M_ARC_TARGET_COUNT 2

typedef enum {
    M_OCB_ARC_BEHIND_1 = 2,
    M_OCB_ARC_BEHIND_2 = 4,
    // Applies to this OCB and all above it.
    M_OCB_ARC_TARGETS = 3,
} M_OCB;

typedef struct {
    int16_t timer;
    int16_t offsets;
    bool second;
    // The ANIMATING3 items that the arcs strike in turn: the one that shares
    // the OCB of the emitter, and the one with OCB 0.
    int16_t arc_target_nums[M_ARC_TARGET_COUNT];
} M_SPREAD_PRIV;

static const uint8_t m_SpreadOffsets[16][2] = {
    { 9, 9 },   { 24, 9 },  { 40, 9 },  { 55, 9 },  { 9, 24 },  { 24, 24 },
    { 40, 24 }, { 55, 24 }, { 9, 40 },  { 24, 40 }, { 40, 40 }, { 55, 40 },
    { 9, 55 },  { 24, 55 }, { 40, 55 }, { 55, 55 },
};

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

static void M_BurnLaraInSpread(const ITEM *const item)
{
    const LARA_INFO *const lara = Lara_GetLaraInfo();
    if (lara->burn || !Lara_IsNearItem(&item->pos, M_IGNITE_DISTANCE)) {
        return;
    }

    Lara_TakeDamage(M_SPREAD_BURN_DAMAGE, true);
    const ITEM *const lara_item = Lara_GetItem();
    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    if (SQUARE(dx) + SQUARE(dz) < M_SPREAD_BURN_DISTANCE_SQR) {
        Lara_CatchFire();
    }
}

static void M_InitialiseSpread(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_SPREAD_PRIV *const p = item->priv;
    p->arc_target_nums[0] = NO_ITEM;
    p->arc_target_nums[1] = NO_ITEM;

    const int32_t ocb = M_GetOCB(item);
    if (ocb < M_OCB_ARC_TARGETS) {
        return;
    }

    for (int16_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const other = Item_Get(i);
        if (other->object_id != O_ANIMATING_3) {
            continue;
        }
        const int32_t other_ocb = M_GetOCB(other);
        if (other_ocb == ocb) {
            p->arc_target_nums[0] = i;
        } else if (other_ocb == 0) {
            p->arc_target_nums[1] = i;
        }
    }
}

static void M_CreateArc(
    const XYZ_32 src, const XYZ_32 dst, const RGB_888 color,
    const uint8_t segments)
{
    if ((Random_GetControl() & 3) == 0) {
        FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
            .src = src,
            .dst = dst,
            .variation = (Random_GetControl() & 0x1F) + 96,
            .color = color,
            .life = M_ARC_LONG_LIFE,
            .size = M_ARC_SIZE,
            .segments = segments,
            .flags = FX_LIGHTNING_F_CURVED,
        });
    } else {
        FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
            .src = src,
            .dst = dst,
            .variation = (Random_GetControl() & 0x1F) + 64,
            .color = color,
            .life = M_ARC_LIFE,
            .size = M_ARC_SIZE,
            .segments = segments,
            .flags = FX_LIGHTNING_F_NONE,
        });
    }
}

static void M_ControlArcs(const ITEM *const item, const int32_t ocb)
{
    const M_SPREAD_PRIV *const p = item->priv;
    Sound_Effect(SFX_ELECTRIC_ARCING_LOOP, &item->pos, SPM_NORMAL);
    const uint8_t g = (Random_GetControl() & 0x3F) + 192;
    const uint8_t b = (Random_GetControl() & 0x3F) + 192;
    const RGB_888 color = { .r = 0, .g = g, .b = b };
    const int32_t counter = (int32_t)Output_GetTimeInGame();

    if ((counter & 3) == 0
        && (ocb == M_OCB_ARC_BEHIND_1 || ocb == M_OCB_ARC_BEHIND_2)) {
        const XYZ_32 dst =
            XYZ_32_OffsetYaw(item->pos, item->rot.y - DEG_180, M_ARC_REACH);
        M_CreateArc(item->pos, dst, color, M_ARC_BEHIND_SEGMENTS);
    }

    const int16_t target_num = p->arc_target_nums[(counter >> 2) & 1];
    if (ocb >= M_OCB_ARC_TARGETS && (counter & 1) == 0
        && target_num != NO_ITEM) {
        const ITEM *const target = Item_Get(target_num);
        XYZ_32 dst = { .x = 0, .y = -64, .z = 20 };
        Collide_GetJointAbsPosition(target, &dst, 0);
        if ((counter & 3) == 0) {
            M_CreateArc(item->pos, dst, color, M_ARC_TARGET_SEGMENTS);
        }
        if (ocb != M_OCB_ARC_TARGETS || M_GetOCB(target) != 0) {
            FX_Lightning_CreateGlow(dst, color, M_ARC_GLOW);
        }
    }

    if ((counter & 3) == 2) {
        XYZ_32 dst = item->pos;
        dst.x += (Random_GetControl() & M_ARC_SPARK_SPREAD)
            - (M_ARC_SPARK_SPREAD + 1) / 2;
        dst.y += (Random_GetControl() & M_ARC_SPARK_SPREAD)
            - (M_ARC_SPARK_SPREAD + 1) / 2;
        dst.z += (Random_GetControl() & M_ARC_SPARK_SPREAD)
            - (M_ARC_SPARK_SPREAD + 1) / 2;
        FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
            .src = item->pos,
            .dst = dst,
            .variation = (Random_GetControl() & 0xF) + 16,
            .color = color,
            .life = M_ARC_LIFE,
            .size = M_ARC_SIZE,
            .segments = M_ARC_SPARK_SEGMENTS,
            .flags = FX_LIGHTNING_F_CURVED | FX_LIGHTNING_F_MOVING_END,
        });
        FX_Lightning_CreateGlow(item->pos, color, M_ARC_GLOW);
    }
}

static void M_ControlSpread(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    const int32_t ocb = M_GetOCB(item);
    if (ocb != 0) {
        M_ControlArcs(item, ocb);
        return;
    }

    M_SPREAD_PRIV *const p = item->priv;
    if (p->timer == 0) {
        p->timer = (Random_GetControl() & 3) + 8;
        const int16_t offsets = Random_GetControl() & 0x3F;
        p->offsets = offsets == p->offsets ? (offsets + 13) & 0x3F : offsets;
    } else {
        p->timer--;
    }

    const int32_t idx = p->second ? (p->offsets >> 3) + 8 : p->offsets & 7;
    p->second = !p->second;
    const XYZ_32 pos = {
        .x = item->pos.x + 16 * (m_SpreadOffsets[idx][0] - 32),
        .y = item->pos.y,
        .z = item->pos.z + 16 * (m_SpreadOffsets[idx][1] - 32),
    };
    Sparks_TriggerFireFlame(pos, -1, 2);
    // The original adds its light at the flame offset rather than at the
    // flame, which puts it near the world origin, so these fires cast no
    // light.
    Sound_Effect(SFX_LOOP_FOR_SMALL_FIRES, &item->pos, SPM_NORMAL);
    M_BurnLaraInSpread(item);
}

static void M_SetupGround(OBJECT *const obj)
{
    obj->control_func = M_ControlGround;
    obj->collision_func = Gun_Torch_FireCollision;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupWall(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseWall;
    obj->control_func = M_ControlWall;
    obj->collision_func = Gun_Torch_FireCollision;
    obj->draw_func = nullptr;
    obj->save_flags = true;
}

static void M_SetupSpread(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseSpread;
    obj->control_func = M_ControlSpread;
    obj->draw_func = nullptr;
    obj->save_flags = true;
    obj->priv_size = sizeof(M_SPREAD_PRIV);
}

REGISTER_OBJECT(O_FLAME_EMITTER_TR4_GROUND, M_SetupGround)
REGISTER_OBJECT(O_FLAME_EMITTER_TR4_WALL, M_SetupWall)
REGISTER_OBJECT(O_FLAME_EMITTER_TR4_SPREAD, M_SetupSpread)
