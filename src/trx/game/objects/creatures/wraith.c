#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math/geom.h>
#include <trx/core/utils.h>
#include <trx/game/fx/shockwave.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/matrix.h>
#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

// clang-format off
#define M_SPEED              64
#define M_MIN_SPEED          32
#define M_SLOW_DOWN          12
#define M_TRAIL_POINTS       8
#define M_TRAIL_TURN         1092
#define M_TRAIL_HALF_WIDTH   4.0f
#define M_TRAIL_FADE_START   (12 * WALL_L)
#define M_TRAIL_FADE_END     (20 * WALL_L)
#define M_REACH              28900
#define M_REACH_HEIGHT       256
#define M_TARGET_HEIGHT      384
#define M_CHARGE_STEP        400
#define M_CHARGE_BURN        8000
#define M_TRAP_CONTACT       25
#define M_PARTNER_CONTACT    10
#define M_LIGHT_FALLOFF      16
#define M_EFFECT_SPARKS      15
// clang-format on

typedef struct {
    XYZ_32 pos;
    XYZ_32 prev_pos;
    XYZ_16 vel;
    RGB_888 color;
} M_TRAIL_POINT;

typedef struct {
    // The item the wraith flies at, or NO_ITEM for Lara.
    int16_t target_num;
    int16_t turn_x;
    int16_t turn_y;
    // Grows while the fire wraith touches Lara and sets her on fire, and goes
    // negative under water.
    int16_t charge;
    // Counts the ticks the third wraith has touched its trap. A paired wraith
    // takes a value from its partner on contact, which runs down while the
    // two are apart; a contact while it is above zero destroys both.
    int16_t contact;
    int32_t flip_group;
    M_TRAIL_POINT trail[M_TRAIL_POINTS];
} M_PRIV;

static int32_t M_GetViewDepth(const XYZ_32 pos)
{
    // clang-format off
    return (int32_t)((
        g_ViewMatrix._20 * pos.x +
        g_ViewMatrix._21 * pos.y +
        g_ViewMatrix._22 * pos.z +
        g_ViewMatrix._23) >> W2V_SHIFT);
    // clang-format on
}

static void M_ResetTrail(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    for (int32_t i = 0; i < M_TRAIL_POINTS; i++) {
        p->trail[i] = (M_TRAIL_POINT) {
            .pos = item->pos,
            .prev_pos = item->pos,
        };
    }
}

static RGB_888 M_GetSparkColor(const OBJECT_ID object_id, const bool is_end)
{
    if (object_id == O_WRAITH_1) {
        const uint8_t r = (Random_GetControl() & 0x1F) + 128;
        return (RGB_888) {
            .r = r,
            .g = (Random_GetControl() & 0x1F) + (is_end ? 64 : 48),
            .b = 24,
        };
    } else if (object_id == O_WRAITH_2) {
        const uint8_t b = (Random_GetControl() & 0x1F) + 128;
        return (RGB_888) {
            .r = 24,
            .g = (Random_GetControl() & 0x1F) + (is_end ? 64 : 48),
            .b = b,
        };
    }
    const uint8_t shade = (Random_GetControl() & 0xF) + 64;
    return (RGB_888) { .r = shade, .g = shade, .b = shade };
}

static void M_TriggerFlame(
    const ITEM *const item, const XYZ_32 pos, const XYZ_16 vel)
{
    SPARK *const spark =
        Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION, SPARK_CONTEXT_FIRE);
    if (spark == nullptr) {
        return;
    }

    spark->src_color = M_GetSparkColor(item->object_id, false);
    if (item->object_id == O_WRAITH_1 || item->object_id == O_WRAITH_2) {
        spark->dst_color = M_GetSparkColor(item->object_id, true);
    } else {
        spark->dst_color = spark->src_color;
        spark->dst_color.b += Random_GetControl() & 0xF;
    }
    spark->col_fade_speed = 4;
    spark->fade_to_black = 7;
    spark->life = (Random_GetControl() & 7) + 12;
    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
    spark->friction = 85;
    spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->vel.x = vel.x;
    spark->vel.y = vel.y;
    spark->vel.z = vel.z;
    spark->gravity = 0;
    spark->max_y_vel = 0;
    spark->scalar = 2;
    spark->src_size.width = (Random_GetControl() & 0x1F) + 48;
    spark->src_size.height = spark->src_size.width;
    spark->size = spark->src_size;
    spark->dst_size.width = 2;
    spark->dst_size.height = 2;
    Sparks_FinishSetup(spark);
}

// Bursts the wraith's sparks out sideways as it passes through a wall.
static void M_TriggerEffect(
    const ITEM *const item, const XYZ_32 pos, const int16_t angle)
{
    // The original game draws random numbers for the start colours and then
    // shifts them out, so the draws stay to keep the same random sequence.
    RGB_888 src;
    RGB_888 dst;
    if (item->object_id == O_WRAITH_1) {
        Random_GetControl();
        Random_GetControl();
        src = (RGB_888) { .r = 0, .g = 0, .b = 24 };
        dst.g = (Random_GetControl() & 0x1F) + 48;
        dst.r = (Random_GetControl() & 0x1F) + 128;
        dst.b = 24;
    } else if (item->object_id == O_WRAITH_2) {
        const int32_t r1 = Random_GetControl();
        Random_GetControl();
        src = (RGB_888) { .r = 0, .g = 0, .b = (r1 & 0x1F) + 128 };
        dst.g = (Random_GetControl() & 0x1F) + 48;
        dst.b = (Random_GetControl() & 0x1F) + 128;
        dst.r = 24;
    } else {
        const uint8_t shade = (Random_GetControl() & 0xF) + 64;
        src = (RGB_888) { .r = shade, .g = shade, .b = shade };
        dst = (RGB_888) {
            .r = shade,
            .g = shade,
            .b = shade + (Random_GetControl() & 0xF),
        };
    }

    for (int32_t i = 0; i < M_EFFECT_SPARKS; i++) {
        SPARK *const spark = Sparks_InitialiseSpriteSpark(
            SPARK_TYPE_EXPLOSION, SPARK_CONTEXT_FIRE);
        if (spark == nullptr) {
            continue;
        }

        spark->src_color = src;
        spark->dst_color = dst;
        spark->col_fade_speed = 4;
        spark->fade_to_black = 7;
        spark->life = (Random_GetControl() & 7) + 32;
        spark->s_life = spark->life;
        spark->draw_type = DRAW_BLEND_ADD;
        spark->pos.x = pos.x + (Random_GetControl() & 0x1F) - 16;
        spark->pos.y = pos.y + (Random_GetControl() & 0x1F) - 16;
        spark->pos.z = pos.z + (Random_GetControl() & 0x1F) - 16;
        const int16_t spread = Random_GetControl() - DEG_90;
        const int32_t radius = (Random_GetControl() & 0x3FF) + WALL_L;
        const int16_t heading = spread + angle;
        spark->vel.x = (radius * Math_Sin(heading)) >> W2V_SHIFT;
        spark->vel.y = (Random_GetControl() & 0x7F) - 64;
        spark->vel.z = (radius * Math_Cos(heading)) >> W2V_SHIFT;
        spark->friction = 4;
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
        spark->gravity = (Random_GetControl() & 0x7F) - 64;
        spark->max_y_vel = 0;
        spark->scalar = 3;
        spark->src_size.width = (Random_GetControl() & 0x1F) + 48;
        spark->src_size.height = spark->src_size.width;
        spark->size = spark->src_size;
        spark->dst_size.width = spark->src_size.width >> 2;
        spark->dst_size.height = spark->dst_size.width;
        Sparks_FinishSetup(spark);
    }
}

static void M_TriggerShockwaves(
    const XYZ_32 pos, const RGB_888 color, const int16_t speed)
{
    const bool shrinks = speed < 0;
    for (int32_t i = 0; i < 4; i++) {
        FX_Shockwave_Create(&(FX_SHOCKWAVE_SETUP) {
            .pos = { .x = pos.x, .y = pos.y - M_TARGET_HEIGHT, .z = pos.z },
            .inner_radius = shrinks ? 640 : 32,
            .outer_radius = shrinks ? 512 : 160,
            .speed = speed,
            .x_rot = (int16_t)(i * DEG_45),
            .color = color,
            .life = 24,
        });
    }
}

// Pairs the wraith with an active wraith of the other element, or the third
// wraith with an active trap, in the room it has just entered.
static void M_PairInRoom(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    const ROOM *const room = Room_Get(item->room_num);
    for (int16_t other_num = room->item_num; other_num != NO_ITEM;
         other_num = Item_Get(other_num)->next_item) {
        ITEM *const other = Item_Get(other_num);
        if (!other->is_simulated) {
            continue;
        }

        const OBJECT_ID a = item->object_id;
        const OBJECT_ID b = other->object_id;
        if ((a == O_WRAITH_1 && b == O_WRAITH_2)
            || (a == O_WRAITH_2 && b == O_WRAITH_1)) {
            M_PRIV *const other_p = other->priv;
            p->target_num = other_num;
            other_p->target_num = item_num;
            return;
        }
        if (a == O_WRAITH_3 && b == O_ANIMATING_10) {
            p->target_num = other_num;
            return;
        }
    }
}

// Turns one axis towards the target, faster the longer it keeps turning the
// same way.
static void M_Turn(
    const int16_t delta, const int16_t speed, int16_t *const turn,
    int16_t *const rot)
{
    if (ABS(delta) < *turn && (delta > 0) == (*turn > 0)) {
        *rot += delta;
    } else if (delta < 0) {
        if (*turn < 0) {
            *turn -= speed;
            *rot += *turn;
        } else {
            *turn = -1;
        }
    } else if (*turn > 0) {
        *turn += speed;
        *rot += *turn;
    } else {
        *turn = 1;
    }
}

static void M_UpdateTrail(ITEM *const item)
{
    M_PRIV *const p = item->priv;
    for (int32_t i = 0; i < M_TRAIL_POINTS; i++) {
        p->trail[i].prev_pos = p->trail[i].pos;
    }

    for (int32_t i = M_TRAIL_POINTS - 1; i > 0; i--) {
        M_TRAIL_POINT *const ahead = &p->trail[i - 1];
        ahead->pos.x += ahead->vel.x >> 4;
        ahead->pos.y += ahead->vel.y >> 4;
        ahead->pos.z += ahead->vel.z >> 4;
        ahead->vel.x -= ahead->vel.x >> 4;
        ahead->vel.y -= ahead->vel.y >> 4;
        ahead->vel.z -= ahead->vel.z >> 4;

        M_TRAIL_POINT *const point = &p->trail[i];
        point->pos = ahead->pos;
        point->vel = ahead->vel;

        const int32_t step = M_TRAIL_POINTS - i;
        RGB_888 *const color = &point->color;
        if (item->object_id == O_WRAITH_1) {
            color->r = (Random_GetControl() & 0x3F) - 64;
            color->g = 16 * step + (Random_GetControl() & 0x3F);
            color->b = Random_GetControl() & 0xF;
        } else if (item->object_id == O_WRAITH_2) {
            color->b = (Random_GetControl() & 0x3F) - 64;
            color->g = 16 * step + (Random_GetControl() & 0x3F);
            color->r = Random_GetControl() & 0xF;
        } else {
            color->r = 8 * (step + 1) + (Random_GetControl() & 0x3F);
            color->g = color->r;
            color->b = color->r + (Random_GetControl() & 0xF);
        }
    }
}

static void M_HitTarget(ITEM *const item, ITEM *const target, bool *destroy)
{
    M_PRIV *const p = item->priv;
    const ITEM *const lara_item = Lara_GetItem();
    if (target == lara_item) {
        return;
    }

    if (target->object_id == O_ANIMATING_10) {
        p->contact++;
        if (p->contact <= M_TRAP_CONTACT) {
            return;
        }

        item->pos = (XYZ_32) {
            .x = target->pos.x,
            .y = target->pos.y - M_TARGET_HEIGHT,
            .z = target->pos.z,
        };
        M_TriggerShockwaves(
            item->pos, (RGB_888) { .r = 0x60, .g = 0x60, .b = 0x60 }, -32);
        M_TriggerShockwaves(
            item->pos, (RGB_888) { .r = 0x30, .g = 0x30, .b = 0x30 }, 48);

        // The trap's OCB counts the wraiths it still takes, and its
        // animation restarts while any remain.
        TRX_VALUE ocb = {};
        if (ObjectProperty_GetItemValue(target, "ocb", &ocb)
            && ocb.type == TVT_S32) {
            ocb.as_int--;
            ObjectProperty_SetItemValueRaw(target, "ocb", ocb);
            if (ocb.as_int > 0) {
                target->frame_num = Item_GetAnim(target)->frame_base;
            }
        }
        *destroy = true;
        return;
    }

    M_PRIV *const target_p = target->priv;
    target_p->contact = M_PARTNER_CONTACT;
    if (p->contact > 0) {
        Sparks_TriggerExplosionSparks(item->pos, 2, -2, 1, item->room_num);
        target_p->target_num = NO_ITEM;
        Item_Destroy(Item_GetIndex(target));
        *destroy = true;
    }
}

static bool M_IsInsideRoom(const ITEM *const item)
{
    int16_t room_num = item->room_num;
    const SECTOR *const sector = Room_GetSector(item->pos, &room_num);
    return Room_GetHeight(sector, item->pos) >= item->pos.y
        && Room_GetCeiling(sector, item->pos) <= item->pos.y;
}

// Fades the trail with distance.
static RGBA_8888 M_FadeTrailColor(const RGBA_8888 color, const int32_t depth)
{
    const int32_t keep = M_TRAIL_FADE_END - depth;
    return (RGBA_8888) {
        .r = (color.r * keep) >> 13,
        .g = (color.g * keep) >> 13,
        .b = (color.b * keep) >> 13,
        .a = color.a,
    };
}

static void M_DrawTrail(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    const double ratio = Interpolation_GetWorldRate();

    XYZ_32 pos[M_TRAIL_POINTS];
    RGBA_8888 color[M_TRAIL_POINTS];
    for (int32_t i = 0; i < M_TRAIL_POINTS; i++) {
        const M_TRAIL_POINT *const point = &p->trail[i];
        pos[i] = (XYZ_32) {
            .x = (int32_t)LERP(point->prev_pos.x, point->pos.x, ratio),
            .y = (int32_t)LERP(point->prev_pos.y, point->pos.y, ratio),
            .z = (int32_t)LERP(point->prev_pos.z, point->pos.z, ratio),
        };
        const bool is_end = i == 0 || i == M_TRAIL_POINTS - 1;
        color[i] = is_end ? (RGBA_8888) { .a = 0xFF }
                          : (RGBA_8888) {
                                .r = point->color.r,
                                .g = point->color.g,
                                .b = point->color.b,
                                .a = 0xFF,
                            };
    }

    // Draws the trail five times about the head, as the original does. The
    // original turns the trail between draws and undoes only some of the
    // turns, which leaves two copies turned about Y, two in place and one
    // turned about Z.
    const XYZ_16 turns[] = {
        { .y = -M_TRAIL_TURN },
        { .y = -M_TRAIL_TURN },
        {},
        { .z = -M_TRAIL_TURN },
        {},
    };
    for (int32_t i = 0; i < (int32_t)ARRAY_SIZE(turns); i++) {
        XYZ_32 turned[M_TRAIL_POINTS];
        for (int32_t j = 0; j < M_TRAIL_POINTS; j++) {
            const XYZ_32 offset = XYZ_32_Subtract(pos[j], pos[0]);
            turned[j] = XYZ_32_Add(pos[0], XYZ_32_Rotate(offset, turns[i]));
        }

        for (int32_t j = 0; j < M_TRAIL_POINTS - 1; j++) {
            const int32_t depth = M_GetViewDepth(turned[j]);
            if (depth >= M_TRAIL_FADE_END) {
                continue;
            }

            RGBA_8888 from_color = color[j];
            RGBA_8888 to_color = color[j + 1];
            if (depth > M_TRAIL_FADE_START) {
                from_color = M_FadeTrailColor(from_color, depth);
                to_color = M_FadeTrailColor(to_color, depth);
            }
            OutputSource_PolyFX_StageLineSegment(
                turned[j], from_color, turned[j + 1], to_color,
                M_TRAIL_HALF_WIDTH, DRAW_BLEND_ADD);
        }
    }
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "target_num", &p->target_num));
    MUST(JSON_READ_OPT(io, "turn_x", &p->turn_x));
    MUST(JSON_READ_OPT(io, "turn_y", &p->turn_y));
    MUST(JSON_READ_OPT(io, "charge", &p->charge));
    MUST(JSON_READ_OPT(io, "contact", &p->contact));
    MUST(JSON_READ_OPT(io, "flip_group", &p->flip_group));
    M_ResetTrail(item);
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "target_num", p->target_num);
    JSONW_WRITE(io, "turn_x", p->turn_x);
    JSONW_WRITE(io, "turn_y", p->turn_y);
    JSONW_WRITE(io, "charge", p->charge);
    JSONW_WRITE(io, "contact", p->contact);
    JSONW_WRITE(io, "flip_group", p->flip_group);
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    p->target_num = NO_ITEM;
    p->turn_x = 0;
    p->turn_y = 0;
    p->charge = 0;
    p->contact = 0;
    item->speed = M_SPEED;

    TRX_VALUE ocb = {};
    p->flip_group = 0;
    if (ObjectProperty_GetItemValue(item, "ocb", &ocb) && ocb.type == TVT_S32
        && ocb.as_int > 0 && ocb.as_int < MAX_FLIP_MAPS) {
        p->flip_group = ocb.as_int;
    }

    M_ResetTrail(item);
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    M_PRIV *const p = item->priv;
    ITEM *const lara_item = Lara_GetItem();
    bool destroy = false;

    Sound_Effect(SFX_WRAITH_WHISPERS, &item->pos, SPM_NORMAL);

    ITEM *const target =
        p->target_num == NO_ITEM ? lara_item : Item_Get(p->target_num);

    // A wraith paired with another flies at the middle of Lara's room, so the
    // two meet there.
    int32_t dx;
    int32_t dz;
    int32_t y;
    int32_t dy;
    int64_t dist;
    if (target != lara_item && target->object_id != O_ANIMATING_10) {
        const ROOM *const room = Room_Get(lara_item->room_num);
        dx = room->pos.x + room->size.x * WALL_L / 2 - item->pos.x;
        dz = room->pos.z + room->size.z * WALL_L / 2 - item->pos.z;
        dist = SQUARE((int64_t)dx) + SQUARE((int64_t)dz);
        dy = (int32_t)ABS((dist >> 13) - 768);
        y = room->pos.y + ((room->min_floor - room->max_ceiling) >> 1);
    } else {
        dx = target->pos.x - item->pos.x;
        dz = target->pos.z - item->pos.z;
        dist = SQUARE((int64_t)dx) + SQUARE((int64_t)dz);
        dy = (int32_t)ABS((dist >> 13) - 512);
        y = target->pos.y;
    }
    y = y - item->pos.y - dy - 128;

    const int16_t delta_y = (int16_t)(Math_Atan(dz, dx) - item->rot.y);
    int16_t delta_x;
    if (ABS(dx) > ABS(dz)) {
        delta_x = (int16_t)Math_Atan(ABS(dx) + (ABS(dz) >> 1), y);
    } else {
        delta_x = (int16_t)Math_Atan(ABS(dz) + (ABS(dx) >> 1), y);
    }
    delta_x -= item->rot.x;

    const int16_t turn_speed = (M_SPEED / item->speed) << 3;
    M_Turn(delta_y, turn_speed, &p->turn_y, &item->rot.y);
    M_Turn(delta_x, turn_speed, &p->turn_x, &item->rot.x);

    const bool was_inside = M_IsInsideRoom(item);
    const XYZ_32 old_pos = item->pos;
    item->pos.x += (item->speed * Math_Sin(item->rot.y)) >> W2V_SHIFT;
    item->pos.y += (item->speed * Math_Sin(item->rot.x)) >> W2V_SHIFT;
    item->pos.z += (item->speed * Math_Cos(item->rot.y)) >> W2V_SHIFT;

    int16_t room_num = NO_ROOM;
    Room_GetOutsideStatus(item->pos, nullptr, &room_num);
    if (room_num != NO_ROOM && room_num != item->room_num) {
        Item_UpdateRoom(item_num, room_num);
        M_PairInRoom(item_num);
    }

    if (item->object_id != O_WRAITH_3) {
        if (Room_Get(item->room_num)->flags.underwater) {
            Sparks_TriggerExplosionSparks(item->pos, 2, -2, 1, item->room_num);
            if (p->charge > -1) {
                p->charge = -1;
            } else {
                if (item->object_id == O_WRAITH_2 && p->flip_group != 0
                    && !Room_GetFlipGroupStatus(p->flip_group)) {
                    Room_FlipMap(p->flip_group);
                }
                destroy = true;
            }
        } else if (p->charge < 1) {
            p->charge = 0;
        } else {
            p->charge--;
        }
    }

    if (dist < M_REACH
        && ABS(item->pos.y - target->pos.y + M_TARGET_HEIGHT)
            < M_REACH_HEIGHT) {
        if (item->speed > M_MIN_SPEED) {
            item->speed -= M_SLOW_DOWN;
        }

        if (target == lara_item) {
            Lara_TakeDamage((int16_t)(dist >> 10), false);
            if (item->object_id == O_WRAITH_1) {
                p->charge += M_CHARGE_STEP;
                if (p->charge > M_CHARGE_BURN) {
                    Lara_CatchFire();
                }
            }
        } else {
            M_HitTarget(item, target, &destroy);
        }
    } else if ((((int32_t)Output_GetTimeInGame() * 4) & 0x10) != 0) {
        if (item->speed < M_SPEED) {
            item->speed++;
        }
        if (target != lara_item && target->object_id != O_ANIMATING_10) {
            M_PRIV *const target_p = target->priv;
            if (target_p->contact > 0) {
                target_p->contact--;
            }
        }
    }

    const bool is_inside = M_IsInsideRoom(item);
    if (is_inside && !was_inside) {
        M_TriggerEffect(item, item->pos, item->rot.y);
    } else if (!is_inside && was_inside) {
        M_TriggerEffect(item, old_pos, item->rot.y + DEG_180);
    }

    M_UpdateTrail(item);
    M_TRAIL_POINT *const head = &p->trail[0];
    head->pos = item->pos;
    head->vel = (XYZ_16) {
        .x = (int16_t)((item->pos.x - old_pos.x) << 2),
        .y = (int16_t)((item->pos.y - old_pos.y) << 2),
        .z = (int16_t)((item->pos.z - old_pos.z) << 2),
    };

    M_TriggerFlame(item, item->pos, head->vel);
    const XYZ_32 mid = {
        .x = (item->pos.x + old_pos.x) >> 1,
        .y = (item->pos.y + old_pos.y) >> 1,
        .z = (item->pos.z + old_pos.z) >> 1,
    };
    M_TriggerFlame(item, mid, head->vel);

    const RGB_888 light =
        item->object_id == O_WRAITH_3 ? p->trail[5].color : p->trail[1].color;
    Output_AddDynamicLightRGB(head->pos, M_LIGHT_FALLOFF, light);

    if (destroy) {
        Item_Destroy(item_num);
    }
}

static bool M_Draw(const ITEM *const item)
{
    const bool drawn = Item_DrawAnimating(item);
    M_DrawTrail(item);
    return drawn;
}

static void M_Setup(OBJECT *const obj)
{
    if (!obj->loaded) {
        return;
    }

    obj->initialise_func = M_Initialise;
    obj->control_func = M_Control;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_WRAITH_1, M_Setup)
REGISTER_OBJECT(O_WRAITH_2, M_Setup)
REGISTER_OBJECT(O_WRAITH_3, M_Setup)
REGISTER_OBJECT(O_WRAITH_4, M_Setup)
