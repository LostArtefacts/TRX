#pragma once

#include <trx/core/utils.h>
#include <trx/game/gun/types.h>
#include <trx/game/lara/enum.h>

// Define the shared routines for an implemented weapon kind.
typedef struct {
    void (*draw_func)(LARA_GUN_TYPE gun_type);
    void (*undraw_func)(LARA_GUN_TYPE gun_type);
    void (*draw_meshes_func)(LARA_GUN_TYPE gun_type);
    void (*control_func)(LARA_GUN_TYPE gun_type, LARA_GUN_STATE gun_status);
} GUN_KIND_ROUTINES;

// Register the shared routines for an implemented weapon kind.
void Gun_Routines_AddKind(WEAPON_TYPE type, GUN_KIND_ROUTINES routines);

// Return the shared routines for an implemented weapon kind.
const GUN_KIND_ROUTINES *Gun_Routines_GetKind(WEAPON_TYPE type);

// Register a named routine that a weapon spec can reference.
void Gun_Routines_AddFire(
    const char *name, void (*func)(LARA_GUN_TYPE gun_type, bool running));
void Gun_Routines_AddFlash(const char *name, GUN_FLASH (*func)(void));
void Gun_Routines_AddSound(const char *name, void (*func)(bool stopping));
void Gun_Routines_AddReadyAnim(const char *name, int16_t (*func)(void));
void Gun_Routines_AddSmokeSize(const char *name, uint8_t (*func)(void));

// Return a named routine that a weapon spec references.
void (*Gun_Routines_GetFire(const char *name))(LARA_GUN_TYPE, bool);
GUN_FLASH (*Gun_Routines_GetFlash(const char *name))(void);
void (*Gun_Routines_GetSound(const char *name))(bool);
int16_t (*Gun_Routines_GetReadyAnim(const char *name))(void);
uint8_t (*Gun_Routines_GetSmokeSize(const char *name))(void);

// Register routines during module initialization.
#define REGISTER_GUN_KIND(type_, ...)                                          \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterGunKind_, __LINE__)(void)                                    \
    {                                                                          \
        Gun_Routines_AddKind((type_), (GUN_KIND_ROUTINES) { __VA_ARGS__ });    \
    }

#define REGISTER_GUN_ROUTINE(kind_, name_, func_)                              \
    __attribute__((__constructor__)) static void CONCAT(                       \
        M_RegisterGunRoutine_, __LINE__)(void)                                 \
    {                                                                          \
        CONCAT(Gun_Routines_Add, kind_)((name_), (func_));                     \
    }
