#include "game/objects/creatures/skidoo_driver.h"

#include "global/funcs.h"
#include "global/vars.h"

void SkidooDriver_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_SKIDOO_DRIVER];
    if (!obj->loaded) {
        return;
    }

    obj->initialise = SkidooDriver_Initialise;
    obj->control = SkidooDriver_Control;

    obj->hit_points = 1;

    obj->save_position = 1;
    obj->save_anim = 1;
    obj->save_flags = 1;
}
