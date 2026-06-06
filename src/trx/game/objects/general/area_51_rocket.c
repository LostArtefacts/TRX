#include <trx/core/utils.h>
#include <trx/game/camera.h>
#include <trx/game/game_buf.h>
#include <trx/game/lara.h>
#include <trx/game/objects.h>
#include <trx/game/output.h>
#include <trx/game/random.h>
#include <trx/game/rooms.h>
#include <trx/game/sound.h>
#include <trx/game/sparks.h>

#define M_SMOKE_END 512

typedef enum {
    M_SUPPORT_STATE_WAIT = 1,
    M_SUPPORT_STATE_FALL = 2,
} M_SUPPORT_STATE;

typedef struct {
    int16_t fire_room_num;
} M_PRIV;

static bool m_SupportFallen = false;

static bool M_DoesRoomLeadToItemCeilingPos(
    const ITEM *const item, const int16_t room_num)
{
    const SECTOR *const sector =
        Room_GetWorldSector(Room_Get(room_num), item->pos.x, item->pos.z);
    return sector->portal_room.sky == item->room_num;
}

static int16_t M_FindFireRoomNum(const ITEM *const item)
{
    int16_t fire_room_num = item->room_num;
    const XYZ_32 probe_pos = {
        .x = item->pos.x,
        .y = item->pos.y + WALL_L,
        .z = item->pos.z,
    };
    const int16_t room_num = Room_GetIndexFromPos(probe_pos);
    if (room_num != NO_ROOM && room_num != fire_room_num
        && M_DoesRoomLeadToItemCeilingPos(item, room_num)) {
        fire_room_num = room_num;
    }
    return fire_room_num;
}

static void M_InitialiseMain(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (item->priv == nullptr) {
        item->priv = GameBuf_Alloc(sizeof(M_PRIV), GBUF_ITEM_DATA);
    }

    M_PRIV *const p = item->priv;
    p->fire_room_num = M_FindFireRoomNum(item);
}

static int32_t M_GetFloorY(const ITEM *const item)
{
    const M_PRIV *const p = item->priv;
    if (p != nullptr && p->fire_room_num != NO_ROOM) {
        return Room_Get(p->fire_room_num)->min_floor;
    }

    int16_t room_num = item->room_num;
    Room_GetSector(item->pos, &room_num);
    return Room_Get(room_num)->min_floor;
}

static void M_TriggerBlastFire(
    const XYZ_32 pos, const bool smoke, const int32_t end)
{
    SPARK *const spark = end < 0
        ? Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION)
        : Sparks_GetSpark(end);
    if (spark == nullptr) {
        return;
    }
    spark->on = true;

    if (smoke) {
        spark->src_color.r = 0;
        spark->src_color.g = 0;
        spark->src_color.b = 0;
        spark->dst_color.r = 64;
        spark->dst_color.g = 64;
        spark->dst_color.b = 64;
    } else {
        spark->src_color.r = (Random_GetControl() & 0x1F) + 128;
        spark->src_color.g = (Random_GetControl() & 0x1F) + 64;
        spark->src_color.b = 32;
        spark->dst_color.r = (Random_GetControl() & 0x1F) + 224;
        spark->dst_color.g = (Random_GetControl() & 0x1F) + 160;
        spark->dst_color.b = 32;
    }

    spark->col_fade_speed = 16;

    if (end) {
        spark->fade_to_black = (Random_GetControl() & 0x1F) + 32;
        spark->life = (end >> 1) + 72;
    } else {
        spark->fade_to_black = smoke ? 32 : 8;
        spark->life = (smoke ? 32 : 0) + (Random_GetControl() & 7) + 32;
    }

    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0xF) - 8;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = -(Random_GetControl() & 7);
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;

    spark->friction = 4;
    spark->flags =
        SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE | SPARK_F_SCALE;
    spark->rot_angle = Random_GetControl() & 0xFFF;

    if (Random_GetControl() & 1) {
        spark->rot_add = -16 - (Random_GetControl() & 0xF);
    } else {
        spark->rot_add = (Random_GetControl() & 0xF) + 16;
    }

    spark->scalar = 4;
    spark->max_y_vel = 0;
    spark->gravity = 0;

    const int32_t size = (Random_GetControl() & 0x3F) + 64;
    if (end) {
        spark->size.width = size;
        spark->src_size.width = spark->size.width;
        spark->dst_size.width = spark->size.width;

        spark->size.height = spark->size.width;
        spark->src_size.height = spark->size.height;
        spark->dst_size.height = spark->size.height;
    } else {
        spark->dst_size.width = size;
        spark->size.width = spark->dst_size.width >> 1;
        spark->src_size.width = spark->size.width;

        spark->dst_size.height = spark->dst_size.width;
        spark->size.height = spark->dst_size.height >> 1;
        spark->src_size.height = spark->size.height;
    }
    Sparks_FinishSetup(spark);
}

static void M_TriggerRocketSmoke(
    const XYZ_32 pos, const int32_t yv, const bool fire)
{
    SPARK *const spark = Sparks_InitialiseSpriteSpark(SPARK_TYPE_EXPLOSION);
    if (spark == nullptr) {
        return;
    }

    if (fire) {
        spark->src_color.r = (Random_GetControl() & 0x1F) + 48;
        spark->src_color.g = spark->src_color.r;
        spark->src_color.b = (Random_GetControl() & 0x3F) + 192;
        spark->dst_color.r = (Random_GetControl() & 0x3F) + 192;
        spark->dst_color.g = (Random_GetControl() & 0x3F) + 128;
        spark->dst_color.b = 32;
    } else {
        spark->src_color.r = 0;
        spark->src_color.g = 0;
        spark->src_color.b = 0;
        spark->dst_color.r = (yv >> 5) + 32;
        spark->dst_color.g = spark->dst_color.r;
        spark->dst_color.b = spark->dst_color.r;
    }

    spark->col_fade_speed = 16 - (fire ? yv >> 9 : 0);
    spark->fade_to_black = !fire ? 16 : 0;
    spark->life = (Random_GetControl() & 3) - (fire ? yv >> 8 : 0) + 60;
    spark->s_life = spark->life;
    spark->draw_type = DRAW_BLEND_ADD;
    spark->extras = 0;
    spark->dynamic = -1;
    spark->pos.x = pos.x + (Random_GetControl() & 0xF) - 8;
    spark->pos.y = pos.y;
    spark->pos.z = pos.z + (Random_GetControl() & 0xF) - 8;
    spark->vel.x = (Random_GetControl() & 0xFF) - 128;
    spark->vel.y = yv + (Random_GetControl() & 0xF);
    spark->vel.z = (Random_GetControl() & 0xFF) - 128;
    spark->friction = 4;

    if (Random_GetControl() & 1) {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_ROTATE | SPARK_F_SPRITE
            | SPARK_F_SCALE;
        spark->rot_angle = Random_GetControl() & 0xFFF;

        if (Random_GetControl() & 1) {
            spark->rot_add = -16 - (Random_GetControl() & 0xF);
        } else {
            spark->rot_add = (Random_GetControl() & 0xF) + 16;
        }
    } else {
        spark->flags = SPARK_F_ALT_SPRITE | SPARK_F_SPRITE | SPARK_F_SCALE;
    }

    spark->scalar = 3;
    spark->max_y_vel = 0;
    spark->gravity = 0;

    int32_t size = (Random_GetControl() & 0x3F) + (yv >> 5) + 64;
    CLAMPG(size, 255);

    spark->dst_size.width = (uint8_t)size;
    spark->size.width = spark->dst_size.width >> 2;
    spark->src_size.width = spark->size.width;

    spark->dst_size.height = spark->dst_size.width;
    spark->size.height = spark->dst_size.height >> 2;
    spark->src_size.height = spark->size.height;
    Sparks_FinishSetup(spark);
}

static void M_ControlRocket(ITEM *const item)
{
    if (item->required_anim_state < M_SMOKE_END) {
        item->required_anim_state += 8;

        if (item->required_anim_state < M_SMOKE_END) {
            Sound_Effect(SFX_LARA_FLARE_BURN, nullptr, SPM_NORMAL);
            item->current_anim_state = 0;
            item->goal_anim_state = 0;
        } else {
            item->required_anim_state += 2048;
            item->goal_anim_state = 64;
            Sound_Effect(SFX_EXPLOSION_2, nullptr, SPM_NORMAL);
        }
    } else {
        if (item->current_anim_state) {
            Sound_Effect(SFX_HUGE_ROCKET_LOOP, nullptr, SPM_NORMAL);
            item->required_anim_state += 32;

            if (item->required_anim_state > 16000) {
                Item_Kill(Item_GetIndex(item));
            } else {
                const int32_t base = 0x4000 - item->required_anim_state;
                const RGB_888 color = {
                    .r = (base * ((Random_GetControl() & 0x1F) + 224)) >> 12,
                    .g = (base * ((Random_GetControl() & 0x3F) + 96)) >> 12,
                    .b = (base * (Random_GetControl() & 0x1F)) >> 12,
                };
                Output_AddDynamicLightRGB(
                    (XYZ_32) {
                        .x = item->pos.x - 7680,
                        .y = M_GetFloorY(item) - (Random_GetControl() & 0x1FF)
                            - 256,
                        .z = item->pos.z - 1024,
                    },
                    24, color);
                g_Camera.bounce = -((0x4000 - item->required_anim_state) >> 6);
            }

            return;
        }

        if (!Lara_GetLaraInfo()->burn) {
            int32_t rad = item->goal_anim_state;
            CLAMPG(rad, 8192);

            const ITEM *const lara_item = Lara_GetItem();
            if (lara_item->pos.x > item->pos.x - rad - 1536) {
                Lara_TakeDamage(lara_item->hit_points, false);
                Lara_CatchFire();
            }

            item->goal_anim_state += 80;
        }

        item->required_anim_state += 32;
    }

    if (item->required_anim_state < 4096 + M_SMOKE_END) {
        if (item->goal_anim_state > 768) {
            Sound_Effect(SFX_HUGE_ROCKET_LOOP, nullptr, SPM_NORMAL);
        }

        if (item->goal_anim_state > 1024) {
            item->goal_anim_state = 1024;
        }
    } else {
        Sound_Effect(SFX_HUGE_ROCKET_LOOP, nullptr, SPM_NORMAL);
        if (item->required_anim_state > 12288) {
            item->required_anim_state = 12288;
        }

        if (item->goal_anim_state > 22528) {
            for (int32_t i = 0; i < 64; i++) {
                const XYZ_32 pos = {
                    .x = item->pos.x - (Random_GetControl() & 0xFFF) - 5632,
                    .y = M_GetFloorY(item) - (Random_GetControl() & 0x7FF),
                    .z = item->pos.z + (Random_GetControl() & 0x7FF) - 2048,
                };
                M_TriggerBlastFire(pos, false, i);
            }

            for (int32_t i = 64; i < 96; i++) {
                const XYZ_32 pos = {
                    .x = item->pos.x - (Random_GetControl() & 0xFFF) - 5632,
                    .y = M_GetFloorY(item) - (Random_GetControl() & 0x7FF),
                    .z = item->pos.z + (Random_GetControl() & 0x7FF) - 2048,
                };
                M_TriggerBlastFire(pos, true, i);
            }

            g_Camera.bounce = -((0x4000 - item->required_anim_state) >> 6);
            item->current_anim_state = 1;
            return;
        }

        item->fall_speed--;
        CLAMPL(item->fall_speed, -1024);

        if (item->fall_speed < -72) {
            m_SupportFallen = true;
        }

        item->pos.y += item->fall_speed >> 2;
        int16_t room_num = item->room_num;
        Room_GetSector(item->pos, &room_num);

        if (item->room_num != room_num) {
            Item_UpdateRoom(Item_GetIndex(item), room_num);
        }
    }

    if (item->required_anim_state >= 0x2000) {
        g_Camera.bounce = -((0x4000 - item->required_anim_state) >> 6);
    } else if (item->required_anim_state <= 64) {
        g_Camera.bounce = -1;
    } else {
        g_Camera.bounce = -(item->required_anim_state >> 6);
    }
}

static void M_ControlBlast(ITEM *const item)
{
    const int32_t time4 = Output_GetTimeInGame() * 4;
    if (!(time4 & 0xC)) {
        int32_t yv;
        if (item->required_anim_state >= M_SMOKE_END >> 2) {
            yv = 4 * (item->required_anim_state + (Random_GetControl() & 0x1F))
                - M_SMOKE_END;
        } else {
            yv = Random_GetControl() & 0x1F;
        }
        CLAMPG(yv, 6144);

        const bool fire = item->required_anim_state >= M_SMOKE_END;
        M_TriggerRocketSmoke(
            (XYZ_32) {
                .x = item->pos.x - 896,
                .y = item->pos.y - 64,
                .z = item->pos.z - 512,
            },
            yv, fire);
        M_TriggerRocketSmoke(
            (XYZ_32) {
                .x = item->pos.x - 128,
                .y = item->pos.y - 64,
                .z = item->pos.z - 512,
            },
            yv, fire);
        M_TriggerRocketSmoke(
            (XYZ_32) {
                .x = item->pos.x - 512,
                .y = item->pos.y - 64,
                .z = item->pos.z - 896,
            },
            yv, fire);
        M_TriggerRocketSmoke(
            (XYZ_32) {
                .x = item->pos.x - 512,
                .y = item->pos.y - 64,
                .z = item->pos.z - 128,
            },
            yv, fire);
    }

    if (item->goal_anim_state != 0) {
        RGB_888 color = {
            .r = (Random_GetControl() & 0x1F) + 224,
            .g = (Random_GetControl() & 0x3F) + 96,
            .b = Random_GetControl() & 0x1F,
        };
        Output_AddDynamicLightRGB(
            (XYZ_32) {
                .x = item->pos.x - 512,
                .y = item->pos.y,
                .z = item->pos.z - 512,
            },
            31, color);

        int32_t rad = item->goal_anim_state;
        CLAMPG(rad, 8192);
        const int32_t y_mask = item->goal_anim_state >= 1024 ? 2047 : 255;

        XYZ_32 pos = {
            .x = item->pos.x - (Random_GetControl() & 0x7FF) - rad + 512,
            .y = M_GetFloorY(item) - (y_mask & Random_GetControl()),
            .z = item->pos.z + (Random_GetControl() & 0x7FF) - 2048,
        };

        if (time4 & 4) {
            M_TriggerBlastFire(pos, false, -1);
        }

        pos.x = item->pos.x - rad + 512;
        pos.z = item->pos.z - 1024;
        color.r = (Random_GetControl() & 0x1F) + 224;
        color.g = (Random_GetControl() & 0x3F) + 96;
        color.b = Random_GetControl() & 0x1F;
        Output_AddDynamicLightRGB(pos, 24, color);

        if (time4 & 4) {
            pos.x = item->pos.x - (Random_GetControl() & 0x3FF) - rad - 512;
            pos.y = M_GetFloorY(item) - (y_mask & Random_GetControl());
            pos.z = item->pos.z + (Random_GetControl() & 0x7FF) - 2048;
            M_TriggerBlastFire(pos, true, -1);
        }
    }
}

static void M_ControlMain(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);
    if (!Item_IsTriggerActive(item)) {
        return;
    }

    if (item->object_id == O_AREA_51_ROCKET) {
        M_ControlRocket(item);
    }

    M_ControlBlast(item);
}

void M_InitialiseSupport(const int16_t item_num)
{
    m_SupportFallen = false;
}

static void M_ControlSupport(const int16_t item_num)
{
    ITEM *const item = Item_Get(item_num);

    if (!m_SupportFallen) {
        item->current_anim_state = M_SUPPORT_STATE_WAIT;
    } else if (
        item->goal_anim_state != M_SUPPORT_STATE_FALL
        && item->current_anim_state != M_SUPPORT_STATE_FALL) {
        item->goal_anim_state = M_SUPPORT_STATE_FALL;
    }

    Item_Animate(item);
}

static void M_SetupRocket(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseMain;
    obj->control_func = M_ControlMain;
    obj->save_position = true;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupBlast(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseMain;
    obj->control_func = M_ControlMain;
    obj->draw_func = nullptr;
    obj->save_flags = true;
    obj->save_anim = true;
}

static void M_SetupSupport(OBJECT *const obj)
{
    obj->initialise_func = M_InitialiseSupport;
    obj->control_func = M_ControlSupport;
    obj->save_anim = true;
}

REGISTER_OBJECT(O_AREA_51_ROCKET, M_SetupRocket)
REGISTER_OBJECT(O_AREA_51_ROCKET_BLAST, M_SetupBlast)
REGISTER_OBJECT(O_AREA_51_ROCKET_SUPPORT, M_SetupSupport)
