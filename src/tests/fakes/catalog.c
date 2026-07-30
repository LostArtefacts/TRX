// The slot mapping, faked. The real one is loaded per game from a CSV; this one
// is arithmetic, so the test pins the plumbing rather than TR1's object table.
//
// The objects catalog is mapped one for one. The samples catalog stands for a
// game carrying a single sample, which every catalog id maps onto: that is
// enough for a test to see that a sound was asked for. Music and the rest stand
// for a catalog this game has nothing in, which is the case a script has to
// handle.

#include <fakes/sound.h>

#include <trx/game/catalog/manager.h>

#define FAKE_SLOT_OFFSET 13

bool Catalog_EnumToGameID(
    const CATALOG_CONTEXT context, const CATALOG_ID id,
    int32_t *const out_game_id)
{
    if (context == CATALOG_SAMPLES) {
        *out_game_id = FAKE_SAMPLE;
        return true;
    }
    if (context != CATALOG_OBJECTS) {
        return false;
    }
    *out_game_id = id + FAKE_SLOT_OFFSET;
    return true;
}

bool Catalog_GameIDToEnum(
    const CATALOG_CONTEXT context, const int32_t game_id,
    CATALOG_ID *const out_id)
{
    if (context != CATALOG_OBJECTS || game_id < FAKE_SLOT_OFFSET) {
        return false;
    }
    *out_id = game_id - FAKE_SLOT_OFFSET;
    return true;
}
