#include "willard_internal.h"

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/utils.h>
#include <trx/game/collision.h>
#include <trx/game/creature.h>
#include <trx/game/fx.h>
#include <trx/game/inventory.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/pathing/lot.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>
#include <trx/game/spawn.h>
#include <trx/game/stats.h>

// clang-format off
#define M_HIT_POINTS        200
#define M_TURN              (5 * DEG_1)
#define M_ATTACK_TURN       (2 * DEG_1)
#define M_TOUCH_BITS        0x900000
#define M_BITE_DAMAGE       220
#define M_LUNGE_DAMAGE      (M_BITE_DAMAGE * 2)
#define M_TOUCH_DAMAGE      10
#define M_ATTACK_RANGE      SQUARE(WALL_L * 3 / 2)
#define M_LUNGE_RANGE       SQUARE(WALL_L * 2)
#define M_FIRE_RANGE        SQUARE(WALL_L * 4)
#define M_HP_AFTER_KO       M_HIT_POINTS
#define M_KO_TIME           280
#define M_WALK_ATTACK_FRAME 30
#define M_TURN_180_FRAME    51
#define M_SHOOT_FRAME       40
#define M_CHARGE_FRAME_MAX  16
#define M_PLASMA_X          64
#define M_PLASMA_Y          410
// clang-format on

typedef enum {
    M_STATE_STOP,
    M_STATE_WALK,
    M_STATE_LUNGE,
    M_STATE_BIG_KILL,
    M_STATE_STUNNED,
    M_STATE_KNOCKOUT,
    M_STATE_GET_UP,
    M_STATE_WALK_ATTACK_1,
    M_STATE_WALK_ATTACK_2,
    M_STATE_TURN_180,
    M_STATE_SHOOT,
} M_STATE;

typedef enum {
    M_ANIM_BIG_KILL = 6,
    M_ANIM_STUNNED = 7,
} M_ANIM;

typedef struct {
    XYZ_16 pos;
    RGB_888 sub;
    RGB_888 color;
} M_SHIELD_POINT;

typedef struct {
    XYZ_32 pos;
    XYZ_16 rot;
} M_AI_POINT;

typedef struct {
    bool puzzle_ready;
    uint8_t ring_count;
    int16_t explode_count;
    bool dead;
    int16_t death_count;
    int32_t direction;
    int32_t desired_direction;
    M_SHIELD_POINT shield[5][8];
    int32_t closest_ai_path;
    int32_t lara_ai_path;
    int32_t lara_junction;
    int32_t junction_index[4];
    M_AI_POINT ai_path[16];
    M_AI_POINT ai_junction[4];
} M_PRIV;

static const BITE m_BiteLeft = {
    .pos = { .x = 19, .y = -13, .z = 3 },
    .mesh_num = 20,
};
static const BITE m_BiteRight = {
    .pos = { .x = 19, .y = -13, .z = 3 },
    .mesh_num = 23,
};

static const int32_t m_DDist[5] = { 1600, 5600, 6400, 5600, 1600 };
static const int32_t m_DHeights1[5] = { -7680, -4224, -768, 2688, 6144 };
static const int32_t m_DHeights2[5] = { -1536, -1152, -768, -384, 0 };
static int32_t m_DeathDist[5] = {};
static int32_t m_DeathHeights[5] = {};

static int32_t M_GetDamage(
    const ITEM *const item, const char *const key, const int32_t default_value)
{
    OBJECT_PROPERTY_VALUE damage = {};
    if (ObjectProperty_GetItemValue(item, key, &damage)) {
        return damage.as_int;
    }

    return default_value;
}

static void M_ResetPriv(M_PRIV *const p)
{
    *p = (M_PRIV) {};
    p->closest_ai_path = -1;
    p->lara_ai_path = -1;
    p->lara_junction = -1;
    p->direction = 1;
    p->desired_direction = 1;
}

static void M_LoadShieldPoint(
    JSON_READ_IO *const io, M_SHIELD_POINT *const point)
{
    JSON_SHOULD(JSON_READ(io, "pos", &point->pos));
    JSON_SHOULD(JSON_READ(io, "sub", &point->sub));
    JSON_SHOULD(JSON_READ(io, "color", &point->color));
}

static void M_SaveShieldPoint(
    JSON_WRITE_IO *const io, const M_SHIELD_POINT *const point)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "pos", point->pos);
    JSONW_WRITE(io, "sub", point->sub);
    JSONW_WRITE(io, "color", point->color);
    JSONW_POP_AND_APPEND(io);
}

static void M_LoadAIPoint(JSON_READ_IO *const io, M_AI_POINT *const point)
{
    JSON_SHOULD(JSON_READ(io, "pos", &point->pos));
    JSON_SHOULD(JSON_READ(io, "rot", &point->rot));
}

static void M_SaveAIPoint(
    JSON_WRITE_IO *const io, const M_AI_POINT *const point)
{
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "pos", point->pos);
    JSONW_WRITE(io, "rot", point->rot);
    JSONW_POP_AND_APPEND(io);
}

static void M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    M_ResetPriv(p);
    JSON_SHOULD(JSON_READ(io, "puzzle_ready", &p->puzzle_ready));
    JSON_SHOULD(JSON_READ(io, "ring_count", &p->ring_count));
    JSON_SHOULD(JSON_READ(io, "explode_count", &p->explode_count));
    JSON_SHOULD(JSON_READ(io, "dead", &p->dead));
    JSON_SHOULD(JSON_READ(io, "death_count", &p->death_count));
    JSON_SHOULD(JSON_READ(io, "direction", &p->direction));
    JSON_SHOULD(JSON_READ(io, "desired_direction", &p->desired_direction));
    JSON_SHOULD(JSON_READ(io, "closest_ai_path", &p->closest_ai_path));
    JSON_SHOULD(JSON_READ(io, "lara_ai_path", &p->lara_ai_path));
    JSON_SHOULD(JSON_READ(io, "lara_junction", &p->lara_junction));

    if (JSON_SHOULD(JSON_PUSH(io, "shield"))) {
        for (int32_t i = 0; i < 5; i++) {
            if (!JSON_SHOULD(JSON_PUSH_INDEX(io, i))) {
                continue;
            }
            for (int32_t j = 0; j < 8; j++) {
                if (!JSON_SHOULD(JSON_PUSH_INDEX(io, j))) {
                    continue;
                }
                M_LoadShieldPoint(io, &p->shield[i][j]);
                JSON_POP(io);
            }
            JSON_POP(io);
        }
        JSON_POP(io);
    }

    if (JSON_SHOULD(JSON_PUSH(io, "junction_index"))) {
        for (int32_t i = 0; i < 4; i++) {
            JSON_SHOULD(JSON_READ_A(io, i, &p->junction_index[i]));
        }
        JSON_POP(io);
    }

    if (JSON_SHOULD(JSON_PUSH(io, "ai_path"))) {
        for (int32_t i = 0; i < 16; i++) {
            if (!JSON_SHOULD(JSON_PUSH_INDEX(io, i))) {
                continue;
            }
            M_LoadAIPoint(io, &p->ai_path[i]);
            JSON_POP(io);
        }
        JSON_POP(io);
    }

    if (JSON_SHOULD(JSON_PUSH(io, "ai_junction"))) {
        for (int32_t i = 0; i < 4; i++) {
            if (!JSON_SHOULD(JSON_PUSH_INDEX(io, i))) {
                continue;
            }
            M_LoadAIPoint(io, &p->ai_junction[i]);
            JSON_POP(io);
        }
        JSON_POP(io);
    }
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "puzzle_ready", p->puzzle_ready);
    JSONW_WRITE(io, "ring_count", p->ring_count);
    JSONW_WRITE(io, "explode_count", p->explode_count);
    JSONW_WRITE(io, "dead", p->dead);
    JSONW_WRITE(io, "death_count", p->death_count);
    JSONW_WRITE(io, "direction", p->direction);
    JSONW_WRITE(io, "desired_direction", p->desired_direction);
    JSONW_WRITE(io, "closest_ai_path", p->closest_ai_path);
    JSONW_WRITE(io, "lara_ai_path", p->lara_ai_path);
    JSONW_WRITE(io, "lara_junction", p->lara_junction);

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < 5; i++) {
        JSONW_PUSH_ARRAY(io);
        for (int32_t j = 0; j < 8; j++) {
            M_SaveShieldPoint(io, &p->shield[i][j]);
        }
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "shield");

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < 4; i++) {
        JSONW_PUSH_VALUE(io, p->junction_index[i]);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "junction_index");

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < 16; i++) {
        M_SaveAIPoint(io, &p->ai_path[i]);
    }
    JSONW_POP_AND_SET(io, "ai_path");

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < 4; i++) {
        M_SaveAIPoint(io, &p->ai_junction[i]);
    }
    JSONW_POP_AND_SET(io, "ai_junction");
}

static void M_TriggerPlasma(
    const int16_t item_num, const int32_t node, int32_t size)
{
    const ITEM *const item = Item_Get(item_num);
    const ITEM *const lara_item = Lara_GetItem();

    const int32_t dx = lara_item->pos.x - item->pos.x;
    const int32_t dz = lara_item->pos.z - item->pos.z;
    const int32_t max_dist = 16 * WALL_L;
    if (dx < -max_dist || dx > max_dist || dz < -max_dist || dz > max_dist) {
        return;
    }

    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    spark->src_color.r = 48;
    spark->src_color.g = 255;
    spark->src_color.b = (Random_GetControl() & 0x1F) + 48;
    spark->dst_color.r = 32;
    spark->dst_color.g = (Random_GetControl() & 0x3F) + 192;
    spark->dst_color.b = (Random_GetControl() & 0x3F) + 128;
    spark->fade_to_black = 8;
    spark->col_fade_speed = (Random_GetControl() & 3) + 12;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->life = (Random_GetControl() & 7) + 24;
    spark->s_life = spark->life;
    spark->dynamic = -1;
    spark->friction = 3;
    spark->pos.x = (Random_GetControl() & 0xF) - 8;
    spark->pos.y = 0;
    spark->pos.z = (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0x1F) - 16;
    spark->vel.y = (Random_GetControl() & 7) + 8;
    spark->vel.z = (Random_GetControl() & 0x1F) - 16;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ATTACHED_NODE | SPARK_F_ALT_SPRITE | SPARK_F_ITEM
            | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_ATTACHED_NODE | SPARK_F_ALT_SPRITE | SPARK_F_ITEM
            | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->gravity = (Random_GetControl() & 7) + 8;
    spark->node_num = (uint8_t)node;
    spark->max_y_vel = (Random_GetControl() & 7) + 16;
    spark->item_num = item_num;
    spark->scalar = 1;
    size += Random_GetControl() & 0xF;
    spark->size.width = (uint8_t)size;
    spark->src_size.width = spark->size.width;
    spark->dst_size.width = spark->size.width >> 2;
    spark->size.height = spark->size.width;
    spark->src_size.height = spark->size.height;
    spark->dst_size.height = spark->size.height >> 2;
    Sparks_FinishSetup(spark);
}

static void M_Explode(ITEM *const item)
{
    M_PRIV *const p = item->priv;

    if (p->explode_count == 1 || p->explode_count == 15
        || p->explode_count == 25 || p->explode_count == 35
        || p->explode_count == 45 || p->explode_count == 55) {
        XYZ_32 pos = {
            .x = item->pos.x + (Random_GetDraw() & 0x3FF) - 512,
            .y = item->pos.y - (Random_GetDraw() & 0x3FF) - 256,
            .z = item->pos.z + (Random_GetDraw() & 0x3FF) - 512,
        };

        FX_RING *const ring =
            FX_Ring_GetRing(FX_RING_TYPE_BLAST, p->ring_count);
        if (ring != nullptr) {
            ring->pos = pos;
            ring->on = 1;
            FX_Ring_Sync(ring);
            p->ring_count++;
        }

        for (int32_t i = 0; i < 24; i += 3) {
            pos = (XYZ_32) {};
            Collide_GetJointAbsPosition(item, &pos, i);
            Willard_TriggerPlasmaBall(
                pos, item->room_num, (int16_t)(Random_GetControl() << 1), 4);
        }

        Sparks_TriggerExplosionSparks(pos, 3, -2, 2, 0);
        Sound_Effect(SFX_BLAST_CIRCLE, &item->pos, 0x800000 | SPM_PITCH);
    }

    for (int32_t i = 0; i < 5; i++) {
        if (p->explode_count < 128) {
            m_DeathDist[i] =
                (m_DDist[i] >> 4) + ((p->explode_count * m_DDist[i]) >> 7);
            m_DeathHeights[i] = m_DHeights2[i]
                + ((p->explode_count * (m_DHeights1[i] - m_DHeights2[i])) >> 7);
        }
    }

    for (int32_t i = 0; i < 5; i++) {
        const int32_t y = m_DeathHeights[i];
        const int32_t dist = m_DeathDist[i];
        const int32_t time4 = Output_GetTimeInGame() * 4;
        int32_t angle = (time4 & 0x3F) << 3;

        for (int32_t j = 0; j < 8; j++) {
            M_SHIELD_POINT *const shield = &p->shield[i][j];
            shield->pos.x = (dist * Math_Sin(angle << 4)) >> 13;
            shield->pos.y = y;
            shield->pos.z = (dist * Math_Cos(angle << 4)) >> 13;
            shield->sub = (RGB_888) { 0, 0, 0 };

            if (i != 0 && i != 4 && p->explode_count < 64) {
                int32_t r = Random_GetDraw() & 0x3F;
                int32_t g = (Random_GetDraw() & 0x1F) + 224;
                int32_t b = (g >> 1) + (Random_GetDraw() & 0x3F);

                const int32_t m = 64 - p->explode_count;
                r = (m * r) >> 6;
                g = (m * g) >> 6;
                b = (m * b) >> 6;

                shield->color = (RGB_888) { r, g, b };
            } else {
                shield->color = COLOR_RGB_888_BLACK;
            }

            angle = (angle + 512) & 0xFFF;
        }
    }
}

static void M_Die(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    Stats_AddKill();
    item->hit_points = 0;
    item->collidable = false;
    Item_Kill(item_num);
    LOT_DisableBaddieAI(item_num);
    item->flags |= IF_INVISIBLE;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    M_ResetPriv(p);
    item->include_in_kill_stats = false;
}

static void M_Control(const int16_t item_num)
{
    const ITEM *const lara_item = Lara_GetItem();

    if (!Creature_Activate(item_num)) {
        return;
    }

    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    CREATURE *const creature = item->creature_data;
    const bool lara_was_alive = lara_item->hit_points > 0;
    XYZ_32 pos;

    if (p->closest_ai_path == -1) {
        int32_t n_junction = 0;
        int32_t n_path = 0;

        for (int32_t i = Room_Get(item->room_num)->item_num; i != NO_ITEM;
             i = Item_Get(i)->next_item) {
            const ITEM *const ai = Item_Get(i);

            if (ai->object_id == O_AI_X1 && n_path < 16) {
                p->ai_path[n_path].pos = ai->pos;
                p->ai_path[n_path].rot = ai->rot;
                n_path++;
            } else if (ai->object_id == O_AI_X2 && n_junction < 4) {
                p->ai_junction[n_junction].pos = ai->pos;
                p->ai_junction[n_junction].rot = ai->rot;
                n_junction++;
            }
        }

        p->closest_ai_path = -1;
        int32_t best_dist = INT32_MAX;
        for (int32_t i = 0; i < 16; i++) {
            const int32_t x = (p->ai_path[i].pos.x - item->pos.x) >> 6;
            const int32_t z = (p->ai_path[i].pos.z - item->pos.z) >> 6;
            const int32_t dist = SQUARE(x) + SQUARE(z);

            if (dist < best_dist) {
                p->closest_ai_path = i;
                best_dist = dist;
            }
        }

        p->lara_ai_path = -1;
        best_dist = INT32_MAX;
        for (int32_t i = 0; i < 16; i++) {
            const int32_t x = (p->ai_path[i].pos.x - lara_item->pos.x) >> 6;
            const int32_t z = (p->ai_path[i].pos.z - lara_item->pos.z) >> 6;
            const int32_t dist = SQUARE(x) + SQUARE(z);

            if (dist < best_dist) {
                p->lara_ai_path = i;
                best_dist = dist;
            }
        }

        for (int32_t j = 0; j < 4; j++) {
            int32_t index = -1;
            best_dist = INT32_MAX;
            for (int32_t i = 0; i < 16; i++) {
                const int32_t x =
                    ABS((p->ai_path[i].pos.x - p->ai_junction[j].pos.x) >> 6);
                const int32_t z =
                    ABS((p->ai_path[i].pos.z - p->ai_junction[j].pos.z) >> 6);
                const int32_t dist = x + (z >> 1);

                if (dist < best_dist) {
                    index = i;
                    best_dist = dist;
                }
            }

            p->junction_index[j] = index;
        }
    }

    int32_t best_dist = INT32_MAX;
    int32_t j = p->closest_ai_path;
    for (int32_t i = j - 1; i < j + 2; i++) {
        int32_t n_path;
        if (i < 0) {
            n_path = i + 16;
        } else if (i > 15) {
            n_path = i - 16;
        } else {
            n_path = i;
        }

        const int32_t x = (p->ai_path[n_path].pos.x - item->pos.x) >> 6;
        const int32_t z = (p->ai_path[n_path].pos.z - item->pos.z) >> 6;
        const int32_t dist = SQUARE(x) + SQUARE(z);

        if (dist < best_dist) {
            p->closest_ai_path = n_path;
            best_dist = dist;
        }
    }

    j = p->lara_ai_path;
    best_dist = INT32_MAX;
    for (int32_t i = j - 1; i < j + 2; i++) {
        int32_t n_path;
        if (i < 0) {
            n_path = i + 16;
        } else if (i > 15) {
            n_path = i - 16;
        } else {
            n_path = i;
        }

        const int32_t x = (p->ai_path[n_path].pos.x - lara_item->pos.x) >> 6;
        const int32_t z = (p->ai_path[n_path].pos.z - lara_item->pos.z) >> 6;
        const int32_t dist = SQUARE(x) + SQUARE(z);

        if (dist < best_dist) {
            p->lara_ai_path = n_path;
            best_dist = dist;
        }
    }

    int32_t best_dist2 = INT32_MAX;
    for (int32_t i = 0; i < 4; i++) {
        const int32_t x = (p->ai_junction[i].pos.x - lara_item->pos.x) >> 6;
        const int32_t z = (p->ai_junction[i].pos.z - lara_item->pos.z) >> 6;
        const int32_t dist = SQUARE(x) + SQUARE(z);

        if (dist < best_dist2) {
            p->lara_junction = i;
            best_dist2 = dist;
        }
    }

    const bool fire =
        best_dist2 < best_dist || item->pos.y > lara_item->pos.y + 2048;
    const int32_t x = p->ai_junction[p->lara_junction].pos.x - item->pos.x;
    const int32_t z = p->ai_junction[p->lara_junction].pos.z - item->pos.z;
    const int32_t dist = SQUARE(x) + SQUARE(z);

    if (item->hit_points <= 0) {
        const bool puzzle_complete = Inv_RequestItem(O_QUEST_ITEM_1) > 0
            && Inv_RequestItem(O_QUEST_ITEM_2) > 0
            && Inv_RequestItem(O_QUEST_ITEM_3) > 0
            && Inv_RequestItem(O_QUEST_ITEM_4) > 0;

        if (puzzle_complete && p->puzzle_ready) {
            if (item->current_anim_state != M_STATE_STUNNED) {
                Item_SwitchToAnim(item, M_ANIM_STUNNED, 0);
                item->current_anim_state = M_STATE_STUNNED;
            } else if (
                Item_GetRelativeFrame(item) >= Item_GetAnim(item)->frame_end
                    - Item_GetAnim(item)->frame_base - 2) {
                item->mesh_bits = 0;
                Item_SwitchToAnim(item, Item_GetRelativeAnim(item), -2);

                if (!p->explode_count) {
                    p->ring_count = 0;

                    for (int32_t i = 0; i < 6; i++) {
                        FX_RING *const ring =
                            FX_Ring_GetRing(FX_RING_TYPE_BLAST, i);
                        if (ring == nullptr) {
                            continue;
                        }
                        ring->on = 0;
                        ring->life = 32;
                        ring->radius = 512;
                        ring->speed = (i + 4) << 5;
                        ring->rot.x =
                            ((Random_GetControl() & 0x1FF) - 256) & 0xFFF;
                        ring->rot.z =
                            ((Random_GetControl() & 0x1FF) - 256) & 0xFFF;
                        FX_Ring_Sync(ring);
                    }
                }

                if (p->explode_count < 256) {
                    p->explode_count++;
                }

                if (p->explode_count <= 128 || p->ring_count != 6
                    || FX_Ring_GetRing(FX_RING_TYPE_BLAST, 5)->life != 0) {
                    M_Explode(item);
                } else {
                    M_Die(item_num);
                    p->dead = true;
                }

                return;
            }
        } else {
            creature->maximum_turn = 0;

            switch (item->current_anim_state) {
            case M_STATE_STOP:
                item->goal_anim_state = M_STATE_STUNNED;
                break;

            case M_STATE_STUNNED:
                p->death_count = M_KO_TIME;
                break;

            case M_STATE_KNOCKOUT:
                p->death_count--;

                if (p->death_count < 0) {
                    item->goal_anim_state = M_STATE_GET_UP;
                }
                break;

            case M_STATE_GET_UP:
                item->hit_points = M_HP_AFTER_KO;

                if (puzzle_complete) {
                    p->puzzle_ready = true;
                }

                creature->maximum_turn = M_ATTACK_TURN;
                break;

            default:
                item->goal_anim_state = M_STATE_STOP;
                break;
            }
        }
    } else {
        AI_INFO info = {};
        Creature_AIInfo(item, &info);

        if (item->touch_bits) {
            Lara_TakeDamage(
                M_GetDamage(item, "touch_damage", M_TOUCH_DAMAGE), false);
        }

        const int32_t index = p->lara_ai_path - p->closest_ai_path;
        if (p->direction == -1 && ((index < 0 && index > -6) || index > 10)) {
            p->desired_direction = 1;
        } else if (
            p->direction == 1 && ((index > 0 && index < 6) || index < -10)) {
            p->desired_direction = -1;
        }

        creature->target = XYZ_32_OffsetYaw(
            p->ai_path[p->closest_ai_path].pos,
            p->ai_path[p->closest_ai_path].rot.y, WALL_L * p->direction);

        switch (item->current_anim_state) {
        case M_STATE_STOP:
            creature->maximum_turn = 0;
            creature->flags = 0;

            if (p->direction != p->desired_direction) {
                item->goal_anim_state = M_STATE_TURN_180;
            } else if (
                fire && info.ahead && dist < M_FIRE_RANGE
                && lara_item->hit_points > 0) {
                item->goal_anim_state = M_STATE_SHOOT;
            } else if (!info.bite || info.distance >= M_LUNGE_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            } else {
                item->goal_anim_state = M_STATE_LUNGE;
            }
            break;

        case M_STATE_WALK:
            creature->maximum_turn = M_TURN;
            creature->flags = 0;

            if (p->direction != p->desired_direction) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (fire && info.ahead && dist < M_FIRE_RANGE) {
                item->goal_anim_state = M_STATE_STOP;
            } else if (info.bite && info.distance < M_ATTACK_RANGE) {
                if ((Random_GetControl() & 3) == 1) {
                    item->goal_anim_state = M_STATE_STOP;
                } else if (
                    item->frame_num
                    >= Item_GetAnim(item)->frame_base + M_WALK_ATTACK_FRAME) {
                    item->goal_anim_state = M_STATE_WALK_ATTACK_1;
                } else {
                    item->goal_anim_state = M_STATE_WALK_ATTACK_2;
                }
            }
            break;

        case M_STATE_LUNGE:
            creature->target.x = lara_item->pos.x;
            creature->target.z = lara_item->pos.z;
            creature->maximum_turn = M_ATTACK_TURN;

            if (!creature->flags && item->touch_bits & M_TOUCH_BITS) {
                Lara_TakeDamage(
                    M_GetDamage(item, "lunge_damage", M_LUNGE_DAMAGE), true);
                Creature_Effect(item, &m_BiteLeft, Spawn_Blood);
                Creature_Effect(item, &m_BiteRight, Spawn_Blood);
                creature->flags = 1;
            }
            break;

        case M_STATE_BIG_KILL:
            switch (Item_GetRelativeFrame(item)) {
            case 0:
            case 43:
            case 95:
            case 105:
                Creature_Effect(item, &m_BiteLeft, Spawn_Blood);
                break;

            case 61:
            case 91:
            case 101:
                Creature_Effect(item, &m_BiteRight, Spawn_Blood);
                break;
            }
            break;

        case M_STATE_WALK_ATTACK_1:
        case M_STATE_WALK_ATTACK_2:
            if (!creature->flags && (item->touch_bits & M_TOUCH_BITS) != 0) {
                Lara_TakeDamage(
                    M_GetDamage(item, "bite_damage", M_BITE_DAMAGE), true);
                Creature_Effect(item, &m_BiteLeft, Spawn_Blood);
                Creature_Effect(item, &m_BiteRight, Spawn_Blood);
                creature->flags = 1;
            }

            if (fire && info.bite && dist < M_FIRE_RANGE) {
                item->goal_anim_state = M_STATE_WALK;
            } else if (info.bite && info.distance < M_ATTACK_RANGE) {
                if (item->current_anim_state == M_STATE_WALK_ATTACK_1) {
                    item->goal_anim_state = M_STATE_WALK_ATTACK_2;
                } else {
                    item->goal_anim_state = M_STATE_WALK_ATTACK_1;
                }
            } else {
                item->goal_anim_state = M_STATE_WALK;
            }
            break;

        case M_STATE_TURN_180:
            creature->maximum_turn = 0;
            creature->flags = 0;

            if (Item_GetRelativeFrame(item) == M_TURN_180_FRAME) {
                item->rot.y += DEG_180;
                p->direction = -p->direction;
            }
            break;

        case M_STATE_SHOOT:
            creature->target.x = lara_item->pos.x;
            creature->target.z = lara_item->pos.z;
            creature->maximum_turn = M_ATTACK_TURN;

            if (Item_GetRelativeFrame(item) == M_SHOOT_FRAME
                && lara_item->hit_points > 0) {
                pos.x = -M_PLASMA_X;
                pos.y = M_PLASMA_Y;
                pos.z = 0;
                Collide_GetJointAbsPosition(item, &pos, 20);
                Willard_TriggerPlasmaBall(
                    pos, item->room_num, item->rot.y - 4096, 0);

                pos.x = M_PLASMA_X;
                pos.y = M_PLASMA_Y;
                pos.z = 0;
                Collide_GetJointAbsPosition(item, &pos, 23);
                Willard_TriggerPlasmaBall(
                    pos, item->room_num, item->rot.y + 4096, 0);
            }

            int32_t f = Item_GetRelativeFrame(item);
            if (f > M_CHARGE_FRAME_MAX) {
                f = Item_GetAnim(item)->frame_end - item->frame_num;
                CLAMPG(f, M_CHARGE_FRAME_MAX);
            }

            pos.x = 0;
            pos.y = 0;
            pos.z = 0;
            Collide_GetJointAbsPosition(item, &pos, 17);

            const int32_t color_base = Random_GetControl();
            const int32_t r = (f * (color_base & 0x3F)) >> 4;
            const int32_t g = (f * (255 - ((color_base >> 4) & 0x1F))) >> 4;
            const int32_t b = (f * (192 - ((color_base >> 6) & 0x1F))) >> 4;

            Output_AddDynamicLightRGB(pos, 12, (RGB_888) { r, g, b });
            M_TriggerPlasma(item_num, 7, f << 2);
            M_TriggerPlasma(item_num, 8, f << 2);
            break;
        }

        if (lara_was_alive && lara_item->hit_points <= 0) {
            Creature_SpecialKill(
                item, M_ANIM_BIG_KILL, M_STATE_BIG_KILL, LS_EXTRA_WILLARD_KILL);
            creature->maximum_turn = 0;
            return;
        }
    }

    const int16_t angle = Creature_Turn(item, creature->maximum_turn);
    Creature_Animate(item_num, angle, 0);
}

static bool M_CanBeExploded(const ITEM *const item)
{
    return false;
}

static void M_DrawShield(const ITEM *const item)
{
    const int32_t sprite_base = Sparks_GetSpriteIndex(SPARK_TYPE_SHIELD);
    if (sprite_base == NO_ITEM) {
        return;
    }

    const M_PRIV *const p = item->priv;
    const int32_t time4 = Output_GetTimeInGame() * 4;

    for (int32_t band = 0; band < 4; band++) {
        const int32_t sprite_idx = sprite_base + ((band + (time4 >> 3)) & 7);

        for (int32_t j = 0; j < 8; j++) {
            const int32_t j2 = (j == 7) ? 0 : (j + 1);
            const M_SHIELD_POINT *const s00 = &p->shield[band][j];
            const M_SHIELD_POINT *const s01 = &p->shield[band][j2];
            const M_SHIELD_POINT *const s10 = &p->shield[band + 1][j];
            const M_SHIELD_POINT *const s11 = &p->shield[band + 1][j2];

            const int32_t idx00 = band * 8 + j;
            const int32_t idx01 = band * 8 + j2;
            const int32_t idx10 = (band + 1) * 8 + j;
            const int32_t idx11 = (band + 1) * 8 + j2;

            RGB_888 c00 = s00->color;
            RGB_888 c01 = s01->color;
            RGB_888 c10 = s10->color;
            RGB_888 c11 = s11->color;

            if (idx00 >= 8 && idx00 <= 31) {
                c00.r = (uint8_t)MAX(0, (int32_t)c00.r - (int32_t)s00->sub.r);
                c00.g = (uint8_t)MAX(0, (int32_t)c00.g - (int32_t)s00->sub.g);
                c00.b = (uint8_t)MAX(0, (int32_t)c00.b - (int32_t)s00->sub.b);
            }
            if (idx01 >= 8 && idx01 <= 31) {
                c01.r = (uint8_t)MAX(0, (int32_t)c01.r - (int32_t)s01->sub.r);
                c01.g = (uint8_t)MAX(0, (int32_t)c01.g - (int32_t)s01->sub.g);
                c01.b = (uint8_t)MAX(0, (int32_t)c01.b - (int32_t)s01->sub.b);
            }
            if (idx10 >= 8 && idx10 <= 31) {
                c10.r = (uint8_t)MAX(0, (int32_t)c10.r - (int32_t)s10->sub.r);
                c10.g = (uint8_t)MAX(0, (int32_t)c10.g - (int32_t)s10->sub.g);
                c10.b = (uint8_t)MAX(0, (int32_t)c10.b - (int32_t)s10->sub.b);
            }
            if (idx11 >= 8 && idx11 <= 31) {
                c11.r = (uint8_t)MAX(0, (int32_t)c11.r - (int32_t)s11->sub.r);
                c11.g = (uint8_t)MAX(0, (int32_t)c11.g - (int32_t)s11->sub.g);
                c11.b = (uint8_t)MAX(0, (int32_t)c11.b - (int32_t)s11->sub.b);
            }

            if (((c00.r | c00.g | c00.b | c01.r | c01.g | c01.b | c11.r | c11.g
                  | c11.b | c10.r | c10.g | c10.b)
                 == 0U)) {
                continue;
            }

            const XYZ_32 world_pos[4] = {
                {
                    item->pos.x + s00->pos.x,
                    item->pos.y + s00->pos.y,
                    item->pos.z + s00->pos.z,
                },
                {
                    item->pos.x + s01->pos.x,
                    item->pos.y + s01->pos.y,
                    item->pos.z + s01->pos.z,
                },
                {
                    item->pos.x + s11->pos.x,
                    item->pos.y + s11->pos.y,
                    item->pos.z + s11->pos.z,
                },
                {
                    item->pos.x + s10->pos.x,
                    item->pos.y + s10->pos.y,
                    item->pos.z + s10->pos.z,
                },
            };
            const RGBA_8888 color[4] = {
                { c00.r, c00.g, c00.b, 255 },
                { c01.r, c01.g, c01.b, 255 },
                { c11.r, c11.g, c11.b, 255 },
                { c10.r, c10.g, c10.b, 255 },
            };
            OutputSource_PolyFX_StageSpriteQuadWorld(
                sprite_idx, world_pos, color, DRAW_BLEND_ADD);
        }
    }
}

static bool M_Draw(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    const bool result = Object_DrawAnimatingItem(item);
    if (p->explode_count != 0) {
        FX_Ring_Draw();

        if (p->explode_count <= 64) {
            M_DrawShield(item);
        }
    }
    return result;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->collision_func = Creature_Collision;
    obj->draw_func = M_Draw;
    obj->can_be_exploded_func = M_CanBeExploded;

    obj->shadow_size = 128;

    obj->pivot_length = 50;
    obj->radius = 102;
    obj->intelligent = true;
    obj->lot_setup = LOT_Setup(LOT_SETUP_CLIMBER);

    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_INT(
            "max_hit_points", M_HIT_POINTS, "Maximum hit points."),
        OBJECT_PROPERTY_INT(
            "touch_damage", M_TOUCH_DAMAGE, "Damage dealt by body contact."),
        OBJECT_PROPERTY_INT(
            "bite_damage", M_BITE_DAMAGE, "Damage dealt by bite attacks."),
        OBJECT_PROPERTY_INT(
            "lunge_damage", M_LUNGE_DAMAGE,
            "Damage dealt by the lunge attack."),
        OBJECT_PROPERTY_INT(
            "plasma_ball_damage", WILLARD_PLASMA_BALL_DAMAGE,
            "Damage dealt by direct plasma ball hits."));
}

REGISTER_OBJECT(O_WILLARD, M_Setup)
