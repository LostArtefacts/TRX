#pragma once

#include <trx/game/collision/common.h>
#include <trx/game/items/types.h>

// How far a TR4 blast reaches.
#define BLAST_RADIUS 2048

// Smashes, shatters and hurts what a TR4 blast reaches, as grenades and
// explosive crossbow bolts do. A blast that hurts_lara also hurts Lara where
// she is in reach.
void Blast_HitObjects(
    const ITEM *source, const COLLIDED_OBJECTS *found, bool hurts_lara);

// Sets off a TR4 blast at the source: a fireball and a shockwave out of
// water, or a burst of bubbles in it, with the sound of the explosion.
void Blast_Explode(ITEM *source, XYZ_32 old_pos);

// Plays the sound of a TR4 explosion at the source, and alerts the guards
// nearby.
void Blast_SoundOff(const ITEM *source);
