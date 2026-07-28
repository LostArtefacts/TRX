#include <trx/game/objects.h>
#include <trx/game/objects/property.h>
#include <trx/game/objects/traps/propeller.h>

static void M_Setup(OBJECT *const obj)
{
    obj->priv_size = sizeof(PROPELLER_PRIV);
    obj->control_func = Propeller_Control;
    obj->collision_func = Object_Collision;
    obj->save_flags = true;
    obj->save_anim = true;
    OBJECT_PROPERTIES(
        obj,
        OBJECT_PROPERTY(
            PROPELLER_PRIV, damage, PROPELLER_DEFAULT_DAMAGE,
            "Damage dealt while Lara is touching the power saw."));
}

REGISTER_OBJECT(O_POWER_SAW, M_Setup)
