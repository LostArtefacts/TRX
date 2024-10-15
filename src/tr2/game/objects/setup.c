#include "game/objects/setup.h"

#include "game/lara/control.h"
#include "game/objects/common.h"
#include "game/objects/creatures/bandit.h"
#include "game/objects/creatures/barracuda.h"
#include "game/objects/creatures/bartoli.h"
#include "game/objects/creatures/big_eel.h"
#include "game/objects/creatures/big_spider.h"
#include "game/objects/creatures/bird.h"
#include "game/objects/creatures/cultist.h"
#include "game/objects/creatures/diver.h"
#include "game/objects/creatures/dog.h"
#include "game/objects/creatures/dragon.h"
#include "game/objects/creatures/eel.h"
#include "game/objects/creatures/giant_yeti.h"
#include "game/objects/creatures/jelly.h"
#include "game/objects/creatures/monk.h"
#include "game/objects/creatures/mouse.h"
#include "game/objects/creatures/shark.h"
#include "game/objects/creatures/skidoo_driver.h"
#include "game/objects/creatures/spider.h"
#include "game/objects/creatures/tiger.h"
#include "game/objects/creatures/trex.h"
#include "game/objects/creatures/winston.h"
#include "game/objects/creatures/worker.h"
#include "game/objects/creatures/xian_knight.h"
#include "game/objects/creatures/xian_spearman.h"
#include "game/objects/creatures/yeti.h"
#include "global/funcs.h"
#include "global/types.h"
#include "global/vars.h"

#define DEFAULT_RADIUS 10

static void M_SetupLara(void);
static void M_SetupLaraExtra(void);

static void M_SetupLara(void)
{
    OBJECT *const obj = &g_Objects[O_LARA];
    obj->initialise = Lara_InitialiseLoad;

    obj->shadow_size = (UNIT_SHADOW / 16) * 10;
    obj->hit_points = LARA_MAX_HITPOINTS;
    obj->draw_routine = Object_DrawDummyItem;

    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
    obj->save_anim = 1;
}

static void M_SetupLaraExtra(void)
{
    OBJECT *const obj = &g_Objects[O_LARA_EXTRA];
    obj->control = Lara_ControlExtra;
}

void __cdecl Object_SetupBaddyObjects(void)
{
    M_SetupLara();
    M_SetupLaraExtra();

    Dog_Setup();
    Mouse_Setup();
    Cultist1_Setup();
    Cultist1A_Setup();
    Cultist1B_Setup();
    Cultist2_Setup();
    Cultist3_Setup();
    Shark_Setup();
    Tiger_Setup();
    Barracuda_Setup();
    Spider_Setup();
    BigSpider_Setup();
    Yeti_Setup();
    Jelly_Setup();
    Diver_Setup();
    Worker1_Setup();
    Worker2_Setup();
    Worker3_Setup();
    Worker4_Setup();
    Worker5_Setup();
    Monk1_Setup();
    Monk2_Setup();
    Bird_SetupEagle();
    Bird_SetupCrow();
    SkidooArmed_Setup();
    SkidooDriver_Setup();
    Bartoli_Setup();
    Dragon_SetupFront();
    Dragon_SetupBack();
    Bandit1_Setup();
    Bandit2_Setup();
    Bandit2B_Setup();
    BigEel_Setup();
    Eel_Setup();
    XianKnight_Setup();
    XianSpearman_Setup();
    GiantYeti_Setup();
    TRex_Setup();
    Winston_Setup();
}

void __cdecl Object_SetupAllObjects(void)
{
    for (int32_t i = 0; i < O_NUMBER_OF; i++) {
        OBJECT *const object = Object_GetObject(i);
        object->initialise = NULL;
        object->control = NULL;
        object->floor = NULL;
        object->ceiling = NULL;
        object->draw_routine = Object_DrawAnimatingItem;
        object->collision = NULL;
        object->hit_points = DONT_TARGET;
        object->pivot_length = 0;
        object->radius = DEFAULT_RADIUS;
        object->shadow_size = 0;

        object->save_position = 0;
        object->save_hitpoints = 0;
        object->save_flags = 0;
        object->save_anim = 0;
        object->intelligent = 0;
        object->water_creature = 0;
    }

    Object_SetupBaddyObjects();
    Object_SetupTrapObjects();
    Object_SetupGeneralObjects();

    InitialiseHair();
}
