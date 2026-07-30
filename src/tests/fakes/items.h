#pragma once

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
