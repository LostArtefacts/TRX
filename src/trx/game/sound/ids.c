#include <trx/game/catalog/manager.h>
#include <trx/game/sound.h>

SAMPLE_SLOT Sound_IDToSlot(const SAMPLE_ID id)
{
    return Catalog_IDToSlot(CATALOG_SAMPLES, id, SFX_INVALID);
}

SAMPLE_ID Sound_SlotToID(const SAMPLE_SLOT slot)
{
    return Catalog_SlotToID(CATALOG_SAMPLES, slot, NO_CATALOG_ID);
}
