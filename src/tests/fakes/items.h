#pragma once

#include <trx/game/objects/ids.h>

#include <stdint.h>

#define FAKE_ITEM_POOL 8 // small, so pool exhaustion is reachable
#define FAKE_OBJ_WOLF 1 // intelligent, has animations
#define FAKE_OBJ_VASE 2 // inert scenery, and a pickup
#define FAKE_OBJ_UNLOADED 3 // declared but not loaded
#define FAKE_OBJ_KEY 4 // a second pickup, so a group name matches more than one
#define FAKE_OBJ_SPRITE 8 // drawn from a sprite rather than from meshes

// Family membership with nothing else to it: these are declared but never
// loaded, so they say which family they are in without joining the counts the
// tests take over the level's own objects.
#define FAKE_OBJ_SWITCH 5
#define FAKE_OBJ_RECEPTACLE 6
#define FAKE_OBJ_DOOR 7

// Real ids, because the pickup families come from pickups.def and a made-up id
// is in none of them. A command that selects on a family needs this level to
// hold something the family names.
#define FAKE_OBJ_REAL_KEY O_KEY_ITEM_1
#define FAKE_OBJ_PUZZLE O_PUZZLE_ITEM_1
#define FAKE_OBJ_TOOL O_CROWBAR_ITEM
// A tool a level sends Lara to find, rather than one she carries and uses.
#define FAKE_OBJ_LEADBAR O_LEAD_BAR_ITEM
#define FAKE_OBJ_MEDIPACK O_SMALL_MEDIPACK_ITEM
// A pickup a family names but no cheat hands over.
#define FAKE_OBJ_TRINKET O_SECRET_1
// Two states of one thing, sharing a backpack entry and a name; and a variant
// in no family at all, only the base it stands in for.
#define FAKE_OBJ_SCION O_SCION_ITEM_1
#define FAKE_OBJ_SCION_2 O_SCION_ITEM_2
#define FAKE_OBJ_WATERSKIN O_WATERSKIN_1_1
// Not a pickup: an inventory icon with a control routine of its own.
#define FAKE_OBJ_CRYSTAL O_SAVE_CRYSTAL_ITEM

// Puts a savegame crystal in the level, far from the other pickups, and a scion
// further out still.
void FakeItems_PlaceCrystal(void);
void FakeItems_PlaceScion(void);
