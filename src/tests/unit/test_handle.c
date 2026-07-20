#include "harness.h"

#include <trx/core/handle.h>

TEST(equal_handles_agree_on_id_and_generation)
{
    CHECK(Handle_Equal((TRX_HANDLE) { 3, 7 }, (TRX_HANDLE) { 3, 7 }));
    CHECK(!Handle_Equal((TRX_HANDLE) { 3, 7 }, (TRX_HANDLE) { 3, 8 }));
    CHECK(!Handle_Equal((TRX_HANDLE) { 3, 7 }, (TRX_HANDLE) { 4, 7 }));
}

TEST(an_epoch_retires_every_handle_at_once)
{
    HANDLE_EPOCH epoch = {};
    const TRX_HANDLE a = Handle_EpochMint(&epoch, 0);
    const TRX_HANDLE b = Handle_EpochMint(&epoch, 5);
    CHECK(Handle_EpochIsLive(&epoch, a));
    CHECK(Handle_EpochIsLive(&epoch, b));

    Handle_EpochBump(&epoch);
    CHECK(!Handle_EpochIsLive(&epoch, a));
    CHECK(!Handle_EpochIsLive(&epoch, b));

    // A handle minted in the new epoch is live, regardless of its id.
    CHECK(Handle_EpochIsLive(&epoch, Handle_EpochMint(&epoch, 0)));
}

TEST(a_registry_retires_one_slot_without_touching_others)
{
    uint32_t gens[4] = {};
    HANDLE_REGISTRY reg;
    Handle_RegistryInit(&reg, gens, 4);

    const TRX_HANDLE a = Handle_RegistryMint(&reg, 1);
    const TRX_HANDLE b = Handle_RegistryMint(&reg, 2);

    Handle_RegistryBump(&reg, 1);
    CHECK(!Handle_RegistryIsLive(&reg, a));
    CHECK(Handle_RegistryIsLive(&reg, b));

    // The slot's new occupant has a handle of its own that resolves.
    CHECK(Handle_RegistryIsLive(&reg, Handle_RegistryMint(&reg, 1)));
}

TEST(a_recycled_slot_never_matches_its_previous_occupant)
{
    uint32_t gens[4] = {};
    HANDLE_REGISTRY reg;
    Handle_RegistryInit(&reg, gens, 4);

    const TRX_HANDLE old = Handle_RegistryMint(&reg, 2);
    Handle_RegistryBump(&reg, 2);
    const TRX_HANDLE fresh = Handle_RegistryMint(&reg, 2);

    CHECK(old.id == fresh.id);
    CHECK(!Handle_Equal(old, fresh));
}

TEST(bump_all_retires_the_whole_pool)
{
    uint32_t gens[4] = {};
    HANDLE_REGISTRY reg;
    Handle_RegistryInit(&reg, gens, 4);

    const TRX_HANDLE handles[] = {
        Handle_RegistryMint(&reg, 0),
        Handle_RegistryMint(&reg, 3),
    };
    Handle_RegistryBumpAll(&reg);
    CHECK(!Handle_RegistryIsLive(&reg, handles[0]));
    CHECK(!Handle_RegistryIsLive(&reg, handles[1]));
}

TEST(an_out_of_range_handle_is_never_live)
{
    uint32_t gens[2] = {};
    HANDLE_REGISTRY reg;
    Handle_RegistryInit(&reg, gens, 2);
    CHECK(!Handle_RegistryIsLive(&reg, (TRX_HANDLE) { -1, 0 }));
    CHECK(!Handle_RegistryIsLive(&reg, (TRX_HANDLE) { 2, 0 }));
}
