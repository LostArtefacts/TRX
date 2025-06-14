#include "../collision.h"
#include "./enum.h"

void Lara_State_Register(
    LARA_STATE state, void (*handle_func)(ITEM *item, COLL_INFO *coll));
void Lara_State_RegisterExtra(
    LARA_EXTRA_STATE state, void (*handle_func)(ITEM *item, COLL_INFO *coll));
void Lara_State_Update(ITEM *item, COLL_INFO *coll);
