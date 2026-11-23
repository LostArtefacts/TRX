#include <trx/game/objects/traps/obelisk.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/game/collision.h>
#include <trx/game/fx/debris.h>
#include <trx/game/fx/lightning.h>
#include <trx/game/input.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/general/pulley_switch.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_OTHERS            2
#define M_MAX_SPHERES       34
#define M_CHARGE_TIME       256
#define M_MAX_TIME          346
#define M_BOLT_REACH        3456
#define M_TARGET_DIST       0x2000
#define M_BOLT_HEIGHT       256
#define M_SPARK_SPREAD      0x3FF
#define M_LARA_RANGE        20480
#define M_CASING_DEBRIS     128
#define M_BOLT_LIFE         24
#define M_BOLT_SIZE         32
#define M_BOLT_SEGMENTS     5
#define M_SPARK_SIZE        24
#define M_SPARK_SEGMENTS    3
#define M_BOLT_GLOW         48
#define M_SPARK_GLOW        64
// clang-format on

typedef enum {
    M_OCB_LEADER = 2,
} M_OCB;

typedef enum {
    // clang-format off
    M_ANIM_FRONT_PUSH     = 1,
    M_ANIM_FRONT_PUSH_END = 2,
    M_ANIM_STILL          = 3,
    M_ANIM_BACK_PUSH      = 5,
    M_ANIM_BACK_PUSH_END  = 6,
    // clang-format on
} M_ANIM;

typedef struct {
    // The two other obelisks, which the leader checks to see whether all
    // three line up.
    int16_t others[M_OTHERS];
    // The item that the bolts of the leader destroy, or NO_ITEM once it is
    // gone.
    int16_t target_num;
    // Counts the ticks since the obelisk was triggered.
    int16_t timer;
} M_PRIV;

static int32_t M_GetOCB(const ITEM *const item)
{
    TRX_VALUE value;
    if (!ObjectProperty_GetItemValue(item, "ocb", &value)) {
        return 0;
    }
    return value.as_int;
}

static int32_t M_GetCounter(void)
{
    return (int32_t)Output_GetTimeInGame();
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "target_num", &p->target_num));
    MUST(JSON_READ_OPT(io, "timer", &p->timer));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "target_num", p->target_num);
    JSONW_WRITE(io, "timer", p->timer);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->others[0] = NO_ITEM;
    p->others[1] = NO_ITEM;
    p->target_num = NO_ITEM;
    p->timer = 0;
    Item_SwitchToAnim(item, M_ANIM_STILL, 0);
    Item_AddSimulated(item_num);

    if (M_GetOCB(item) != M_OCB_LEADER) {
        return;
    }

    int32_t other_count = 0;
    for (int16_t i = 0; i < Item_GetLevelCount(); i++) {
        const ITEM *const other = Item_Get(i);
        if (other->object_id == O_OBELISK && i != item_num
            && other_count < M_OTHERS) {
            p->others[other_count++] = i;
        }
        if (other->object_id == O_ANIMATING_3) {
            p->target_num = i;
        }
    }
}

static bool M_IsLinedUp(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p->others[0] == NO_ITEM || p->others[1] == NO_ITEM) {
        return false;
    }
    return item->rot.y == -DEG_90 && Item_Get(p->others[0])->rot.y == DEG_90
        && Item_Get(p->others[1])->rot.y == 0;
}

static bool M_IsNearLara(const XYZ_32 pos)
{
    return XYZ_32_IsNearby(pos, Lara_GetItem()->pos, M_LARA_RANGE);
}

static XYZ_32 M_GetFacingPoint(const ITEM *const item, const int32_t dist)
{
    const int16_t angle = item->rot.y + DEG_90;
    return (XYZ_32) {
        .x = item->pos.x + ((dist * Math_Sin(angle)) >> W2V_SHIFT),
        .y = item->pos.y,
        .z = item->pos.z + ((dist * Math_Cos(angle)) >> W2V_SHIFT),
    };
}

static void M_Turn(ITEM *const item)
{
    Item_Animate(item);

    bool stop = false;
    if (Item_TestAnimEqual(item, M_ANIM_FRONT_PUSH_END)) {
        item->rot.y -= DEG_90;
        if (g_Input.action) {
            Item_SwitchToAnim(item, M_ANIM_FRONT_PUSH, 0);
        } else {
            stop = true;
        }
    }

    if (Item_TestAnimEqual(item, M_ANIM_BACK_PUSH_END)) {
        item->rot.y += DEG_90;
        if (g_Input.action) {
            Item_SwitchToAnim(item, M_ANIM_BACK_PUSH, 0);
        } else {
            stop = true;
        }
    }

    if (stop) {
        Item_SwitchToAnim(item, M_ANIM_STILL, 0);
    }

    if (M_GetOCB(item) == M_OCB_LEADER) {
        ITEM *const pulley = Item_Find(O_SWITCH_TYPE_PULLEY);
        if (pulley != nullptr) {
            PulleySwitch_SetLocked(pulley, !M_IsLinedUp(item));
        }
    }
}

static void M_ShatterTarget(const int16_t target_num)
{
    ITEM *const target = Item_Get(target_num);
    if ((target->mesh_bits & 1) == 0) {
        return;
    }

    Sound_Effect(SFX_HIT_ROCK, &target->pos, SPM_NORMAL);
    SPHERE spheres[M_MAX_SPHERES];
    if (Collide_GetSpheres(target, spheres, true) > 0) {
        const OBJECT *const obj = Object_Get(target->object_id);
        const SHATTER_ITEM shatter_item = {
            .pos = spheres[0].pos,
            .mesh = Object_GetMesh(obj->mesh_idx),
            .bit = 1,
            .yaw = target->rot.y,
        };
        FX_Debris_ShatterItem(
            &shatter_item, M_CASING_DEBRIS, target->room_num, 0);
    }
    target->mesh_bits &= ~1;
}

// Destroys the casing in the middle of the room, and shows the item hidden
// inside it.
static void M_DestroyTarget(M_PRIV *const p, const XYZ_32 pos)
{
    const int16_t room_num = Item_Get(p->target_num)->room_num;
    M_ShatterTarget(p->target_num);
    Item_Destroy(p->target_num);
    Sparks_TriggerExplosionSparks(pos, 3, -2, 0, room_num);
    Sparks_TriggerExplosionSparks(pos, 3, -1, 0, room_num);
    p->target_num = NO_ITEM;

    ITEM *const reward = Item_Find(O_PUZZLE_ITEM_1_COMBO_1);
    if (reward != nullptr) {
        Item_SetVisible(reward, true);
        Sound_Effect(SFX_EXPLOSION_1, &reward->pos, SPM_NORMAL);
        Sound_Effect(SFX_EXPLOSION_2, &reward->pos, SPM_NORMAL);
    }
}

static void M_Shoot(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    if (p->timer > M_MAX_TIME) {
        return;
    }
    p->timer++;

    const int32_t shade = (Random_GetControl() & 0x1F) + 224;
    const int32_t green = shade - (Random_GetControl() & 0x1F) - 32;
    const int32_t blue = green - (Random_GetControl() & 0x1F) - 128;
    const RGB_888 color = { shade, green, blue };

    const int32_t counter = M_GetCounter();
    if ((counter & 1) == 0
        && (p->timer >= M_CHARGE_TIME || (Random_GetControl() & 1) != 0)) {
        int32_t reach = M_TARGET_DIST;
        if (p->timer < M_CHARGE_TIME && (counter & 3) == 0) {
            Sound_Effect(SFX_ELECTRIC_ONE_SHOT, &item->pos, SPM_NORMAL);
            reach = (Random_GetControl() & 0xFFF) + M_BOLT_REACH;
        }

        XYZ_32 src = M_GetFacingPoint(item, M_BOLT_REACH);
        src.y -= M_BOLT_HEIGHT;
        const XYZ_32 dst = M_GetFacingPoint(item, reach);
        if (M_IsNearLara(src) && M_IsNearLara(dst)) {
            if ((counter & 3) == 0) {
                FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
                    .src = src,
                    .dst = dst,
                    .variation = (Random_GetControl() & 0x1F) + 32,
                    .color = color,
                    .life = M_BOLT_LIFE,
                    .size = M_BOLT_SIZE,
                    .segments = M_BOLT_SEGMENTS,
                    .flags = FX_LIGHTNING_F_CURVED,
                });
            }
            FX_Lightning_CreateGlow(src, color, M_BOLT_GLOW);
        }
    }

    if (p->timer < M_CHARGE_TIME || M_GetOCB(item) != M_OCB_LEADER) {
        return;
    }

    const XYZ_32 src = M_GetFacingPoint(item, M_TARGET_DIST);
    Sound_Effect(SFX_ELECTRIC_ARCING_LOOP, &src, SPM_NORMAL);
    if ((counter & 1) == 0) {
        return;
    }

    XYZ_32 dst = src;
    dst.x += (Random_GetControl() & M_SPARK_SPREAD) - (M_SPARK_SPREAD + 1) / 2;
    dst.y += (Random_GetControl() & M_SPARK_SPREAD) - (M_SPARK_SPREAD + 1) / 2;
    dst.z += (Random_GetControl() & M_SPARK_SPREAD) - (M_SPARK_SPREAD + 1) / 2;
    if (!M_IsNearLara(src) || !M_IsNearLara(dst)) {
        return;
    }

    if (p->target_num != NO_ITEM) {
        M_DestroyTarget(p, src);
    }

    FX_Lightning_Create(&(FX_LIGHTNING_SETUP) {
        .src = src,
        .dst = dst,
        .variation = (Random_GetControl() & 0xF) + 16,
        .color = color,
        .life = M_BOLT_LIFE,
        .size = M_SPARK_SIZE,
        .segments = M_SPARK_SEGMENTS,
        .flags = FX_LIGHTNING_F_CURVED | FX_LIGHTNING_F_MOVING_END,
    });
    FX_Lightning_CreateGlow(src, color, M_SPARK_GLOW);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        M_Shoot(item);
    } else {
        M_Turn(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Object_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_position = true;
}

void Obelisk_StartIfLinedUp(const int16_t room_num)
{
    int16_t item_num = Room_Get(room_num)->item_num;
    while (item_num != NO_ITEM) {
        ITEM *const item = Item_Get(item_num);
        if (item->object_id == O_OBELISK && M_IsLinedUp(item)) {
            const M_PRIV *const p = item->priv;
            item->trigger.mask = TRIGGER_MASK_ALL;
            Item_Get(p->others[0])->trigger.mask = TRIGGER_MASK_ALL;
            Item_Get(p->others[1])->trigger.mask = TRIGGER_MASK_ALL;
            return;
        }
        item_num = item->next_item;
    }
}

REGISTER_OBJECT(O_OBELISK, M_Setup)
