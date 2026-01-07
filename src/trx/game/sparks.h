#pragma once

#include <trx/game/items/types.h>
#include <trx/game/types.h>

#include <stdint.h>

typedef struct SPARK {
    bool on;

    uint8_t s_life;
    uint8_t life;

    // NOTE: `pos` is either absolute world position, or a relative offset when
    // attached to an FX/ITEM and not using `SPARK_F_ATTACHED_POS`.
    XYZ_32 pos;
    XYZ_32 vel;
    struct {
        uint8_t width;
        uint8_t height;
    } src_size, dst_size, size;

    RGB_888 src_color;
    RGB_888 dst_color;
    RGB_888 color;

    uint8_t scalar;
    uint8_t col_fade_speed;
    uint8_t fade_to_black;
    int16_t gravity;
    int8_t max_y_vel;
    uint8_t friction;

    uint16_t flags;
    union {
        // effect/item index depending on flags (SF_FX/SF_ITEM)
        int16_t effect_num;
        int16_t item_num;
    };
    uint8_t room_num;
    uint8_t node_num;

    uint8_t extras;
    int8_t dynamic;

    uint16_t rot_angle; // 0..0xFFF
    int8_t rot_add;

    int32_t sprite_idx;
    DRAW_TYPE draw_type;
} SPARK;

enum {
    // clang-format off
    SPARK_F_NONE          = 0x0,
    SPARK_F_SCALE         = 0x2,
    SPARK_F_BLOOD         = 0x4,
    SPARK_F_SPRITE        = 0x8,
    SPARK_F_ROTATE        = 0x10,
    SPARK_F_FX            = 0x40,
    SPARK_F_ITEM          = 0x80,
    SPARK_F_OUTSIDE       = 0x100,
    SPARK_F_ALT_SPRITE    = 0x200,
    SPARK_F_ATTACHED_POS  = 0x400,
    SPARK_F_UNDERWATER    = 0x800,
    SPARK_F_ATTACHED_NODE = 0x1000,
    SPARK_F_GREEN         = 0x2000,
    // clang-format on
};

void Sparks_Init(void);
void Sparks_Control(void);
void Sparks_Draw(void);

void Sparks_DetachEffect(int16_t effect_num);
void Sparks_DetachItem(int16_t item_num);

XYZ_32 Sparks_GetWorldPos(const SPARK *spark);

SPARK *Sparks_GetFreeSpark(void);
SPARK *Sparks_GetSpark(int32_t idx);

int8_t Sparks_AllocDynamic(uint8_t flags);
void Sparks_FreeDynamic(int8_t idx);

typedef struct {
    void (*trigger_explosion_sparks)(
        XYZ_32 pos, int32_t extras, int8_t dynamic, int32_t uw,
        int16_t room_num);
    void (*trigger_explosion_bubble)(XYZ_32 pos, int16_t room_num);
} SPARKS_CALLBACKS;

void Sparks_SetCallbacks(const SPARKS_CALLBACKS *callbacks);
XZ_32 Sparks_GetSmokeWind(void);
void Sparks_SetSmokeWind(XZ_32 wind);
int32_t Sparks_GetHairWindZ(void);

void Sparks_TriggerBubble(
    int32_t x, int32_t y, int32_t z, int32_t size, int32_t size_range,
    int16_t effect_num);

void Sparks_TriggerWaterfallMist(int32_t x, int32_t y, int32_t z, int32_t ang);

void Sparks_TriggerBreath(XYZ_32 pos, XYZ_32 vel, int16_t room_num);

void Sparks_TriggerUnderwaterExplosion(const ITEM *item);
void Sparks_TriggerExplosionSparks(
    int32_t x, int32_t y, int32_t z, int32_t extras, int32_t dynamic,
    int32_t uw, int16_t room_num);
void Sparks_TriggerFireFlame(XYZ_32 pos, int32_t body_part, int32_t type);
void Sparks_TriggerStaticFlame(XYZ_32 pos, int32_t size);
void Sparks_TriggerFireSmoke(XYZ_32 pos, int32_t body_part, int32_t type);
void Sparks_TriggerSideFlame(
    XYZ_32 pos, int32_t angle, int32_t speed, bool pilot);

void Sparks_TriggerFlareSparks(XYZ_32 pos, XYZ_32 vel, bool smoke);

void Sparks_TriggerRicochet(GAME_VECTOR pos, int32_t angle, int32_t size);
