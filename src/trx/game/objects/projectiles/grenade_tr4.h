#pragma once

#include <trx/core/math/types.h>
#include <trx/game/objects/types.h>

#include <stdint.h>

typedef enum {
    GRENADE_TR4_KIND_NORMAL,
    GRENADE_TR4_KIND_SUPER,
    GRENADE_TR4_KIND_FLASH,
} GRENADE_TR4_KIND;

// Sets the grenade object up to behave as in TR4.
void Grenade_TR4_Setup(OBJECT *obj);

// Returns the kind of grenade the grenade gun is loaded with: the first,
// second or third kind of ammunition it takes.
GRENADE_TR4_KIND Grenade_TR4_GetLoadedKind(void);

// Fires a TR4 grenade of a kind from a place, flying the way rot points at a
// speed. Returns the grenade's item number, or NO_ITEM where no item is free.
int16_t Grenade_TR4_Fire(
    XYZ_32 pos, int16_t room_num, XYZ_16 rot, int16_t speed,
    GRENADE_TR4_KIND kind);
