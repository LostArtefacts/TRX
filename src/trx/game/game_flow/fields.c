// How to reach the members of GF_LEVEL. Offsets and storage types, nothing
// else: which of these are public, under what name, and what they mean is
// declared in data/scripting/game.lua.

#include <trx/core/field.h>
#include <trx/game/game_flow/common.h>
#include <trx/game/game_flow/types.h>

// The number the level goes by, which is not the same as its place in the
// table: skipped levels do not count, and a gym level has no number at all.
static bool M_GetOrdinal(const void *const self, FIELD_VALUE *const out)
{
    const GF_LEVEL *const level = self;
    *out = (FIELD_VALUE) {
        .type = FT_INT32,
        .as_int =
            GF_GetLevelOrdinalNumber(GF_GetLevelTableType(level->type), level),
    };
    return true;
}

// clang-format off
static const FIELD_DESC M_GF_LEVEL_FIELDS[] = {
    // what the level is
    FIELD_FN("num",        FT_INT32, M_GetOrdinal, nullptr),
    FIELD_RO(GF_LEVEL, type),
    FIELD_RO(GF_LEVEL, path),
    FIELD_RO(GF_LEVEL, title),
    FIELD_RO(GF_LEVEL, script_path),
    FIELD_RO(GF_LEVEL, lara_outfit),
    FIELD_RO(GF_LEVEL, music_track),
    FIELD_RO(GF_LEVEL, water_particles),

    // what the stats screen must not count against the player
    FIELD_RO(GF_LEVEL, unobtainable.pickups),
    FIELD_RO(GF_LEVEL, unobtainable.kills),
    FIELD_RO(GF_LEVEL, unobtainable.ally_kills),
    FIELD_RO(GF_LEVEL, unobtainable.secrets),

    // Every member here is read-only, and deliberately: a level is what the game
    // flow file says it is. The sequence, the injections and the item drops are
    // not exposed at all - they are the level's program and its load-time data,
    // and neither is a contract. The sequence in particular is due to be
    // rewritten as a coroutine, and declaring its shape now would freeze it.
};
// clang-format on

TYPE_DEFINE(GF_LEVEL, M_GF_LEVEL_FIELDS)
