#include "game/data.h"
#include "game/draw.h"
#include "game/setup.h"
#include "game/types.h"
#include "mod.h"
#include "util.h"

void __cdecl DrawDummyItem(ITEM_INFO* item)
{
}

void __cdecl InitialiseObjects()
{
    for (int i = 0; i < NUMBER_OBJECTS; i++) {
        OBJECT_INFO* obj = &Objects[i];
        obj->intelligent = 0;
        obj->save_position = 0;
        obj->save_hitpoints = 0;
        obj->save_flags = 0;
        obj->save_anim = 0;
        obj->initialise = NULL;
        obj->collision = NULL;
        obj->control = NULL;
        obj->draw_routine = DrawAnimatingItem;
        obj->ceiling = NULL;
        obj->floor = NULL;
        obj->pivot_length = 0;
        obj->radius = DEFAULT_RADIUS;
        obj->shadow_size = 0;
        obj->hit_points = DONT_TARGET;
    }
    BaddyObjects();
    TrapObjects();
    ObjectObjects();

    if (Tomb1MConfig.disable_medpacks) {
        Objects[O_MEDI_ITEM].initialise = NULL;
        Objects[O_MEDI_ITEM].collision = NULL;
        Objects[O_MEDI_ITEM].control = NULL;
        Objects[O_MEDI_ITEM].draw_routine = DrawDummyItem;
        Objects[O_MEDI_ITEM].ceiling = NULL;
        Objects[O_MEDI_ITEM].floor = NULL;

        Objects[O_BIGMEDI_ITEM].initialise = NULL;
        Objects[O_BIGMEDI_ITEM].collision = NULL;
        Objects[O_BIGMEDI_ITEM].control = NULL;
        Objects[O_BIGMEDI_ITEM].draw_routine = DrawDummyItem;
        Objects[O_BIGMEDI_ITEM].ceiling = NULL;
        Objects[O_BIGMEDI_ITEM].floor = NULL;
    }

    if (Tomb1MConfig.disable_magnums) {
        Objects[O_MAGNUM_ITEM].initialise = NULL;
        Objects[O_MAGNUM_ITEM].collision = NULL;
        Objects[O_MAGNUM_ITEM].control = NULL;
        Objects[O_MAGNUM_ITEM].draw_routine = DrawDummyItem;
        Objects[O_MAGNUM_ITEM].ceiling = NULL;
        Objects[O_MAGNUM_ITEM].floor = NULL;

        Objects[O_MAG_AMMO_ITEM].initialise = NULL;
        Objects[O_MAG_AMMO_ITEM].collision = NULL;
        Objects[O_MAG_AMMO_ITEM].control = NULL;
        Objects[O_MAG_AMMO_ITEM].draw_routine = DrawDummyItem;
        Objects[O_MAG_AMMO_ITEM].ceiling = NULL;
        Objects[O_MAG_AMMO_ITEM].floor = NULL;
    }

    if (Tomb1MConfig.disable_uzis) {
        Objects[O_UZI_ITEM].initialise = NULL;
        Objects[O_UZI_ITEM].collision = NULL;
        Objects[O_UZI_ITEM].control = NULL;
        Objects[O_UZI_ITEM].draw_routine = DrawDummyItem;
        Objects[O_UZI_ITEM].ceiling = NULL;
        Objects[O_UZI_ITEM].floor = NULL;

        Objects[O_UZI_AMMO_ITEM].initialise = NULL;
        Objects[O_UZI_AMMO_ITEM].collision = NULL;
        Objects[O_UZI_AMMO_ITEM].control = NULL;
        Objects[O_UZI_AMMO_ITEM].draw_routine = DrawDummyItem;
        Objects[O_UZI_AMMO_ITEM].ceiling = NULL;
        Objects[O_UZI_AMMO_ITEM].floor = NULL;
    }

    if (Tomb1MConfig.disable_shotgun) {
        Objects[O_SHOTGUN_ITEM].initialise = NULL;
        Objects[O_SHOTGUN_ITEM].collision = NULL;
        Objects[O_SHOTGUN_ITEM].control = NULL;
        Objects[O_SHOTGUN_ITEM].draw_routine = DrawDummyItem;
        Objects[O_SHOTGUN_ITEM].ceiling = NULL;
        Objects[O_SHOTGUN_ITEM].floor = NULL;

        Objects[O_SG_AMMO_ITEM].initialise = NULL;
        Objects[O_SG_AMMO_ITEM].collision = NULL;
        Objects[O_SG_AMMO_ITEM].control = NULL;
        Objects[O_SG_AMMO_ITEM].draw_routine = DrawDummyItem;
        Objects[O_SG_AMMO_ITEM].ceiling = NULL;
        Objects[O_SG_AMMO_ITEM].floor = NULL;
    }
}

void Tomb1MInjectGameSetup()
{
    INJECT(0x00437A50, InitialiseObjects);
}
