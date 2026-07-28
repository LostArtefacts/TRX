#pragma once

#include <stdint.h>

#define PROPELLER_DEFAULT_DAMAGE 200

// What Propeller_Control reads out of an item. An object that borrows the
// control sizes its priv to this and binds the property to the member.
typedef struct {
    int32_t damage;
} PROPELLER_PRIV;

void Propeller_Control(int16_t item_num);
