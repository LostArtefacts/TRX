#include <trx/game/objects.h>

// Keeps Horus undrawn until his control is ported, so that he does not
// stand still in the temple.
static void M_Setup(OBJECT *const obj)
{
    obj->draw_func = nullptr;
}

REGISTER_OBJECT(O_HORUS, M_Setup)
