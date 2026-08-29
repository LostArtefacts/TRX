// A table indexed by a catalog id asks how many identities a context holds,
// which is what bounds the id. Standing that up here keeps the unit tests
// engine-free, the same reason the other stubs here exist.

#include <trx/game/catalog/manager.h>
#include <harness/fake_objects.h>

int32_t Catalog_GetCount(const CATALOG_CONTEXT context)
{
    return FAKE_OBJ_COUNT;
}

int32_t Catalog_GetBuiltInCount(const CATALOG_CONTEXT context)
{
    // Report only built-in identities because the stub creates no minted
    // identities.
    return Catalog_GetCount(context);
}

bool Catalog_IsValidID(const CATALOG_CONTEXT context, const CATALOG_ID id)
{
    return id >= 0 && id < Catalog_GetCount(context);
}

const char *Catalog_IDToKey(const CATALOG_CONTEXT context, const CATALOG_ID id)
{
    // Every id the stub reports is one the exe names, so none is anonymous.
    return id >= 0 && id < Catalog_GetCount(context) ? "" : nullptr;
}
