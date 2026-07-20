#include <trx/core/handle.h>

bool Handle_Equal(const TRX_HANDLE a, const TRX_HANDLE b)
{
    return a.id == b.id && a.gen == b.gen;
}

void Handle_EpochBump(HANDLE_EPOCH *const epoch)
{
    epoch->current++;
}

TRX_HANDLE Handle_EpochMint(const HANDLE_EPOCH *const epoch, const int32_t id)
{
    return (TRX_HANDLE) { .id = id, .gen = epoch->current };
}

bool Handle_EpochIsLive(
    const HANDLE_EPOCH *const epoch, const TRX_HANDLE handle)
{
    return handle.gen == epoch->current;
}

void Handle_RegistryInit(
    HANDLE_REGISTRY *const registry, uint32_t *const gens,
    const int32_t capacity)
{
    registry->gens = gens;
    registry->capacity = capacity;
    registry->next = 0;
}

uint32_t Handle_RegistryBump(HANDLE_REGISTRY *const registry, const int32_t id)
{
    // A fresh generation off the shared counter, so no two live handles in the
    // pool carry the same one even when their slots differ.
    return registry->gens[id] = ++registry->next;
}

void Handle_RegistryBumpAll(HANDLE_REGISTRY *const registry)
{
    for (int32_t i = 0; i < registry->capacity; i++) {
        registry->gens[i] = ++registry->next;
    }
}

TRX_HANDLE Handle_RegistryMint(
    const HANDLE_REGISTRY *const registry, const int32_t id)
{
    return (TRX_HANDLE) { .id = id, .gen = registry->gens[id] };
}

bool Handle_RegistryIsLive(
    const HANDLE_REGISTRY *const registry, const TRX_HANDLE handle)
{
    if (handle.id < 0 || handle.id >= registry->capacity) {
        return false;
    }
    return registry->gens[handle.id] == handle.gen;
}
