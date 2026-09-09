#include <trx/core/file.h>
#include <trx/game/inject.h>

static void M_HandleSymbols(
    const INJECTION_CONTEXT *const ctx, const INJECTION_CHUNK chunk)
{
    // The table is read before any chunk can refer to its identities.
    File_Skip(chunk.injection->fp, chunk.total_size);
}

REGISTER_INJECTOR(ICT_SYMBOLS, M_HandleSymbols)
