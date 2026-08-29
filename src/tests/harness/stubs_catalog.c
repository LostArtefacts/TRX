// A table indexed by a catalog id asks how many identities a context holds,
// which is what bounds the id. Standing that up here keeps the unit tests
// engine-free, the same reason the other stubs here exist.

#include <trx/game/catalog/manager.h>
#include <trx/game/objects/ids.h>

int32_t Catalog_GetCount(const CATALOG_CONTEXT context)
{
    return context == CATALOG_OBJECTS ? O_NUMBER_OF : 0;
}

const char *Catalog_IDToKey(const CATALOG_CONTEXT context, const CATALOG_ID id)
{
    // Every id the stub reports is one the exe names, so none is anonymous.
    return id >= 0 && id < Catalog_GetCount(context) ? "" : nullptr;
}
