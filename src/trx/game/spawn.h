#pragma once

#include <trx/game/gun/types.h>
#include <trx/game/items/types.h>
#include <trx/game/lara/types.h>
#include <trx/game/types.h>

XYZ_32 Spawn_GetRayPos(GAME_VECTOR start, GAME_VECTOR hit_pos, int32_t dist);

void Spawn_Splash(const ITEM *item);
void Spawn_Ricochet(GAME_VECTOR pos);
// `count` is the number of streaks to spawn, and only applies to TR4.
void Spawn_RicochetRay(GAME_VECTOR start, GAME_VECTOR hit_pos, int32_t count);

// The explosion a rocket or a grenade leaves behind, without the damage: the
// sprite effect in TR1/2, and a spark fireball in TR3/4, which has no
// explosion sprite. Under water the effect is the drowned one.
void Spawn_Explosion(XYZ_32 pos, int16_t room_num, bool with_sound);

void Spawn_Bubble(const XYZ_32 *pos, int16_t room_num);
void Spawn_BubbleEx(
    const XYZ_32 *pos, int16_t room_num, int32_t size, int32_t size_range);

int16_t Spawn_Blood(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_BloodD(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
void Spawn_BloodBath(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num, int32_t count);
void Spawn_BloodBathD(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num, int32_t count);
int16_t Spawn_GunShot(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_GunHit(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_GunMiss(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

void Spawn_GunShell(LARA_GUN_TYPE weapon_type, bool right);
void Spawn_ShotgunShell(void);

int16_t Spawn_AtlanteanShard(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_AtlanteanBomb(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

int16_t Spawn_FireStream(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);

void Spawn_MysticLight(int16_t item_num);

int16_t Spawn_Knife(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
int16_t Spawn_Harpoon(
    int32_t x, int32_t y, int32_t z, int16_t speed, int16_t y_rot,
    int16_t room_num);
