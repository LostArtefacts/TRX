// The slot mapping, faked. The real one is loaded per game from a CSV; this one
// is arithmetic, so the test pins the plumbing rather than TR1's object table.
//
// The objects catalog is mapped one for one. The samples catalog stands for a
// game carrying a single sample, which every catalog id maps onto: that is
// enough for a test to see that a sound was asked for. Music and the rest stand
// for a catalog this game has nothing in, which is the case a script has to
// handle.

#include <fakes/sound.h>

#include <trx/core/utils.h>
#include <trx/game/catalog/manager.h>

#include <string.h>

#define FAKE_SLOT_OFFSET 13

// Preserve each identity because its position in the context's numerically
// ordered .def names is its ID.
#define X_CATALOG_ID(enum_value) #enum_value,
static const char *const m_Objects[] = {
#include <trx/game/catalog/objects.def>
};
static const char *const m_Samples[] = {
#include <trx/game/catalog/samples.def>
};
static const char *const m_Music[] = {
#include <trx/game/catalog/music.def>
};
static const char *const m_LaraStates[] = {
#include <trx/game/catalog/lara_states.def>
};
static const char *const m_LaraAnims[] = {
#include <trx/game/catalog/lara_anims.def>
};
static const char *const m_ItemActions[] = {
#include <trx/game/catalog/item_actions.def>
};
#undef X_CATALOG_ID

static const char *const *M_Names(
    const CATALOG_CONTEXT context, int32_t *const out_count)
{
    switch (context) {
    case CATALOG_OBJECTS:
        *out_count = ARRAY_SIZE(m_Objects);
        return m_Objects;
    case CATALOG_SAMPLES:
        *out_count = ARRAY_SIZE(m_Samples);
        return m_Samples;
    case CATALOG_MUSIC:
        *out_count = ARRAY_SIZE(m_Music);
        return m_Music;
    case CATALOG_LARA_STATES:
        *out_count = ARRAY_SIZE(m_LaraStates);
        return m_LaraStates;
    case CATALOG_LARA_ANIMS:
        *out_count = ARRAY_SIZE(m_LaraAnims);
        return m_LaraAnims;
    case CATALOG_ITEM_ACTIONS:
        *out_count = ARRAY_SIZE(m_ItemActions);
        return m_ItemActions;
    default:
        *out_count = 0;
        return nullptr;
    }
}

int32_t Catalog_GetCount(const CATALOG_CONTEXT context)
{
    int32_t count;
    M_Names(context, &count);
    return count;
}

const char *Catalog_GetKey(const CATALOG_CONTEXT context, const CATALOG_ID id)
{
    int32_t count;
    const char *const *const names = M_Names(context, &count);
    if (names == nullptr || id < 0 || id >= count) {
        return nullptr;
    }
    return Catalog_KeyForEnum(context, names[id]);
}

CATALOG_ID Catalog_FromKey(
    const CATALOG_CONTEXT context, const char *const key,
    const CATALOG_ID fallback)
{
    int32_t count;
    const char *const *const names = M_Names(context, &count);
    for (CATALOG_ID id = 0; id < count; id++) {
        if (strcmp(Catalog_KeyForEnum(context, names[id]), key) == 0) {
            return id;
        }
    }
    return fallback;
}

int32_t Catalog_ToSlot(
    const CATALOG_CONTEXT context, const CATALOG_ID id, const int32_t fallback)
{
    if (context == CATALOG_SAMPLES) {
        return FAKE_SAMPLE;
    }
    if (context != CATALOG_OBJECTS) {
        return fallback;
    }
    return id + FAKE_SLOT_OFFSET;
}

CATALOG_ID Catalog_FromSlot(
    const CATALOG_CONTEXT context, const int32_t slot,
    const CATALOG_ID fallback)
{
    if (context != CATALOG_OBJECTS || slot < FAKE_SLOT_OFFSET) {
        return fallback;
    }
    return slot - FAKE_SLOT_OFFSET;
}
