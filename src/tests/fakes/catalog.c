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

int32_t Catalog_ToSlot(
    const CATALOG_CONTEXT context, const CATALOG_ID id, const int32_t fallback)
{
    if (context == CATALOG_SAMPLES) {
        return FAKE_SAMPLE;
    }
    if (context != CATALOG_OBJECTS) {
        return fallback;
    }
    return id + FAKE_SLOT_OFFSET;
}

CATALOG_ID Catalog_FromSlot(
    const CATALOG_CONTEXT context, const int32_t slot,
    const CATALOG_ID fallback)
{
    if (context != CATALOG_OBJECTS || slot < FAKE_SLOT_OFFSET) {
        return fallback;
    }
    return slot - FAKE_SLOT_OFFSET;
}
