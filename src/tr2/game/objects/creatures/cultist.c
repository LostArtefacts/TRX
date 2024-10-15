#include "game/objects/creatures/cultist.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#include <assert.h>

#define CULTIST_1_HITPOINTS 25
#define CULTIST_2_HITPOINTS 60
#define CULTIST_3_HITPOINTS 150
#define CULTIST_RADIUS (WALL_L / 10) // = 102

void Cultist1_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_CULT_1];
    if (!obj->loaded) {
        return;
    }

    obj->initialise = Cultist1_Initialise;
    obj->control = Cultist1_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = CULTIST_1_HITPOINTS;
    obj->radius = CULTIST_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx] |= BF_ROT_Y;
}

void Cultist1A_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_CULT_1A];
    if (!obj->loaded) {
        return;
    }

    assert(g_Objects[O_CULT_1].loaded);
    obj->frame_base = g_Objects[O_CULT_1].frame_base;
    obj->anim_idx = g_Objects[O_CULT_1].anim_idx;

    obj->initialise = Cultist1_Initialise;
    obj->control = Cultist1_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = CULTIST_1_HITPOINTS;
    obj->radius = CULTIST_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx] |= BF_ROT_Y;
}

void Cultist1B_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_CULT_1B];
    if (!obj->loaded) {
        return;
    }

    assert(g_Objects[O_CULT_1].loaded);
    obj->frame_base = g_Objects[O_CULT_1].frame_base;
    obj->anim_idx = g_Objects[O_CULT_1].anim_idx;

    obj->initialise = Cultist1_Initialise;
    obj->control = Cultist1_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = CULTIST_1_HITPOINTS;
    obj->radius = CULTIST_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx] |= BF_ROT_Y;
}

void Cultist2_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_CULT_2];
    if (!obj->loaded) {
        return;
    }

    obj->control = Cultist2_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = CULTIST_2_HITPOINTS;
    obj->radius = CULTIST_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 50;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx] |= BF_ROT_Y;
    g_AnimBones[obj->bone_idx + 32] |= BF_ROT_Y;
}

void Cultist3_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_CULT_3];
    if (!obj->loaded) {
        return;
    }

    obj->initialise = Cultist3_Initialise;
    obj->control = Cultist3_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = CULTIST_3_HITPOINTS;
    obj->radius = CULTIST_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;
}
