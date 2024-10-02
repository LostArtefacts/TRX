#pragma once

#include "global/const.h"
#include "global/types.h"

#include <libtrx/utils.h>

#include <stdbool.h>
#include <stdint.h>

#define CREATURE_SHOOT_RANGE SQUARE(WALL_L * 7) // = 51380224
#define CREATURE_MISS_CHANCE 0x2000

typedef struct {
    struct {
        GAME_OBJECT_ID id;
        int16_t active_anim;
        int16_t death_anim;
        int16_t death_state;
    } land;
    struct {
        GAME_OBJECT_ID id;
        int16_t active_anim;
    } water;
} HYBRID_INFO;

void Creature_Initialise(int16_t item_num);
void Creature_AIInfo(ITEM *item, AI_INFO *info);
void Creature_Mood(ITEM *item, AI_INFO *info, bool violent);
int16_t Creature_Turn(ITEM *item, int16_t maximum_turn);
void Creature_Tilt(ITEM *item, int16_t angle);
void Creature_Head(ITEM *item, int16_t required);
int16_t Creature_Effect(
    ITEM *item, BITE *bite,
    int16_t (*spawn)(
        int32_t x, int32_t y, int32_t z, int16_t speed, int16_t yrot,
        int16_t room_num));
bool Creature_CheckBaddieOverlap(int16_t item_num);
void Creature_Collision(int16_t item_num, ITEM *lara_item, COLL_INFO *coll);
bool Creature_Animate(int16_t item_num, int16_t angle, int16_t tilt);
bool Creature_CanTargetEnemy(ITEM *item, AI_INFO *info);
bool Creature_ShootAtLara(
    ITEM *item, int32_t distance, BITE *gun, int16_t extra_rotation,
    int16_t damage);
bool Creature_EnsureHabitat(
    int16_t item_num, int32_t *wh, const HYBRID_INFO *info);
bool Creature_IsBoss(int16_t item_num);
