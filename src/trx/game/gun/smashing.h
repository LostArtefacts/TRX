#pragma once

#include <trx/game/items.h>
#include <trx/game/rooms/types.h>

typedef enum {
    PROJECTILE_HIT_NONE,
    PROJECTILE_HIT_SHATTER, // some objects were shattered
    PROJECTILE_HIT_STOP, // a hard object (eg a bell) has been hit
} PROJECTILE_HIT;

typedef enum {
    GUN_SMASH_POLICY_NONE,
    GUN_SMASH_POLICY_CONTINUE,
    GUN_SMASH_POLICY_STOP,
    GUN_SMASH_POLICY_HEAVY,
} GUN_SMASH_POLICY;

GUN_SMASH_POLICY Gun_GetSmashPolicy(const ITEM *item);

void Gun_SmashItem(int16_t item_num);
void Gun_ShatterStatic(const STATIC_MESH *mesh, GAME_VECTOR hit_pos);
// Gives the shot to the nearest shatterable static in its path. Returns whether
// a static took it. Smashables in front of the static still take the shot
// first.
bool Gun_ShootStatic(GAME_VECTOR start, GAME_VECTOR target);

PROJECTILE_HIT Gun_SmashItems(
    GAME_VECTOR start, GAME_VECTOR target, XYZ_32 *out_hit_pos,
    OBJECT_ID missile_obj_id);
