#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/game/collision.h>
#include <trx/game/fx/debris.h>
#include <trx/game/gun.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_MAX_SPHERES  34
#define M_BURN_TIME    (LOGIC_FPS * 5)
#define M_DEBRIS       (-64)
#define M_REACH_XZ     (WALL_L * 4)
#define M_REACH_Y      (WALL_L * 2)
// clang-format on

typedef struct {
    bool triggers_on_light;
    bool is_lit;
    int32_t timer;
    // Counts in half meshes while the fire spreads, and in whole meshes
    // once the rope starts to break.
    int32_t first;
    int32_t last;
    int32_t start;
} M_PRIV;

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "is_lit", &p->is_lit));
    MUST(JSON_READ_OPT(io, "timer", &p->timer));
    MUST(JSON_READ_OPT(io, "first", &p->first));
    MUST(JSON_READ_OPT(io, "last", &p->last));
    MUST(JSON_READ_OPT(io, "start", &p->start));
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "is_lit", p->is_lit);
    JSONW_WRITE(io, "timer", p->timer);
    JSONW_WRITE(io, "first", p->first);
    JSONW_WRITE(io, "last", p->last);
    JSONW_WRITE(io, "start", p->start);
}

static void M_TriggerFlame(const XYZ_32 pos)
{
    SPARK *const spark =
        Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION, SPARK_CONTEXT_FIRE);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 255;
    spark->src_color.g = (Random_GetControl() & 0x1F) + 48;
    spark->src_color.b = 48;
    spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
    spark->dst_color.b = 32;
    spark->col_fade_speed = (Random_GetControl() & 3) + 4;
    spark->fade_to_black = 4;
    spark->life = (Random_GetControl() & 3) + 24;
    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = (Random_GetControl() & 0xF) + pos.x - 8;
    spark->pos.y = pos.y;
    spark->pos.z = (Random_GetControl() & 0xF) + pos.z - 8;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;
    spark->friction = 5;
    spark->flags =
        SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
    // The original game uses this roll to keep one flame in four in a full
    // spark pool. The pool of TRX picks the sparks to drop by its own rule,
    // and the roll only keeps the random sequence the same.
    Random_GetControl();
    spark->rot_angle = Random_GetControl() & 0xFFF;
    if ((Random_GetControl() & 1) != 0) {
        spark->rot_add = -16 - (Random_GetControl() & 0xF);
    } else {
        spark->rot_add = (Random_GetControl() & 0xF) + 16;
    }
    if ((Random_GetControl() & 0xF) != 0) {
        spark->vel.y = -24 - (Random_GetControl() & 0xF);
        spark->gravity = -24 - (Random_GetControl() & 0x1F);
        spark->max_y_vel = -16 - (Random_GetControl() & 7);
    } else {
        spark->vel.y = (Random_GetControl() & 0xF) + 24;
        spark->gravity = (Random_GetControl() & 0x1F) + 24;
        spark->max_y_vel = 0;
    }
    spark->scalar = 2;
    const uint8_t size = (Random_GetControl() & 0xF) + 96;
    spark->size.width = size;
    spark->size.height = size;
    spark->src_size = spark->size;
    spark->dst_size.width = size >> 3;
    spark->dst_size.height = size >> 3;
    Sparks_FinishSetup(spark);
}

static void M_Light(const int16_t item_num, const int32_t mesh_idx)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    if (p->triggers_on_light) {
        Sound_Effect(SFX_ROLLING_BALL_FALL, nullptr, SPM_NORMAL);
        Room_TestTriggersEx(item, true);
    }

    p->is_lit = true;
    p->first = mesh_idx << 1;
    p->last = mesh_idx << 1;
    p->start = mesh_idx;
    p->timer = M_BURN_TIME;
    item->trigger.mask = TRIGGER_MASK_ALL;
    item->trigger.reversed = false;
    item->timer = 0;
    Item_AddSimulated(item_num);
}

// Catches fire at the sphere that the flame of Lara's torch touches.
static void M_Collision(
    const int16_t item_num, ITEM *const lara_item, COLL_INFO *const coll)
{
    const ITEM *const item = Item_Get(item_num);
    const M_PRIV *const p = item->priv;
    if (p->is_lit) {
        return;
    }

    const XYZ_32 diff = XYZ_32_Subtract(item->pos, lara_item->pos);
    if (ABS(diff.x) > M_REACH_XZ || ABS(diff.y) > M_REACH_Y
        || ABS(diff.z) > M_REACH_XZ) {
        return;
    }

    XYZ_32 flame_pos;
    if (!Gun_Torch_GetFlamePos(&flame_pos)) {
        return;
    }

    SPHERE spheres[M_MAX_SPHERES];
    const int32_t count = Collide_GetSpheres(item, spheres, true);
    for (int32_t i = 0; i < count; i++) {
        const SPHERE *const sphere = &spheres[i];
        if (ABS(sphere->pos.x - flame_pos.x) < sphere->r
            && ABS(sphere->pos.y - flame_pos.y) < sphere->r
            && ABS(sphere->pos.z - flame_pos.z) < sphere->r) {
            M_Light(item_num, i);
            return;
        }
    }
}

// Lights the burning part of the rope. Every second flame sits halfway
// between two joints, and the two kinds take turns.
static void M_AddFlames(
    const ITEM *const item, const M_PRIV *const p, const int32_t counter)
{
    SPHERE spheres[M_MAX_SPHERES] = {};
    Collide_GetSpheres(item, spheres, true);
    const SPHERE *sphere = &spheres[p->first >> 1];
    for (int32_t i = p->first; i < p->last; i++) {
        if (Item_IsMeshVisible(item, i >> 1)) {
            if ((i & 1) != 0 && (counter & 4) != 0) {
                const XYZ_32 pos = {
                    .x = (sphere[0].pos.x + sphere[1].pos.x) >> 1,
                    .y = (sphere[0].pos.y + sphere[1].pos.y) >> 1,
                    .z = (sphere[0].pos.z + sphere[1].pos.z) >> 1,
                };
                M_TriggerFlame(pos);
            } else if ((counter & 4) == 0) {
                M_TriggerFlame(sphere->pos);
            }
        }
        if ((i & 1) != 0) {
            sphere++;
        }
    }
}

// Breaks the rope one mesh further each way from where it caught fire.
// Returns whether no mesh is left to break.
static bool M_BreakFurther(ITEM *const item, M_PRIV *const p)
{
    const int32_t last_mesh = Object_Get(item->object_id)->mesh_count - 1;
    bool is_done = true;
    if (p->first > 0) {
        p->first--;
        FX_Debris_ShatterItemMesh(item, p->first, M_DEBRIS, 0, true);
        is_done = false;
    }
    if (p->last < last_mesh) {
        p->last++;
        FX_Debris_ShatterItemMesh(item, p->last, M_DEBRIS, 0, true);
        is_done = false;
    }
    return is_done;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    M_PRIV *const p = item->priv;
    const int32_t last_mesh = Object_Get(item->object_id)->mesh_count - 1;
    const int32_t counter = (int32_t)Output_GetTimeInGame();
    if ((counter & 3) == 0) {
        M_AddFlames(item, p, counter);
        if (p->timer != 0) {
            if (p->first > 1) {
                p->first--;
            }
            if (p->last < last_mesh << 1) {
                p->last++;
            }
        }
    }

    if (p->timer != 0) {
        p->timer--;
        if (p->timer == 0) {
            p->first = p->start;
            p->last = p->start;
            FX_Debris_ShatterItemMesh(item, p->start, M_DEBRIS, 0, true);
        }
    } else if ((counter & 1) != 0 && M_BreakFurther(item, p)) {
        if (!p->triggers_on_light) {
            Room_TestTriggersEx(item, true);
        }
        Item_Destroy(item_num);
    }
}

static void M_Setup(OBJECT *const obj)
{
    obj->control_func = M_Control;
    obj->collision_func = M_Collision;
    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, triggers_on_light, false,
            "Whether the rope sets off its triggers, with the sound of a "
            "falling boulder, as soon as it catches fire, rather than once it "
            "has burnt away."));
}

REGISTER_OBJECT(O_FIRE_ROPE, M_Setup)
