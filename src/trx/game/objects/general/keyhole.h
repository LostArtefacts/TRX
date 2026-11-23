#pragma once

#include <stdint.h>

// Reports whether the key trigger of the receptacle fires. An armed trigger
// fires while Lara is busy with the animation that uses the receptacle, and
// leaves the receptacle ready to fire again.
bool Keyhole_Trigger(int16_t item_num, bool is_armed);
