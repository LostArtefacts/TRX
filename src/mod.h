#ifndef TR1MAIN_MOD_H
#define TR1MAIN_MOD_H

#include "util.h"

struct {
    int disable_healing_between_levels;
    int disable_medpacks;
    int fix_end_of_level_freeze;
} TR1MConfig;

struct {
    int stored_lara_health;
} TR1MData;

#endif
