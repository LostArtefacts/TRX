#include "game/objects/general/cutscene_player.h"

#include "decomp/decomp.h"

void CutscenePlayer_Setup(OBJECT *const obj)
{
    obj->initialise_func = CutscenePlayerGen_Initialise;
    obj->control_func = CutscenePlayer_Control;
    obj->hit_points = 1;
}
