#pragma once

#include <stdint.h>

// Mesh_bits: which meshes to affect.
// Damage:
// * Positive values - deal damage, enable body part explosions.
// * Negative values - deal damage, disable body part explosions.
// * Zero - don't deal any damage, disable body part explosions.
int32_t Effect_ExplodingDeath(
    int16_t item_num, int32_t mesh_bits, int16_t damage);
