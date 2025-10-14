#include "game/catalog.h"
#include "game/sound.h"

SAMPLE_ID Sound_ToGameID(const SAMPLE_TRX_ID trx_id)
{
    int32_t out;
    if (Catalog_EnumToGameID(CATALOG_SAMPLES, trx_id, &out)) {
        return out;
    }
    return SFX_INVALID;
}
