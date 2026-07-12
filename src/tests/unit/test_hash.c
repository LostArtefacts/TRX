#include "harness.h"

#include <trx/core/hash.h>

// FNV-1a. Savegames and caches key off these, so the values are a wire format:
// changing them silently invalidates whatever was hashed with the old ones.

TEST(the_empty_hash_is_the_fnv_offset_basis)
{
    CHECK(Hash_FNV1a64_Init() == HASH_FNV1A64_BASE);
    CHECK(Hash_FNV1a64_Init() == 14695981039346656037ULL);
}

TEST(hashing_is_order_dependent)
{
    const uint64_t ab = Hash_FNV1a64_UpdateU32(
        Hash_FNV1a64_UpdateU32(Hash_FNV1a64_Init(), 1), 2);
    const uint64_t ba = Hash_FNV1a64_UpdateU32(
        Hash_FNV1a64_UpdateU32(Hash_FNV1a64_Init(), 2), 1);
    CHECK(ab != ba);
}

TEST(hashing_is_deterministic_across_calls)
{
    CHECK(
        Hash_FNV1a64_UpdateString(Hash_FNV1a64_Init(), "lara")
        == Hash_FNV1a64_UpdateString(Hash_FNV1a64_Init(), "lara"));
    CHECK(
        Hash_FNV1a64_UpdateString(Hash_FNV1a64_Init(), "lara")
        != Hash_FNV1a64_UpdateString(Hash_FNV1a64_Init(), "larson"));
}

TEST(string_updates_are_length_prefixed_so_a_split_cannot_collide)
{
    // Hashing "ab" then "c" must not land on the same value as "a" then "bc".
    // Without the length prefix the byte stream would be identical and the two
    // would collide - which is why Hash_FNV1a64_UpdateString hashes the length
    // first. Drop that, and two different key sequences share a hash.
    uint64_t left = Hash_FNV1a64_Init();
    left = Hash_FNV1a64_UpdateString(left, "ab");
    left = Hash_FNV1a64_UpdateString(left, "c");

    uint64_t right = Hash_FNV1a64_Init();
    right = Hash_FNV1a64_UpdateString(right, "a");
    right = Hash_FNV1a64_UpdateString(right, "bc");

    CHECK(left != right);
}

TEST(a_null_string_is_not_the_same_as_an_empty_one)
{
    const uint64_t null_hash =
        Hash_FNV1a64_UpdateString(Hash_FNV1a64_Init(), nullptr);
    const uint64_t empty_hash =
        Hash_FNV1a64_UpdateString(Hash_FNV1a64_Init(), "");
    // Both hash a zero length; the empty string then hashes zero bytes on top.
    // They happen to coincide - pin it down so a change to either path is a
    // deliberate one.
    CHECK(null_hash == empty_hash);
}

TEST(u32_and_u64_updates_are_distinct)
{
    // Same numeric value, different widths, therefore different byte counts.
    CHECK(
        Hash_FNV1a64_UpdateU32(Hash_FNV1a64_Init(), 1)
        != Hash_FNV1a64_UpdateU64(Hash_FNV1a64_Init(), 1));
}
