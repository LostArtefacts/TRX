#pragma once

#include <trx/core/utils.h>
#include <trx/game/gun/types.h>
#include <trx/game/lara/enum.h>

#include <stdint.h>

// Declares what a weapon does, where the weapon is implemented. A
// declaration applies before the weapon data does, so a field may move
// between the two without moving where it is read from.
void Gun_Registry_Register(const WEAPON_INFO *info);

// Returns every weapon to what its declaration says, discarding what the
// weapon data set the time before. This runs as the session starts, so that
// the table is whole before anything reads it, and a weapon file that cannot
// be read leaves the declarations behind rather than an empty table.
void Gun_Registry_Seed(void);

// Returns the weapon a gun type stands for. Every valid type has one, and a
// type nothing implements reads as empty, which is the case for empty hands
// and for a gun fixed to a vehicle. Read is_declared to tell the two apart.
WEAPON_INFO *Gun_Registry_Get(LARA_GUN_TYPE gun_type);

// Returns the weapon at an index, counting from zero, in the order the gun
// types are numbered, and passing over the types nothing implements.
const WEAPON_INFO *Gun_Registry_GetByIndex(int32_t idx);

// The number of weapons the engine implements, which is fewer than the gun
// types it knows.
int32_t Gun_Registry_GetCount(void);

// Give a weapon the key that draws it, taking that key from whichever
// weapon holds it now. One key draws one weapon, so a mod that claims the
// shotgun's key draws its own weapon with it.
void Gun_Registry_SetInputRole(LARA_GUN_TYPE gun_type, INPUT_ROLE role);

// Return whether the catalog contains the gun type. Legacy saves may contain
// unsupported types.
bool Gun_Registry_IsValidType(LARA_GUN_TYPE gun_type);

// A field that stands for nothing at zero takes that value before the
// declaration names its own, which the compilers report as an override.
#ifdef __clang__
    #define M_GUN_TYPE_SEED_DIAGNOSTIC                                         \
        _Pragma("GCC diagnostic ignored \"-Winitializer-overrides\"")
#else
    #define M_GUN_TYPE_SEED_DIAGNOSTIC                                         \
        _Pragma("GCC diagnostic ignored \"-Woverride-init\"")
#endif

// Declares a weapon as the file is linked, so nothing has to drive a list and
// the order modules initialize in does not matter. A declaration that leaves
// out a field which stands for nothing at zero still reads as empty.
// clang-format off
#define REGISTER_GUN_TYPE(...)                                                 \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterGunType_, __LINE__)(void)                                    \
    {                                                                          \
        _Pragma("GCC diagnostic push")                                         \
        M_GUN_TYPE_SEED_DIAGNOSTIC                                             \
        static const WEAPON_INFO m_GunType = {                                 \
            .projectile_object_id = NO_OBJECT,                                 \
            .equip_input_role = (INPUT_ROLE)-1,                                \
            .glow.scale = 1.0f,                                                \
            .gun_object_id = NO_OBJECT,                                        \
            .ammo_object_id = NO_OBJECT,                                       \
            .anim_object_id = NO_OBJECT,                                       \
            .shell_object_id = NO_OBJECT,                                      \
            __VA_ARGS__,                                                       \
        };                                                                     \
        _Pragma("GCC diagnostic pop")                                          \
        Gun_Registry_Register(&m_GunType);                                     \
    }
// clang-format on
