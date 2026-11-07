#include <trx/config.h>
#include <trx/game/lara.h>
#include <trx/game/random.h>
#include <trx/game/sound.h>
#include <trx/game/spawn.h>

// clang-format off
#define M_DEADLY_BITS    0b00000011'01101101'10110110'00000000
#define M_DEFAULT_DAMAGE 100
#define M_NUM_BOUNDS     4
// clang-format on

typedef struct {
    int32_t damage;
} M_PRIV;

static const BOUNDS_16 m_Bounds[M_NUM_BOUNDS] = {
    {
        .min = { .x = -WALL_L / 2, .y = -WALL_L, .z = -STEP_L * 3 / 8 },
        .max = { .x = +WALL_L / 2, .y = -STEP_L * 7 / 2, .z = +STEP_L * 3 / 8 },
    },
    {
        .min = { .x = -WALL_L / 2, .y = -STEP_L / 2, .z = -STEP_L * 3 / 8 },
        .max = { .x = +WALL_L / 2, .y = +0, .z = +STEP_L * 3 / 8 },
    },
    {
        .min = { .x = -WALL_L / 2, .y = -WALL_L, .z = -STEP_L * 3 / 8 },
        .max = { .x = -STEP_L * 3 / 2, .y = +0, .z = +STEP_L * 3 / 8 },
    },
    {
        .min = { .x = +STEPUP_HEIGHT, .y = -WALL_L, .z = -STEP_L * 3 / 8 },
        .max = { .x = +WALL_L / 2, .y = +0, .z = +STEP_L * 3 / 8 },
    },
};

static bool M_ShouldPush(
    const ITEM *const item, const ITEM *const lara_item,
    const BOUNDS_16 *const item_bounds, const int32_t radius)
{
    const BOUNDS_16 *const lara_bounds = Item_GetBoundsAccurate(lara_item);
    if (item->pos.y + item_bounds->max.y
            <= lara_item->pos.y + lara_bounds->min.y
        || item->pos.y + item_bounds->min.y
            >= lara_item->pos.y + lara_bounds->max.y) {
        return false;
    }

    const XYZ_32 delta = XYZ_32_Subtract(lara_item->pos, item->pos);
    const XYZ_32 rotated = XYZ_32_Rotate(delta, (XYZ_16) { .y = -item->rot.y });
    return rotated.x >= item_bounds->min.x - radius
        && rotated.x <= item_bounds->max.x + radius
        && rotated.z >= item_bounds->min.z - radius
        && rotated.z <= item_bounds->max.z + radius;
}

static void M_SpawnBlood(const ITEM *const lara_item, const int32_t y_pos)
{
    if (g_Config.debug.enable_invulnerability) {
        return;
    }

    const int16_t angle = Random_GetControl() * 2;
    const int16_t speed = (Random_GetControl() & 3) + 2;
    const XYZ_32 pos = {
        .z = lara_item->pos.z + (Random_GetControl() & 0x3F) - (STEP_L / 8),
        .y = y_pos + (Random_GetControl() & 0x1F) - (STEP_L / 16),
        .x = lara_item->pos.x + (Random_GetControl() & 0x3F) - (STEP_L / 8),
    };
    Spawn_Blood(pos.x, pos.y, pos.z, speed, angle, lara_item->room_num);
}

static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    ITEM *const item = Item_Get(item_num);
    if (!item->is_visible) {
        return;
    }

    if (!Lara_TestBoundsCollide(item, coll->radius)) {
        return;
    }

    for (int32_t i = 0; i < M_NUM_BOUNDS; i++) {
        if (M_ShouldPush(item, lara_item, &m_Bounds[i], coll->radius)) {
            Lara_Col_ItemPush(item, coll, false, true);
        }
    }

    if (!Item_IsTriggerActiveRO(item)) {
        return;
    }

    int32_t touch_bits = Collide_TestCollision(item, lara_item);
    int32_t deadly_bits = touch_bits & M_DEADLY_BITS;
    if (deadly_bits == 0) {
        return;
    }

    SPHERE spheres[34] = {};
    Collide_GetSpheres(item, spheres, true);
    const SPHERE *sphere = &spheres[0];
    const M_PRIV *const p = item->priv;

    while (deadly_bits != 0) {
        if ((deadly_bits & 1) == 0) {
            goto loop_end;
        }

        const XYZ_32 lara_pos = lara_item->pos;
        const BOUNDS_16 bounds = {
            .min.x = sphere->pos.x - item->pos.x - sphere->r,
            .max.x = sphere->pos.x - item->pos.x + sphere->r,
            .min.y = sphere->pos.y - item->pos.y - sphere->r,
            .max.y = sphere->pos.y - item->pos.y + sphere->r,
            .min.z = sphere->pos.z - item->pos.z - sphere->r,
            .max.z = sphere->pos.z - item->pos.z + sphere->r,
        };
        if (!Lara_Col_ItemPushEx(
                item, coll, (touch_bits & 1) != 0, true, &bounds)) {
            goto loop_end;
        }

        if ((touch_bits & 1) != 0
            && !XYZ_32_AreEquivalent(lara_pos, lara_item->pos)) {
            M_SpawnBlood(lara_item, sphere->pos.y);
            Lara_TakeDamage(p->damage, false);
        }

    loop_end:
        deadly_bits >>= 1;
        touch_bits >>= 1;
        sphere++;
    }
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (Item_IsTriggerActive(item)) {
        Sound_Effect(SFX_BLADE_RING, &item->pos, SPM_NORMAL);
        Item_Animate(item);
    }
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->collision_func = M_Collision;
    obj->control_func = M_Control;

    obj->priv_size = sizeof(M_PRIV);
    obj->save_anim = true;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, damage, M_DEFAULT_DAMAGE,
            "Damage dealt when Lara touches the blades."));
}

REGISTER_OBJECT(O_BLADE_RING, M_Setup)
