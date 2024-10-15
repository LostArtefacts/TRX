#include "game/objects/creatures/worker.h"

#include "game/creature.h"
#include "game/objects/common.h"
#include "global/const.h"
#include "global/funcs.h"
#include "global/vars.h"

#define WORKER_1_HITPOINTS 25
#define WORKER_2_HITPOINTS 20
#define WORKER_3_HITPOINTS 27
#define WORKER_4_HITPOINTS 27
#define WORKER_5_HITPOINTS 20
#define WORKER_RADIUS (WALL_L / 10) // = 102

void Worker1_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_WORKER_1];
    if (!obj->loaded) {
        return;
    }

    obj->control = Worker1_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = WORKER_1_HITPOINTS;
    obj->radius = WORKER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 16] |= BF_ROT_Y;
    g_AnimBones[obj->bone_idx + 52] |= BF_ROT_Y;
}

void Worker2_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_WORKER_2];
    if (!obj->loaded) {
        return;
    }

    obj->control = Worker2_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = WORKER_2_HITPOINTS;
    obj->radius = WORKER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 16] |= BF_ROT_Y;
    g_AnimBones[obj->bone_idx + 52] |= BF_ROT_Y;
}

void Worker3_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_WORKER_3];
    if (!obj->loaded) {
        return;
    }

    obj->control = Worker3_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = WORKER_3_HITPOINTS;
    obj->radius = WORKER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx] |= BF_ROT_Y;
    g_AnimBones[obj->bone_idx + 16] |= BF_ROT_Y;
}

void Worker4_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_WORKER_4];
    if (!obj->loaded) {
        return;
    }

    obj->control = Worker3_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = WORKER_4_HITPOINTS;
    obj->radius = WORKER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx] |= BF_ROT_Y;
    g_AnimBones[obj->bone_idx + 16] |= BF_ROT_Y;
}

void Worker5_Setup(void)
{
    OBJECT *const obj = &g_Objects[O_WORKER_5];
    if (!obj->loaded) {
        return;
    }

    obj->control = Worker2_Control;
    obj->collision = Creature_Collision;

    obj->hit_points = WORKER_5_HITPOINTS;
    obj->radius = WORKER_RADIUS;
    obj->shadow_size = UNIT_SHADOW / 2;
    obj->pivot_length = 0;

    obj->intelligent = 1;
    obj->save_anim = 1;
    obj->save_position = 1;
    obj->save_hitpoints = 1;
    obj->save_flags = 1;

    g_AnimBones[obj->bone_idx + 16] |= BF_ROT_Y;
}
