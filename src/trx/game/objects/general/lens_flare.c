#include <trx/game/objects.h>
#include <trx/game/output/lens_flares.h>

static void M_Setup(OBJECT *const obj)
{
    obj->draw_func = Output_LensFlares_DrawObject;
}

REGISTER_OBJECT(O_LENS_FLARE, M_Setup)
