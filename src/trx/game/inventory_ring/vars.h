#pragma once

#include <trx/core/vector.h>
#include <trx/game/camera/types.h>
#include <trx/game/inventory_ring/types.h>

// What the ring was opened for, which decides what its entries do when
// chosen: a save crystal saves rather than heals, the passport reads
// differently at the title screen than in play.
extern INVENTORY_MODE g_InvRing_Mode;
extern CAMERA_INFO g_InvRing_OldCamera;
extern INV_RING_SOURCE g_InvRing_Source[RT_NUMBER_OF];
extern VECTOR *g_InvRing_Items;

void InvRing_LoadVars(const char *path);
