#include <trx/game/objects/general/shoal.h>

#include <trx/core/json/util/read_io.h>
#include <trx/core/json/util/write_io.h>
#include <trx/core/math.h>
#include <trx/core/utils.h>
#include <trx/game/interpolation.h>
#include <trx/game/items.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/output/sources/poly_fx.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/spawn.h>
#include <trx/version.h>

#define M_FISH_PER_SHOAL 24
#define M_DEFAULT_RANGE (XYZ_32) { 1, 1, 1 }

typedef struct {
    int16_t angle;
    uint8_t speed;
    bool on;
    int16_t angle_time;
    int16_t speed_time;
    XYZ_32 range;
} M_LEADER;

typedef struct {
    XYZ_16 pos;
    uint16_t angle; // 0..4095
    int16_t dest_y;
    int8_t ang_add;
    uint8_t speed;
    uint8_t acc;
    uint8_t swim;
    struct {
        struct {
            XYZ_16 pos;
            uint16_t angle; // 0..4095
            uint8_t swim;
        } prev, result;
    } interp;
} M_FISH;

typedef struct {
    M_FISH fish[M_FISH_PER_SHOAL + 1];
    M_LEADER leader;
    XYZ_32 range;
    bool use_room_lighting;
    int32_t piranha_hit_wait;
    int16_t carcass_item_num;
    int32_t sprite_offset;
    GAME_VECTOR anchor;
} M_PRIV;

static void M_ResetFishInterp(M_FISH *const fish)
{
    fish->interp.prev.pos = fish->pos;
    fish->interp.prev.angle = fish->angle;
    fish->interp.prev.swim = fish->swim;
    fish->interp.result.pos = fish->pos;
    fish->interp.result.angle = fish->angle;
    fish->interp.result.swim = fish->swim;
}

static RESULT M_LoadPriv(ITEM *const item, JSON_READ_IO *const io)
{
    M_PRIV *const p = item->priv;
    MUST(JSON_READ_OPT(io, "piranha_hit_wait", &p->piranha_hit_wait));

    if (SHOULD(JSON_PUSH(io, "leader"))) {
        M_LEADER *const leader = &p->leader;
        MUST(JSON_READ_OPT(io, "on", &leader->on));
        MUST(JSON_READ_OPT(io, "angle", &leader->angle));
        MUST(JSON_READ_OPT(io, "speed", &leader->speed));
        MUST(JSON_READ_OPT(io, "angle_time", &leader->angle_time));
        MUST(JSON_READ_OPT(io, "speed_time", &leader->speed_time));
        MUST(JSON_POP(io));
    }

    if (SHOULD(JSON_PUSH(io, "fish"))) {
        const int32_t count = MIN(JSON_ARRAY_LEN(io), M_FISH_PER_SHOAL + 1);
        for (int32_t i = 0; i < count; i++) {
            if (!SHOULD(JSON_PUSH_INDEX(io, i))) {
                break;
            }
            M_FISH *const fish = &p->fish[i];
            MUST(JSON_READ_OPT(io, "pos", &fish->pos));
            MUST(JSON_READ_OPT(io, "angle", &fish->angle));
            MUST(JSON_READ_OPT(io, "dest_y", &fish->dest_y));
            MUST(JSON_READ_OPT(io, "ang_add", &fish->ang_add));
            MUST(JSON_READ_OPT(io, "speed", &fish->speed));
            MUST(JSON_READ_OPT(io, "swim", &fish->swim));
            M_ResetFishInterp(fish);
            MUST(JSON_POP(io));
        }
        MUST(JSON_POP(io));
    }
    return OK;
}

static void M_SavePriv(const ITEM *const item, JSON_WRITE_IO *const io)
{
    const M_PRIV *const p = item->priv;
    JSONW_WRITE(io, "piranha_hit_wait", p->piranha_hit_wait);

    const M_LEADER *const leader = &p->leader;
    JSONW_PUSH_OBJECT(io);
    JSONW_WRITE(io, "on", leader->on);
    JSONW_WRITE(io, "angle", leader->angle);
    JSONW_WRITE(io, "speed", leader->speed);
    JSONW_WRITE(io, "angle_time", leader->angle_time);
    JSONW_WRITE(io, "speed_time", leader->speed_time);
    JSONW_POP_AND_SET(io, "leader");

    JSONW_PUSH_ARRAY(io);
    for (int32_t i = 0; i < M_FISH_PER_SHOAL + 1; i++) {
        const M_FISH *const fish = &p->fish[i];
        JSONW_PUSH_OBJECT(io);
        JSONW_WRITE(io, "pos", fish->pos);
        JSONW_WRITE(io, "angle", fish->angle);
        JSONW_WRITE(io, "dest_y", fish->dest_y);
        JSONW_WRITE(io, "ang_add", fish->ang_add);
        JSONW_WRITE(io, "speed", fish->speed);
        JSONW_WRITE(io, "swim", fish->swim);
        JSONW_POP_AND_APPEND(io);
    }
    JSONW_POP_AND_SET(io, "fish");
}

static uint16_t M_GetFishAngle12(
    const int32_t x1, const int32_t z1, const int32_t x2, const int32_t z2)
{
    const int32_t dx = x2 - x1;
    const int32_t dz = z2 - z1;
    const int32_t fish_angle16 = Math_Atan(dx, dz) - DEG_90;
    return (fish_angle16 >> 4) & 0xFFF; // 0..4095
}

static int32_t M_GetAngle12Diff(const int32_t a, const int32_t b)
{
    int32_t diff = a - b;
    if (diff > 2048) {
        diff -= 4096;
    } else if (diff < -2048) {
        diff += 4096;
    }
    return diff;
}

static bool M_FishNearItem(
    const XYZ_32 *const pos, const int32_t dist, const ITEM *const item)
{
    const int32_t dx = pos->x - item->pos.x;
    const int32_t dy = ABS(pos->y - item->pos.y);
    const int32_t dz = pos->z - item->pos.z;

    // clang-format off
    if (dx < -dist || dx > dist ||
        dz < -dist || dz > dist ||
        dy < -3072 || dy > 3072 ||
        SQUARE(dz) + SQUARE(dx) > SQUARE(dist)
        || dy > dist) {
        return false;
    }
    // clang-format on

    return true;
}

static void M_SetupFish(M_PRIV *const p, const ITEM *const item)
{
    if (p == nullptr || item == nullptr) {
        return;
    }

    M_LEADER *const leader = &p->leader;
    M_FISH *fish = &p->fish[0];

    const int16_t x = leader->range.x;
    const int16_t y = leader->range.y;
    const int16_t z = leader->range.z;

    fish->pos.x = 0;
    fish->pos.y = 0;
    fish->pos.z = 0;
    fish->angle = 0;
    fish->speed = ((Random_GetControl() & 0x3F) + 8);
    fish->swim = (Random_GetControl() & 0x3F);
    M_ResetFishInterp(fish);

    for (int32_t i = 0; i < M_FISH_PER_SHOAL; i++) {
        fish = &p->fish[i + 1];
        fish->pos.x = Random_GetControl() % (x << 1) - x;
        fish->pos.y = Random_GetControl() % y;
        fish->pos.z = Random_GetControl() % (z << 1) - z;
        fish->dest_y = Random_GetControl() % y;
        fish->angle = Random_GetControl() & 0xFFF;
        fish->speed = (Random_GetControl() & 0x1F) + 32;
        fish->swim = Random_GetControl() & 0x3F;
        M_ResetFishInterp(fish);
    }

    leader->on = true;
    leader->angle = 0;
    leader->speed = (Random_GetControl() & 0x7F) + 32;
    leader->angle_time = 0;
    leader->speed_time = 0;
    p->piranha_hit_wait = 0;
}

static void M_FindCarcass(const ITEM *const shoal_item)
{
    M_PRIV *const p = shoal_item->priv;
    p->carcass_item_num = Item_FindTypeInRoom(p->anchor.room_num, O_CARCASS);
}

static bool M_IsTargetable(const ITEM *const item)
{
    return false;
}

static bool M_Trigger(ITEM *const item, const ITEM_TRIGGER *const trigger)
{
    item->timer = 0;

    if (trigger->kind == ITEM_TRIGGER_ANTI) {
        Shoal_TriggerDeactivate(item);
    }

    return true;
}

static void M_Control(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    M_PRIV *const p = item->priv;
    if (p == nullptr) {
        return;
    }

    M_LEADER *const leader = &p->leader;
    if (!leader->on) {
        M_SetupFish(p, item);
    }

    const ITEM *const lara_item = Lara_GetItem();

    int32_t piranha_attack = 0;
    if (item->object_id == O_PIRAHNAS && lara_item != nullptr) {
        if (p->carcass_item_num == NO_ITEM) {
            M_FindCarcass(item);
        }
        if (p->carcass_item_num != NO_ITEM) {
            piranha_attack = 2;
        } else {
            piranha_attack = lara_item->room_num == p->anchor.room_num;
        }
    }

    if (p->piranha_hit_wait != 0) {
        p->piranha_hit_wait--;
    }

    M_FISH *const leader_fish = &p->fish[0];

    const ITEM *enemy = lara_item;
    if (piranha_attack != 0) {
        if (piranha_attack >= 2) {
            enemy = Item_Get(p->carcass_item_num);
        }
        leader_fish->angle = M_GetFishAngle12(
            p->anchor.x + leader_fish->pos.x, p->anchor.z + leader_fish->pos.z,
            enemy->pos.x, enemy->pos.z);
        leader->angle = leader_fish->angle;
        leader->speed = (Random_GetControl() & 0x3F) - 64;
    }

    int32_t diff = M_GetAngle12Diff(leader_fish->angle, leader->angle);
    if (diff > 128) {
        leader_fish->ang_add -= 4;
        CLAMPL(leader_fish->ang_add, -120);
    } else if (diff < -128) {
        leader_fish->ang_add += 4;
        CLAMPG(leader_fish->ang_add, 120);
    } else {
        leader_fish->ang_add -= leader_fish->ang_add >> 2;
        if (ABS(leader_fish->ang_add) < 4) {
            leader_fish->ang_add = 0;
        }
    }
    leader_fish->angle = (leader_fish->angle + leader_fish->ang_add) & 0xFFF;
    if (diff > 1024) {
        leader_fish->angle =
            (leader_fish->angle + (leader_fish->ang_add >> 2)) & 0xFFF;
    }

    diff = (int32_t)leader_fish->speed - (int32_t)leader->speed;
    if (diff < -4) {
        int32_t new_speed =
            (int32_t)leader_fish->speed + (Random_GetControl() & 3) + 1;
        CLAMPL(new_speed, 0);
        leader_fish->speed = new_speed;
    } else if (diff > 4) {
        int32_t new_speed =
            (int32_t)leader_fish->speed - (Random_GetControl() & 3) - 1;
        CLAMPG(new_speed, 255);
        leader_fish->speed = new_speed;
    }

    leader_fish->swim = (leader_fish->swim + (leader_fish->speed >> 4)) & 0x3F;

    // A fish swims with its x mirrored, so the step's x comes off against it.
    const XYZ_32 step = XYZ_32_RotateYaw(
        (XYZ_32) { .z = leader_fish->speed }, leader_fish->angle << 4);
    int32_t x = leader_fish->pos.x - (step.x >> 1);
    int32_t z = leader_fish->pos.z + (step.z >> 1);

    if (piranha_attack == 0) {
        if (z < -leader->range.z) {
            z = -leader->range.z;
            if (leader_fish->angle < 2048) {
                leader->angle =
                    leader_fish->angle - (Random_GetControl() & 0x7F) - 128;
            } else {
                leader->angle =
                    leader_fish->angle + (Random_GetControl() & 0x7F) + 128;
            }
            leader->angle_time = (Random_GetControl() & 0xF) + 8;
            leader->speed_time = 0;
        } else if (z > leader->range.z) {
            z = leader->range.z;
            if (leader_fish->angle > 3072) {
                leader->angle =
                    leader_fish->angle - (Random_GetControl() & 0x7F) - 128;
            } else {
                leader->angle =
                    leader_fish->angle + (Random_GetControl() & 0x7F) + 128;
            }
            leader->angle_time = (Random_GetControl() & 0xF) + 8;
            leader->speed_time = 0;
        }

        if (x < -leader->range.x) {
            x = -leader->range.x;
            if (leader_fish->angle < 1024) {
                leader->angle =
                    leader_fish->angle - (Random_GetControl() & 0x7F) - 128;
            } else {
                leader->angle =
                    leader_fish->angle + (Random_GetControl() & 0x7F) + 128;
            }
            leader->angle_time = (Random_GetControl() & 0xF) + 8;
            leader->speed_time = 0;
        } else if (x > leader->range.x) {
            x = leader->range.x;
            if (leader_fish->angle < 3072) {
                leader->angle =
                    leader_fish->angle - (Random_GetControl() & 0x7F) - 128;
            } else {
                leader->angle =
                    leader_fish->angle + (Random_GetControl() & 0x7F) + 128;
            }
            leader->angle_time = (Random_GetControl() & 0xF) + 8;
            leader->speed_time = 0;
        }

        if ((Random_GetControl() & 0xF) == 0) {
            leader->angle_time = 0;
        }

        if (leader->angle_time != 0) {
            leader->angle_time--;
        } else {
            leader->angle_time = (Random_GetControl() & 0xF) + 8;
            int32_t delta = (Random_GetControl() & 0x3F) - 24;
            if ((Random_GetControl() & 3) == 0) {
                delta *= 32;
            }
            leader->angle = (leader->angle + delta) & 0xFFF;
        }

        if (leader->speed_time != 0) {
            leader->speed_time--;
        } else {
            leader->speed_time = (Random_GetControl() & 0x1F) + 32;

            if ((Random_GetControl() & 7) == 0) {
                leader->speed = (Random_GetControl() & 0x7F) + 128;
            } else if ((Random_GetControl() & 3) == 0) {
                leader->speed += (Random_GetControl() & 0x7F) + 32;
            } else if (leader->speed > 140) {
                leader->speed += 208 - (Random_GetControl() & 0x1F);
            } else {
                leader->speed_time = (Random_GetControl() & 3) + 4;
                leader->speed += (Random_GetControl() & 0x1F) - 15;
            }
        }
    }

    leader_fish->pos.x = x;
    leader_fish->pos.z = z;

    for (int32_t i = 0; i < M_FISH_PER_SHOAL; i++) {
        M_FISH *const fish = &p->fish[i + 1];

        if (item->object_id == O_PIRAHNAS) {
            const XYZ_32 fish_pos = {
                .x = p->anchor.x + fish->pos.x,
                .y = p->anchor.y + fish->pos.y,
                .z = p->anchor.z + fish->pos.z,
            };
            if (M_FishNearItem(&fish_pos, 256, enemy)) {
                if (p->piranha_hit_wait == 0) {
                    Spawn_Blood(
                        fish_pos.x, fish_pos.y, fish_pos.z, 0, 0,
                        enemy->room_num);
                    p->piranha_hit_wait = 8;
                }

                if (piranha_attack != 2) {
                    Lara_TakeDamage(4, false);
                }
            }
        }

        const int32_t dx = SQUARE(fish->pos.x - x - 128 * i + 3072);
        const int32_t dz = SQUARE(fish->pos.z - z + 128 * i - 3072);

        const uint16_t desired =
            M_GetFishAngle12(fish->pos.x, fish->pos.z, x, z);
        diff = M_GetAngle12Diff(fish->angle, desired);

        if (diff > 128) {
            fish->ang_add -= 4;
            CLAMPL(fish->ang_add, -(i >> 1) - 92);
        } else if (diff < -128) {
            fish->ang_add += 4;
            CLAMPG(fish->ang_add, (i >> 1) + 92);
        } else {
            fish->ang_add -= fish->ang_add >> 2;
            if (ABS(fish->ang_add) < 4) {
                fish->ang_add = 0;
            }
        }

        fish->angle = (fish->angle + fish->ang_add) & 0xFFF;
        if (diff > 1024) {
            fish->angle = (fish->angle + (fish->ang_add >> 2)) & 0xFFF;
        }

        if (dx + dz < 16384 * SQUARE(i) + SQUARE(WALL_L)) {
            if (fish->speed > 2 * i + 32) {
                fish->speed -= fish->speed >> 5;
            }
        } else {
            if (fish->speed < (i >> 1) + 160) {
                fish->speed += (i >> 1) + (Random_GetControl() & 3) + 1;
            }

            if (fish->speed > (i >> 1) - 4 * i + 160) {
                fish->speed = (i >> 1) - 4 * i - 96;
            }
        }

        if ((Random_GetControl() & 1) != 0) {
            fish->speed -= Random_GetControl() & 1;
        } else {
            fish->speed += Random_GetControl() & 1;
        }

        CLAMP(fish->speed, 32, 200);

        fish->swim =
            (fish->swim + (fish->speed >> 4) + (fish->speed >> 5)) & 0x3F;

        const XYZ_32 fish_step =
            XYZ_32_RotateYaw((XYZ_32) { .z = fish->speed }, fish->angle << 4);
        int32_t next_x = fish->pos.x - (fish_step.x >> 1);
        int32_t next_z = fish->pos.z + (fish_step.z >> 1);
        CLAMP(next_x, -32000, 32000);
        CLAMP(next_z, -32000, 32000);
        fish->pos.x = next_x;
        fish->pos.z = next_z;

        if (piranha_attack == 0) {
            if (ABS(fish->pos.y - fish->dest_y) < 16) {
                fish->dest_y = Random_GetControl() % leader->range.y;
            }
        } else if (ABS(fish->pos.y - fish->dest_y) < 16 && enemy != nullptr) {
            fish->dest_y =
                (enemy->pos.y - p->anchor.y + (Random_GetControl() & 0xFF));
        }

        fish->pos.y += (fish->dest_y - fish->pos.y) >> 4;
    }

    item->pos = (XYZ_32) {
        .x = p->anchor.x + leader_fish->pos.x,
        .y = p->anchor.y + leader_fish->pos.y,
        .z = p->anchor.z + leader_fish->pos.z,
    };
    int16_t room_num = (piranha_attack != 0 && enemy != nullptr)
        ? enemy->room_num
        : item->room_num;
    Room_GetSector(item->pos, &room_num);
    if (room_num != item->room_num) {
        Item_UpdateRoom(item_num, room_num);
    }
}

static bool M_Draw(const ITEM *const item)
{
    if (!item->is_simulated) {
        return false;
    }

    M_PRIV *const p = item->priv;
    if (p == nullptr) {
        return false;
    }

    if (!p->leader.on) {
        return false;
    }

    const OBJECT *const sprite_obj = Object_Get(
        item->object_id == O_PIRAHNAS ? O_PIRAHNA_GFX : O_TROPICAL_FISH_GFX);
    if (sprite_obj == nullptr || !sprite_obj->loaded
        || sprite_obj->mesh_count == 0) {
        return false;
    }

    const XYZ_32 base_pos = p->anchor.pos;
    const double ratio = Interpolation_GetWorldRate();
    const bool do_interp =
        Interpolation_IsActive() && ratio > 0.0 && ratio < 1.0;
    OUTPUT_LIGHT_INFO light_info = {};
    if (p->use_room_lighting) {
        Output_CalculateLight(item->interp.result.pos, item->room_num);
        light_info = Output_GetLightInfo();
    }

    M_FISH *fish = &p->fish[1];

    for (int32_t i = 0; i < M_FISH_PER_SHOAL; i++, fish++) {
        if (do_interp) {
            fish->interp.result.pos.x = (int16_t)LERP(
                (int32_t)fish->interp.prev.pos.x, (int32_t)fish->pos.x, ratio);
            fish->interp.result.pos.y = (int16_t)LERP(
                (int32_t)fish->interp.prev.pos.y, (int32_t)fish->pos.y, ratio);
            fish->interp.result.pos.z = (int16_t)LERP(
                (int32_t)fish->interp.prev.pos.z, (int32_t)fish->pos.z, ratio);

            fish->interp.result.angle =
                (Math_AngleMean(
                     fish->interp.prev.angle << 4, fish->angle << 4, ratio)
                 >> 4)
                & 0xFFF;

            int32_t swim_diff =
                (int32_t)fish->swim - (int32_t)fish->interp.prev.swim;
            if (swim_diff > 32) {
                swim_diff -= 64;
            } else if (swim_diff < -32) {
                swim_diff += 64;
            }

            int32_t swim_interp = LERP(
                (int32_t)fish->interp.prev.swim,
                (int32_t)fish->interp.prev.swim + swim_diff, ratio);
            swim_interp %= 64;
            if (swim_interp < 0) {
                swim_interp += 64;
            }
            fish->interp.result.swim = swim_interp;
        } else {
            fish->interp.result.pos = fish->pos;
            fish->interp.result.angle = fish->angle;
            fish->interp.result.swim = fish->swim;
        }

        const int32_t x = base_pos.x + fish->interp.result.pos.x;
        const int32_t y = base_pos.y + fish->interp.result.pos.y;
        const int32_t z = base_pos.z + fish->interp.result.pos.z;

        const int32_t swim_ang16 = fish->interp.result.swim << 10;
        const int32_t swim_wibble = Math_Sin(swim_ang16) >> 7;
        const int32_t ang12 =
            (swim_wibble + fish->interp.result.angle - 2048) & 0xFFF;

        const int32_t size = ((128 * Math_Sin(i << 10)) >> W2V_SHIFT) + 192;
        const XYZ_32 back =
            XYZ_32_RotateYaw((XYZ_32) { .z = size }, ang12 << 4);
        const int32_t back_x = x - back.x;
        const int32_t back_z = z + back.z;

        const XYZ_32 tri_world[3] = {
            { .x = x, .y = y, .z = z },
            { .x = back_x, .y = y - size, .z = back_z },
            { .x = back_x, .y = y + size, .z = back_z },
        };

        int32_t shade = ang12;
        if (shade < 1024) {
            shade -= 512;
        } else if (shade < 2048) {
            shade -= 1536;
        } else if (shade < 3072) {
            shade -= 2560;
        } else {
            shade -= 3584;
        }

        if (shade > 512 || shade < 0) {
            shade = 0;
        } else if (shade < 256) {
            shade >>= 2;
        } else {
            shade = (512 - shade) >> 2;
        }

        shade += i;
        if (shade > 128) {
            shade = 128;
        }

        shade += 80;
        CLAMP(shade, 0, 255);

        const RGBA_8888 color = { shade, shade, shade, 255 };
        const RGBA_8888 tri_color[3] = { color, color, color };

        // OG flips the UV mapping depending on the shoal number (tropical
        // fish) or the fish index (piranhas).
        int32_t sprite_offset = 0;
        if (item->object_id == O_TROPICAL_FISH) {
            sprite_offset = p->sprite_offset;
        } else if ((i & 1) == 0) {
            sprite_offset = 1;
        }

        CLAMP(sprite_offset, 0, ABS(sprite_obj->mesh_count) - 1);
        const int32_t sprite_idx = sprite_obj->mesh_idx + sprite_offset;
        if (p->use_room_lighting) {
            OutputSource_PolyFX_StageSpriteTriWorldLight(
                sprite_idx, tri_world, tri_color, light_info, DRAW_BLEND);
        } else {
            OutputSource_PolyFX_StageSpriteTriWorld(
                sprite_idx, tri_world, tri_color, DRAW_BLEND);
        }
    }

    if (Interpolation_IsActive() && ratio >= 1.0) {
        for (int32_t i = 0; i < M_FISH_PER_SHOAL + 1; i++) {
            p->fish[i].interp.prev.pos = p->fish[i].pos;
            p->fish[i].interp.prev.angle = p->fish[i].angle;
            p->fish[i].interp.prev.swim = p->fish[i].swim;
        }
    }

    return true;
}

static const char *M_CheckRange(const TRX_VALUE *const in)
{
    return in->as_xyz.x < 1 || in->as_xyz.y < 1 || in->as_xyz.z < 1
        ? "range is below one quarter tile on an axis"
        : nullptr;
}

// The range is stated in quarter tiles and swum in world units.
static void M_SetRange(ITEM *const item, const TRX_VALUE *const in)
{
    M_PRIV *const p = item->priv;
    p->range = in->as_xyz;
    p->leader.range.x = p->range.x * STEP_L;
    p->leader.range.y = p->range.y * STEP_L;
    p->leader.range.z = p->range.z * STEP_L;
}

static void M_Initialise(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    item->enable_shadow = false;
    item->is_collidable = false;

    M_PRIV *const p = item->priv;
    p->leader.on = false;
    p->piranha_hit_wait = 0;
    p->carcass_item_num = NO_ITEM;
    p->anchor.pos = item->pos;
    p->anchor.room_num = item->room_num;
}

static void M_SetupCommon(OBJECT *const obj)
{
    obj->initialise_func = M_Initialise;
    obj->trigger_func = M_Trigger;
    obj->control_func = M_Control;
    obj->is_targetable_func = M_IsTargetable;
    obj->draw_func = M_Draw;

    obj->priv_size = sizeof(M_PRIV);
    obj->priv_load_func = M_LoadPriv;
    obj->priv_save_func = M_SavePriv;

    obj->save_position = true;
    obj->save_hitpoints = true;
    obj->save_flags = true;

    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY_SETTER(
            M_PRIV, range, (M_DEFAULT_RANGE), M_CheckRange, M_SetRange,
            "Swim range, in quarter tiles. Value range: minimum 1 on each "
            "axis."),
        OBJECT_PROPERTY(
            M_PRIV, use_room_lighting, (bool)(g_TRVersion < 3),
            "Whether the shoal uses the surrounding room lighting."));
}

static void M_SetupTropicalFish(OBJECT *const obj)
{
    M_SetupCommon(obj);
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            M_PRIV, sprite_offset, 0,
            "Texture offset in `O_TROPICAL_FISH_GFX`."));
}

static void M_SetupPiranhas(OBJECT *const obj)
{
    M_SetupCommon(obj);
}

void Shoal_TriggerDeactivate(const ITEM *const item)
{
    // Anti-trigger turns the leader off to force a re-setup.
    if (item->priv != nullptr) {
        M_PRIV *const p = item->priv;
        p->leader.on = false;
    }
}

REGISTER_OBJECT(O_TROPICAL_FISH, M_SetupTropicalFish)
REGISTER_OBJECT(O_PIRAHNAS, M_SetupPiranhas)
