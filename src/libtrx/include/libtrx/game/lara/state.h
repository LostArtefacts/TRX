#include "../collision.h"
#include "./enum.h"

void Lara_State_Register(
    LARA_STATE state, void (*handle_func)(ITEM *item, COLL_INFO *coll));
void Lara_State_RegisterExtra(
    LARA_EXTRA_STATE state, void (*handle_func)(ITEM *item, COLL_INFO *coll));
void Lara_State_Initialise(void);
bool Lara_State_IsResponsive(LARA_ANIMATION anim_idx);
void Lara_State_Update(ITEM *item, COLL_INFO *coll);

// Temporarily public
void Lara_State_Walk(ITEM *item, COLL_INFO *coll);
void Lara_State_Run(ITEM *item, COLL_INFO *coll);
