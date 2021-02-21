#include "game/bat.h"
#include "game/bear.h"
#include "game/box.h"
#include "game/collide.h"
#include "game/croc.h"
#include "game/data.h"
#include "game/dino.h"
#include "game/draw.h"
#include "game/lara.h"
#include "game/lion.h"
#include "game/natla.h"
#include "game/people.h"
#include "game/rat.h"
#include "game/setup.h"
#include "game/types.h"
#include "game/warrior.h"
#include "game/wolf.h"
#include "mod.h"
#include "util.h"

void __cdecl BaddyObjects()
{
    Objects[O_LARA].initialise = InitialiseLaraLoad;
    Objects[O_LARA].draw_routine = DrawDummyItem;
    Objects[O_LARA].hit_points = LARA_HITPOINTS;
    Objects[O_LARA].shadow_size = (UNIT_SHADOW * 10) / 16;
    Objects[O_LARA].save_position = 1;
    Objects[O_LARA].save_hitpoints = 1;
    Objects[O_LARA].save_anim = 1;
    Objects[O_LARA].save_flags = 1;

    Objects[O_LARA_EXTRA].control = ControlLaraExtra;

    Objects[O_EVIL_LARA].initialise = InitialiseEvilLara;
    Objects[O_EVIL_LARA].control = ControlEvilLara;
    Objects[O_EVIL_LARA].draw_routine = DrawEvilLara;
    Objects[O_EVIL_LARA].collision = CreatureCollision;
    Objects[O_EVIL_LARA].hit_points = LARA_HITPOINTS;
    Objects[O_EVIL_LARA].shadow_size = (UNIT_SHADOW * 10) / 16;
    Objects[O_EVIL_LARA].save_position = 1;
    Objects[O_EVIL_LARA].save_hitpoints = 1;
    Objects[O_EVIL_LARA].save_flags = 1;

    if (Objects[O_WOLF].loaded) {
        Objects[O_WOLF].initialise = InitialiseWolf;
        Objects[O_WOLF].control = WolfControl;
        Objects[O_WOLF].collision = CreatureCollision;
        Objects[O_WOLF].shadow_size = UNIT_SHADOW / 2;
        Objects[O_WOLF].hit_points = WOLF_HITPOINTS;
        Objects[O_WOLF].pivot_length = 375;
        Objects[O_WOLF].radius = WOLF_RADIUS;
        Objects[O_WOLF].smartness = WOLF_SMARTNESS;
        Objects[O_WOLF].intelligent = 1;
        Objects[O_WOLF].save_position = 1;
        Objects[O_WOLF].save_hitpoints = 1;
        Objects[O_WOLF].save_anim = 1;
        Objects[O_WOLF].save_flags = 1;
        AnimBones[Objects[O_WOLF].bone_index + 8] |= BEB_ROT_Y;
    }

    if (Objects[O_BEAR].loaded) {
        Objects[O_BEAR].initialise = InitialiseCreature;
        Objects[O_BEAR].control = BearControl;
        Objects[O_BEAR].collision = CreatureCollision;
        Objects[O_BEAR].shadow_size = UNIT_SHADOW / 2;
        Objects[O_BEAR].hit_points = BEAR_HITPOINTS;
        Objects[O_BEAR].pivot_length = 500;
        Objects[O_BEAR].radius = BEAR_RADIUS;
        Objects[O_BEAR].smartness = BEAR_SMARTNESS;
        Objects[O_BEAR].intelligent = 1;
        Objects[O_BEAR].save_position = 1;
        Objects[O_BEAR].save_hitpoints = 1;
        Objects[O_BEAR].save_anim = 1;
        Objects[O_BEAR].save_flags = 1;
        AnimBones[Objects[O_BEAR].bone_index + 52] |= BEB_ROT_Y;
    }

    if (Objects[O_BAT].loaded) {
        Objects[O_BAT].initialise = InitialiseCreature;
        Objects[O_BAT].control = BatControl;
        Objects[O_BAT].collision = CreatureCollision;
        Objects[O_BAT].shadow_size = UNIT_SHADOW / 2;
        Objects[O_BAT].hit_points = BAT_HITPOINTS;
        Objects[O_BAT].radius = BAT_RADIUS;
        Objects[O_BAT].smartness = BAT_SMARTNESS;
        Objects[O_BAT].intelligent = 1;
        Objects[O_BAT].save_position = 1;
        Objects[O_BAT].save_hitpoints = 1;
        Objects[O_BAT].save_anim = 1;
        Objects[O_BAT].save_flags = 1;
    }

    if (Objects[O_DINOSAUR].loaded) {
        Objects[O_DINOSAUR].initialise = InitialiseCreature;
        Objects[O_DINOSAUR].control = DinoControl;
        Objects[O_DINOSAUR].draw_routine = DrawUnclippedItem;
        Objects[O_DINOSAUR].collision = CreatureCollision;
        Objects[O_DINOSAUR].shadow_size = UNIT_SHADOW / 2;
        Objects[O_DINOSAUR].hit_points = DINOSAUR_HITPOINTS;
        Objects[O_DINOSAUR].pivot_length = 2000;
        Objects[O_DINOSAUR].radius = DINOSAUR_RADIUS;
        Objects[O_DINOSAUR].smartness = DINOSAUR_SMARTNESS;
        Objects[O_DINOSAUR].intelligent = 1;
        Objects[O_DINOSAUR].save_position = 1;
        Objects[O_DINOSAUR].save_hitpoints = 1;
        Objects[O_DINOSAUR].save_anim = 1;
        Objects[O_DINOSAUR].save_flags = 1;
        AnimBones[Objects[O_DINOSAUR].bone_index + 40] |= BEB_ROT_Y;
        AnimBones[Objects[O_DINOSAUR].bone_index + 44] |= BEB_ROT_Y;
    }

    if (Objects[O_RAPTOR].loaded) {
        Objects[O_RAPTOR].initialise = InitialiseCreature;
        Objects[O_RAPTOR].control = RaptorControl;
        Objects[O_RAPTOR].collision = CreatureCollision;
        Objects[O_RAPTOR].shadow_size = UNIT_SHADOW / 2;
        Objects[O_RAPTOR].hit_points = RAPTOR_HITPOINTS;
        Objects[O_RAPTOR].pivot_length = 400;
        Objects[O_RAPTOR].radius = RAPTOR_RADIUS;
        Objects[O_RAPTOR].smartness = RAPTOR_SMARTNESS;
        Objects[O_RAPTOR].intelligent = 1;
        Objects[O_RAPTOR].save_position = 1;
        Objects[O_RAPTOR].save_hitpoints = 1;
        Objects[O_RAPTOR].save_anim = 1;
        Objects[O_RAPTOR].save_flags = 1;
        AnimBones[Objects[O_RAPTOR].bone_index + 84] |= BEB_ROT_Y;
    }

    if (Objects[O_LARSON].loaded) {
        Objects[O_LARSON].initialise = InitialiseCreature;
        Objects[O_LARSON].control = PeopleControl;
        Objects[O_LARSON].collision = CreatureCollision;
        Objects[O_LARSON].shadow_size = UNIT_SHADOW / 2;
        Objects[O_LARSON].hit_points = LARSON_HITPOINTS;
        Objects[O_LARSON].radius = LARSON_RADIUS;
        Objects[O_LARSON].smartness = LARSON_SMARTNESS;
        Objects[O_LARSON].intelligent = 1;
        Objects[O_LARSON].save_position = 1;
        Objects[O_LARSON].save_hitpoints = 1;
        Objects[O_LARSON].save_anim = 1;
        Objects[O_LARSON].save_flags = 1;
        AnimBones[Objects[O_LARSON].bone_index + 24] |= BEB_ROT_Y;
    }

    if (Objects[O_PIERRE].loaded) {
        Objects[O_PIERRE].initialise = InitialiseCreature;
        Objects[O_PIERRE].control = PierreControl;
        Objects[O_PIERRE].collision = CreatureCollision;
        Objects[O_PIERRE].shadow_size = UNIT_SHADOW / 2;
        Objects[O_PIERRE].hit_points = PIERRE_HITPOINTS;
        Objects[O_PIERRE].radius = PIERRE_RADIUS;
        Objects[O_PIERRE].smartness = PIERRE_SMARTNESS;
        Objects[O_PIERRE].intelligent = 1;
        Objects[O_PIERRE].save_position = 1;
        Objects[O_PIERRE].save_hitpoints = 1;
        Objects[O_PIERRE].save_anim = 1;
        Objects[O_PIERRE].save_flags = 1;
        AnimBones[Objects[O_PIERRE].bone_index + 24] |= BEB_ROT_Y;
    }

    if (Objects[O_RAT].loaded) {
        Objects[O_RAT].initialise = InitialiseCreature;
        Objects[O_RAT].control = RatControl;
        Objects[O_RAT].collision = CreatureCollision;
        Objects[O_RAT].shadow_size = UNIT_SHADOW / 2;
        Objects[O_RAT].hit_points = RAT_HITPOINTS;
        Objects[O_RAT].pivot_length = 200;
        Objects[O_RAT].radius = RAT_RADIUS;
        Objects[O_RAT].smartness = RAT_SMARTNESS;
        Objects[O_RAT].intelligent = 1;
        Objects[O_RAT].save_position = 1;
        Objects[O_RAT].save_hitpoints = 1;
        Objects[O_RAT].save_anim = 1;
        Objects[O_RAT].save_flags = 1;
        AnimBones[Objects[O_RAT].bone_index + 4] |= BEB_ROT_Y;
    }

    if (Objects[O_VOLE].loaded) {
        Objects[O_VOLE].initialise = InitialiseCreature;
        Objects[O_VOLE].control = VoleControl;
        Objects[O_VOLE].collision = CreatureCollision;
        Objects[O_VOLE].shadow_size = UNIT_SHADOW / 2;
        Objects[O_VOLE].hit_points = RAT_HITPOINTS;
        Objects[O_VOLE].pivot_length = 200;
        Objects[O_VOLE].radius = RAT_RADIUS;
        Objects[O_VOLE].smartness = RAT_SMARTNESS;
        Objects[O_VOLE].intelligent = 1;
        Objects[O_VOLE].save_position = 1;
        Objects[O_VOLE].save_hitpoints = 1;
        Objects[O_VOLE].save_anim = 1;
        Objects[O_VOLE].save_flags = 1;
        AnimBones[Objects[O_VOLE].bone_index + 4] |= BEB_ROT_Y;
    }

    if (Objects[O_LION].loaded) {
        Objects[O_LION].initialise = InitialiseCreature;
        Objects[O_LION].control = LionControl;
        Objects[O_LION].collision = CreatureCollision;
        Objects[O_LION].shadow_size = UNIT_SHADOW / 2;
        Objects[O_LION].hit_points = LION_HITPOINTS;
        Objects[O_LION].pivot_length = 400;
        Objects[O_LION].radius = LION_RADIUS;
        Objects[O_LION].smartness = LION_SMARTNESS;
        Objects[O_LION].intelligent = 1;
        Objects[O_LION].save_position = 1;
        Objects[O_LION].save_hitpoints = 1;
        Objects[O_LION].save_anim = 1;
        Objects[O_LION].save_flags = 1;
        AnimBones[Objects[O_LION].bone_index + 76] |= BEB_ROT_Y;
    }

    if (Objects[O_LIONESS].loaded) {
        Objects[O_LIONESS].initialise = InitialiseCreature;
        Objects[O_LIONESS].control = LionControl;
        Objects[O_LIONESS].collision = CreatureCollision;
        Objects[O_LIONESS].shadow_size = UNIT_SHADOW / 2;
        Objects[O_LIONESS].hit_points = LIONESS_HITPOINTS;
        Objects[O_LIONESS].pivot_length = 400;
        Objects[O_LIONESS].radius = LIONESS_RADIUS;
        Objects[O_LIONESS].smartness = LIONESS_SMARTNESS;
        Objects[O_LIONESS].intelligent = 1;
        Objects[O_LIONESS].save_position = 1;
        Objects[O_LIONESS].save_hitpoints = 1;
        Objects[O_LIONESS].save_anim = 1;
        Objects[O_LIONESS].save_flags = 1;
        AnimBones[Objects[O_LIONESS].bone_index + 76] |= BEB_ROT_Y;
    }

    if (Objects[O_PUMA].loaded) {
        Objects[O_PUMA].initialise = InitialiseCreature;
        Objects[O_PUMA].control = LionControl;
        Objects[O_PUMA].collision = CreatureCollision;
        Objects[O_PUMA].shadow_size = UNIT_SHADOW / 2;
        Objects[O_PUMA].hit_points = PUMA_HITPOINTS;
        Objects[O_PUMA].pivot_length = 400;
        Objects[O_PUMA].radius = PUMA_RADIUS;
        Objects[O_PUMA].smartness = PUMA_SMARTNESS;
        Objects[O_PUMA].intelligent = 1;
        Objects[O_PUMA].save_position = 1;
        Objects[O_PUMA].save_hitpoints = 1;
        Objects[O_PUMA].save_anim = 1;
        Objects[O_PUMA].save_flags = 1;
        AnimBones[Objects[O_PUMA].bone_index + 76] |= BEB_ROT_Y;
    }

    if (Objects[O_CROCODILE].loaded) {
        Objects[O_CROCODILE].initialise = InitialiseCreature;
        Objects[O_CROCODILE].control = CrocControl;
        Objects[O_CROCODILE].collision = CreatureCollision;
        Objects[O_CROCODILE].shadow_size = UNIT_SHADOW / 3;
        Objects[O_CROCODILE].hit_points = CROCODILE_HITPOINTS;
        Objects[O_CROCODILE].pivot_length = 600;
        Objects[O_CROCODILE].radius = CROCODILE_RADIUS;
        Objects[O_CROCODILE].smartness = CROCODILE_SMARTNESS;
        Objects[O_CROCODILE].intelligent = 1;
        Objects[O_CROCODILE].save_position = 1;
        Objects[O_CROCODILE].save_hitpoints = 1;
        Objects[O_CROCODILE].save_anim = 1;
        Objects[O_CROCODILE].save_flags = 1;
        AnimBones[Objects[O_CROCODILE].bone_index + 28] |= BEB_ROT_Y;
    }

    if (Objects[O_ALLIGATOR].loaded) {
        Objects[O_ALLIGATOR].initialise = InitialiseCreature;
        Objects[O_ALLIGATOR].control = AlligatorControl;
        Objects[O_ALLIGATOR].collision = CreatureCollision;
        Objects[O_ALLIGATOR].shadow_size = UNIT_SHADOW / 3;
        Objects[O_ALLIGATOR].hit_points = ALLIGATOR_HITPOINTS;
        Objects[O_ALLIGATOR].pivot_length = 600;
        Objects[O_ALLIGATOR].radius = ALLIGATOR_RADIUS;
        Objects[O_ALLIGATOR].smartness = ALLIGATOR_SMARTNESS;
        Objects[O_ALLIGATOR].intelligent = 1;
        Objects[O_ALLIGATOR].save_position = 1;
        Objects[O_ALLIGATOR].save_hitpoints = 1;
        Objects[O_ALLIGATOR].save_anim = 1;
        Objects[O_ALLIGATOR].save_flags = 1;
        AnimBones[Objects[O_ALLIGATOR].bone_index + 28] |= BEB_ROT_Y;
    }

    if (Objects[O_APE].loaded) {
        Objects[O_APE].initialise = InitialiseCreature;
        Objects[O_APE].control = ApeControl;
        Objects[O_APE].collision = CreatureCollision;
        Objects[O_APE].shadow_size = UNIT_SHADOW / 2;
        Objects[O_APE].hit_points = APE_HITPOINTS;
        Objects[O_APE].pivot_length = 250;
        Objects[O_APE].radius = APE_RADIUS;
        Objects[O_APE].smartness = APE_SMARTNESS;
        Objects[O_APE].intelligent = 1;
        Objects[O_APE].save_position = 1;
        Objects[O_APE].save_hitpoints = 1;
        Objects[O_APE].save_anim = 1;
        Objects[O_APE].save_flags = 1;
        AnimBones[Objects[O_APE].bone_index + 52] |= BEB_ROT_Y;
    }

    if (Objects[O_WARRIOR1].loaded) {
        Objects[O_WARRIOR1].initialise = InitialiseCreature;
        Objects[O_WARRIOR1].control = FlyerControl;
        Objects[O_WARRIOR1].collision = CreatureCollision;
        Objects[O_WARRIOR1].shadow_size = UNIT_SHADOW / 3;
        Objects[O_WARRIOR1].hit_points = FLYER_HITPOINTS;
        Objects[O_WARRIOR1].pivot_length = 150;
        Objects[O_WARRIOR1].radius = FLYER_RADIUS;
        Objects[O_WARRIOR1].smartness = FLYER_SMARTNESS;
        Objects[O_WARRIOR1].intelligent = 1;
        Objects[O_WARRIOR1].save_position = 1;
        Objects[O_WARRIOR1].save_hitpoints = 1;
        Objects[O_WARRIOR1].save_anim = 1;
        Objects[O_WARRIOR1].save_flags = 1;
        AnimBones[Objects[O_WARRIOR1].bone_index] |= BEB_ROT_Y;
        AnimBones[Objects[O_WARRIOR1].bone_index + 8] |= BEB_ROT_Y;
    }

    if (Objects[O_WARRIOR2].loaded) {
        Objects[O_WARRIOR2].initialise = InitialiseWarrior2;
        Objects[O_WARRIOR2].control = FlyerControl;
        Objects[O_WARRIOR2].collision = CreatureCollision;
        Objects[O_WARRIOR2].shadow_size = UNIT_SHADOW / 3;
        Objects[O_WARRIOR2].hit_points = FLYER_HITPOINTS;
        Objects[O_WARRIOR2].pivot_length = 150;
        Objects[O_WARRIOR2].radius = FLYER_RADIUS;
        Objects[O_WARRIOR2].smartness = WARRIOR2_SMARTNESS;
        Objects[O_WARRIOR2].intelligent = 1;
        Objects[O_WARRIOR2].save_position = 1;
        Objects[O_WARRIOR2].save_hitpoints = 1;
        Objects[O_WARRIOR2].save_anim = 1;
        Objects[O_WARRIOR2].save_flags = 1;
    }

    if (Objects[O_WARRIOR3].loaded) {
        Objects[O_WARRIOR3].initialise = InitialiseWarrior2;
        Objects[O_WARRIOR3].control = FlyerControl;
        Objects[O_WARRIOR3].collision = CreatureCollision;
        Objects[O_WARRIOR3].shadow_size = UNIT_SHADOW / 3;
        Objects[O_WARRIOR3].hit_points = FLYER_HITPOINTS;
        Objects[O_WARRIOR3].pivot_length = 150;
        Objects[O_WARRIOR3].radius = FLYER_RADIUS;
        Objects[O_WARRIOR3].smartness = FLYER_SMARTNESS;
        Objects[O_WARRIOR3].intelligent = 1;
        Objects[O_WARRIOR3].save_position = 1;
        Objects[O_WARRIOR3].save_hitpoints = 1;
        Objects[O_WARRIOR3].save_anim = 1;
        Objects[O_WARRIOR3].save_flags = 1;
    }

    if (Objects[O_CENTAUR].loaded) {
        Objects[O_CENTAUR].initialise = InitialiseCreature;
        Objects[O_CENTAUR].control = CentaurControl;
        Objects[O_CENTAUR].collision = CreatureCollision;
        Objects[O_CENTAUR].shadow_size = UNIT_SHADOW / 3;
        Objects[O_CENTAUR].hit_points = CENTAUR_HITPOINTS;
        Objects[O_CENTAUR].pivot_length = 400;
        Objects[O_CENTAUR].radius = CENTAUR_RADIUS;
        Objects[O_CENTAUR].smartness = CENTAUR_SMARTNESS;
        Objects[O_CENTAUR].intelligent = 1;
        Objects[O_CENTAUR].save_position = 1;
        Objects[O_CENTAUR].save_hitpoints = 1;
        Objects[O_CENTAUR].save_anim = 1;
        Objects[O_CENTAUR].save_flags = 1;
        AnimBones[Objects[O_CENTAUR].bone_index + 40] |= 0xCu;
    }

    if (Objects[O_MUMMY].loaded) {
        Objects[O_MUMMY].initialise = InitialiseMummy;
        Objects[O_MUMMY].control = MummyControl;
        Objects[O_MUMMY].collision = ObjectCollision;
        Objects[O_MUMMY].hit_points = MUMMY_HITPOINTS;
        Objects[O_MUMMY].save_flags = 1;
        Objects[O_MUMMY].save_hitpoints = 1;
        Objects[O_MUMMY].save_anim = 1;
        AnimBones[Objects[O_MUMMY].bone_index + 8] |= BEB_ROT_Y;
    }

    if (Objects[O_MERCENARY1].loaded) {
        Objects[O_MERCENARY1].initialise = InitialiseSkateKid;
        Objects[O_MERCENARY1].control = SkateKidControl;
        Objects[O_MERCENARY1].draw_routine = DrawSkateKid;
        Objects[O_MERCENARY1].collision = CreatureCollision;
        Objects[O_MERCENARY1].shadow_size = UNIT_SHADOW / 2;
        Objects[O_MERCENARY1].hit_points = SKATEKID_HITPOINTS;
        Objects[O_MERCENARY1].radius = SKATEKID_RADIUS;
        Objects[O_MERCENARY1].smartness = SKATEKID_SMARTNESS;
        Objects[O_MERCENARY1].intelligent = 1;
        Objects[O_MERCENARY1].save_position = 1;
        Objects[O_MERCENARY1].save_hitpoints = 1;
        Objects[O_MERCENARY1].save_anim = 1;
        Objects[O_MERCENARY1].save_flags = 1;
        AnimBones[Objects[O_MERCENARY1].bone_index] |= BEB_ROT_Y;
    }

    if (Objects[O_MERCENARY2].loaded) {
        Objects[O_MERCENARY2].initialise = InitialiseCreature;
        Objects[O_MERCENARY2].control = CowboyControl;
        Objects[O_MERCENARY2].collision = CreatureCollision;
        Objects[O_MERCENARY2].shadow_size = UNIT_SHADOW / 2;
        Objects[O_MERCENARY2].hit_points = COWBOY_HITPOINTS;
        Objects[O_MERCENARY2].radius = COWBOY_RADIUS;
        Objects[O_MERCENARY2].smartness = COWBOY_SMARTNESS;
        Objects[O_MERCENARY2].intelligent = 1;
        Objects[O_MERCENARY2].save_position = 1;
        Objects[O_MERCENARY2].save_hitpoints = 1;
        Objects[O_MERCENARY2].save_anim = 1;
        Objects[O_MERCENARY2].save_flags = 1;
        AnimBones[Objects[O_MERCENARY2].bone_index] |= BEB_ROT_Y;
    }

    if (Objects[O_MERCENARY3].loaded) {
        Objects[O_MERCENARY3].initialise = InitialiseBaldy;
        Objects[O_MERCENARY3].control = BaldyControl;
        Objects[O_MERCENARY3].collision = CreatureCollision;
        Objects[O_MERCENARY3].shadow_size = UNIT_SHADOW / 2;
        Objects[O_MERCENARY3].hit_points = BALDY_HITPOINTS;
        Objects[O_MERCENARY3].radius = BALDY_RADIUS;
        Objects[O_MERCENARY3].smartness = BALDY_SMARTNESS;
        Objects[O_MERCENARY3].intelligent = 1;
        Objects[O_MERCENARY3].save_position = 1;
        Objects[O_MERCENARY3].save_hitpoints = 1;
        Objects[O_MERCENARY3].save_anim = 1;
        Objects[O_MERCENARY3].save_flags = 1;
        AnimBones[Objects[O_MERCENARY3].bone_index] |= BEB_ROT_Y;
    }

    if (Objects[O_EVIL_NATLA].loaded) {
        Objects[O_EVIL_NATLA].initialise = InitialiseCreature;
        Objects[O_EVIL_NATLA].control = AbortionControl;
        Objects[O_EVIL_NATLA].collision = CreatureCollision;
        Objects[O_EVIL_NATLA].shadow_size = UNIT_SHADOW / 3;
        Objects[O_EVIL_NATLA].hit_points = ABORTION_HITPOINTS;
        Objects[O_EVIL_NATLA].radius = ABORTION_RADIUS;
        Objects[O_EVIL_NATLA].smartness = ABORTION_SMARTNESS;
        Objects[O_EVIL_NATLA].intelligent = 1;
        Objects[O_EVIL_NATLA].save_position = 1;
        Objects[O_EVIL_NATLA].save_hitpoints = 1;
        Objects[O_EVIL_NATLA].save_anim = 1;
        Objects[O_EVIL_NATLA].save_flags = 1;
        AnimBones[Objects[O_EVIL_NATLA].bone_index + 4] |= BEB_ROT_Y;
    }

    if (Objects[O_NATLA].loaded) {
        Objects[O_NATLA].collision = CreatureCollision;
        Objects[O_NATLA].initialise = InitialiseCreature;
        Objects[O_NATLA].control = NatlaControl;
        Objects[O_NATLA].shadow_size = UNIT_SHADOW / 2;
        Objects[O_NATLA].hit_points = NATLA_HITPOINTS;
        Objects[O_NATLA].radius = NATLA_RADIUS;
        Objects[O_NATLA].smartness = NATLA_SMARTNESS;
        Objects[O_NATLA].intelligent = 1;
        Objects[O_NATLA].save_position = 1;
        Objects[O_NATLA].save_hitpoints = 1;
        Objects[O_NATLA].save_anim = 1;
        Objects[O_NATLA].save_flags = 1;
        AnimBones[Objects[O_NATLA].bone_index + 8] |= BEB_ROT_Z | BEB_ROT_X;
    }

    if (Objects[O_PODS].loaded) {
        Objects[O_PODS].initialise = InitialisePod;
        Objects[O_PODS].control = PodControl;
        Objects[O_PODS].collision = ObjectCollision;
        Objects[O_PODS].save_anim = 1;
        Objects[O_PODS].save_flags = 1;
    }

    if (Objects[O_BIG_POD].loaded) {
        Objects[O_BIG_POD].initialise = InitialisePod;
        Objects[O_BIG_POD].control = PodControl;
        Objects[O_BIG_POD].collision = ObjectCollision;
        Objects[O_BIG_POD].save_anim = 1;
        Objects[O_BIG_POD].save_flags = 1;
    }

    if (Objects[O_STATUE].loaded) {
        Objects[O_STATUE].initialise = InitialiseStatue;
        Objects[O_STATUE].control = StatueControl;
        Objects[O_STATUE].collision = ObjectCollision;
        Objects[O_STATUE].save_anim = 1;
        Objects[O_STATUE].save_flags = 1;
    }
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

#ifdef TOMB1M_FEAT_GAMEPLAY
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
#endif
}

void Tomb1MInjectGameSetup()
{
    INJECT(0x004363E0, BaddyObjects);
    INJECT(0x00437A50, InitialiseObjects);
}
