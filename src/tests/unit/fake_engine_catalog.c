// The slot mapping, faked. The real one is loaded per game from a CSV; this one
// is arithmetic, so the test pins the plumbing rather than TR1's object table.
//
// Only the objects catalog is mapped. The others stand for a catalog this game
// has nothing in, which is the case a script has to handle.

#include <trx/game/catalog/manager.h>

#define FAKE_SLOT_OFFSET 13

bool Catalog_EnumToGameID(
    const CATALOG_CONTEXT context, const CATALOG_ID id,
    int32_t *const out_game_id)
{
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
