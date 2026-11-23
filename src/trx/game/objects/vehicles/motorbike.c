#include <trx/game/objects.h>

// Keeps the motorbike undrawn until its control is ported, so that it does
// not stand as a bike that Lara cannot ride.
static void M_Setup(OBJECT *const obj)
{
    obj->draw_func = nullptr;
}

REGISTER_OBJECT(O_MOTORBIKE, M_Setup)
