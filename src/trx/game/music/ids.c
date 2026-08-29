#include <trx/game/catalog/manager.h>
#include <trx/game/music.h>

MUSIC_SLOT Music_IDToSlot(const MUSIC_ID id)
{
    return Catalog_IDToSlot(CATALOG_MUSIC, id, MX_INACTIVE);
}

MUSIC_ID Music_SlotToID(const MUSIC_SLOT slot)
{
    return Catalog_SlotToID(CATALOG_MUSIC, slot, NO_CATALOG_ID);
}
