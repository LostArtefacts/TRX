#pragma once

#include "global/types.h"

#include <libtrx/game/lara/misc.h>

bool Lara_Fallen(ITEM *item, COLL_INFO *coll);
bool Lara_DeflectEdge(ITEM *item, COLL_INFO *coll);
bool Lara_TestVault(ITEM *item, COLL_INFO *coll);
bool Lara_TestHangSwingIn(ITEM *item, PHD_ANGLE angle);
