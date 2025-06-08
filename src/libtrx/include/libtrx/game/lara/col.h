#pragma once

#include "../collision.h"
#include "./enum.h"

void Lara_Col_Register(
    LARA_STATE state, void (*handle_func)(ITEM *item, COLL_INFO *coll));
void Lara_Col_Update(ITEM *item, COLL_INFO *coll);
