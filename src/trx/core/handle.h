#pragma once

#include <stdint.h>

// A weak reference to a game entity, of the kind a script holds across time. It
// pairs the entity's id - a pool slot, a room number, the number the owning
// module keys on - with a generation that says which occupant of that id is
// meant. The value is self-contained: copied, compared and dropped freely, with
// nothing to free. Resolving it back to a live pointer stays with the module
// that owns the entity; core settles only identity and staleness.
typedef struct {
    int32_t id;
    uint32_t gen;
} TRX_HANDLE;

// Two handles name the same occupant when both the id and the generation agree.
bool Handle_Equal(TRX_HANDLE a, TRX_HANDLE b);

// A whole-domain epoch, for entities that are all replaced at once and never
// retired one at a time - rooms, whose table is swapped at each level load. A
// single bump retires every handle the domain has handed out.
typedef struct {
    uint32_t current;
} HANDLE_EPOCH;

void Handle_EpochBump(HANDLE_EPOCH *epoch);
TRX_HANDLE Handle_EpochMint(const HANDLE_EPOCH *epoch, int32_t id);
bool Handle_EpochIsLive(const HANDLE_EPOCH *epoch, TRX_HANDLE handle);

// A per-id generation table, for a bounded pool whose slots are recycled one at
// a time - the item pool, where a freed slot is handed to the next item.
// Bumping a slot retires only the handles that named its previous occupant. The
// generation storage is the caller's, so it can sit wherever the pool already
// keeps its per-slot state and outlast a level.
typedef struct {
    uint32_t *gens;
    int32_t capacity;
    uint32_t next;
} HANDLE_REGISTRY;

// Binds the table to `capacity` slots of caller-owned, zero-initialised
// storage.
void Handle_RegistryInit(
    HANDLE_REGISTRY *registry, uint32_t *gens, int32_t capacity);
// Retires the slot's outstanding handles and returns its fresh generation.
uint32_t Handle_RegistryBump(HANDLE_REGISTRY *registry, int32_t id);
// Retires every slot at once, as when the pool is rebuilt for a new level.
void Handle_RegistryBumpAll(HANDLE_REGISTRY *registry);
// A handle for the slot's current occupant.
TRX_HANDLE Handle_RegistryMint(const HANDLE_REGISTRY *registry, int32_t id);
// Whether the handle still names the slot's current occupant.
bool Handle_RegistryIsLive(const HANDLE_REGISTRY *registry, TRX_HANDLE handle);
