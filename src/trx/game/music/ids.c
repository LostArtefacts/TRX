#include <trx/game/catalog/manager.h>
#include <trx/game/music.h>

MUSIC_ID Music_ToGameID(const MUSIC_TRX_ID music_track)
{
    return Catalog_ToSlot(CATALOG_MUSIC, music_track, MX_INACTIVE);
}

MUSIC_TRX_ID Music_FromGameID(const MUSIC_ID track_id)
{
    return Catalog_FromSlot(CATALOG_MUSIC, track_id, MX_TRX_INVALID);
}
