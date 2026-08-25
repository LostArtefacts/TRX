#include <trx/game/catalog/manager.h>
#include <trx/game/sound.h>

SAMPLE_ID Sound_ToGameID(const SAMPLE_TRX_ID trx_id)
{
    return Catalog_ToSlot(CATALOG_SAMPLES, trx_id, SFX_INVALID);
}

SAMPLE_TRX_ID Sound_FromGameID(const SAMPLE_ID sample_id)
{
    return Catalog_FromSlot(CATALOG_SAMPLES, sample_id, SFX_TRX_INVALID);
}
