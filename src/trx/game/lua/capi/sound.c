#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/sound/common.h>

#include <lauxlib.h>

// A sample handle addresses a sample by id, and exists only while the sample is
// available: a handle to a sample the loaded level does not carry goes stale.
// Its members are widened from SAMPLE_INFO so the field reflection can name
// them.
typedef struct {
    int32_t id;
    int32_t volume;
    int32_t range;
    int32_t randomness;
    int32_t pitch;
} SOUND_SAMPLE_VIEW;

static SOUND_SAMPLE_VIEW m_SampleView;

// A stream handle addresses an active-sound slot; when the slot falls silent,
// the handle goes stale.
typedef struct {
    int32_t sample_id;
} SOUND_STREAM_VIEW;

static SOUND_STREAM_VIEW m_StreamView;

// clang-format off
static const FIELD_DESC m_SampleFields[] = {
    FIELD_RO(SOUND_SAMPLE_VIEW, id),
    FIELD_RO(SOUND_SAMPLE_VIEW, volume),
    FIELD_RO(SOUND_SAMPLE_VIEW, range),
    FIELD_RO(SOUND_SAMPLE_VIEW, randomness),
    FIELD_RO(SOUND_SAMPLE_VIEW, pitch),
};

static const FIELD_DESC m_StreamFields[] = {
    FIELD_RO(SOUND_STREAM_VIEW, sample_id),
};
// clang-format on

TYPE_DEFINE(SOUND_SAMPLE_VIEW, m_SampleFields)
TYPE_DEFINE(SOUND_STREAM_VIEW, m_StreamFields)

static void *M_ResolveSample(const LUA_STRUCT_REF *const ref)
{
    const int32_t id = ref->idx;
    if (id < 0 || !Sound_IsAvailable_Direct((SAMPLE_ID)id)) {
        return nullptr;
    }
    const SAMPLE_INFO *const sample = Sound_GetSample((SAMPLE_ID)id);
    if (sample == nullptr) {
        return nullptr;
    }
    m_SampleView = (SOUND_SAMPLE_VIEW) {
        .id = id,
        .volume = sample->volume,
        .range = sample->range,
        .randomness = sample->randomness,
        .pitch = sample->pitch,
    };
    return &m_SampleView;
}

static void *M_ResolveStream(const LUA_STRUCT_REF *const ref)
{
    SAMPLE_ID sample_id;
    if (!Sound_GetActiveSlot(ref->idx, &sample_id)) {
        return nullptr;
    }
    m_StreamView.sample_id = sample_id;
    return &m_StreamView;
}

// sample:play([opts])
static int M_L_SoundSamplePlay(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_SAMPLE_VIEW);
    LUA_Struct_Deref(L, ref);
    XYZ_32 pos;
    const XYZ_32 *pos_ptr = nullptr;
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        if (lua_getfield(L, 2, "pos") == LUA_TTABLE) {
            pos = LUA_CheckXYZAt(L, -1, 2);
            pos_ptr = &pos;
        }
        lua_pop(L, 1);
    }
    Sound_Effect_Direct(
        (SAMPLE_ID)ref->idx, pos_ptr, SPM_ALWAYS | SPM_STATIC_POS);
    return 0;
}

static const luaL_Reg m_SampleMethods[] = {
    { "play", M_L_SoundSamplePlay },
    { nullptr, nullptr },
};

// stream:stop()
static int M_L_SoundStreamStop(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_StopActiveSlot(ref->idx);
    return 0;
}

// stream:pause()
static int M_L_SoundStreamPause(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_PauseActiveSlot(ref->idx);
    return 0;
}

// stream:unpause()
static int M_L_SoundStreamUnpause(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_UnpauseActiveSlot(ref->idx);
    return 0;
}

static const luaL_Reg m_StreamMethods[] = {
    { "stop", M_L_SoundStreamStop },
    { "pause", M_L_SoundStreamPause },
    { "unpause", M_L_SoundStreamUnpause },
    { nullptr, nullptr },
};

// trxc.sound.sample_get(id) -> sample handle or nil
static int M_L_SoundSampleGet(lua_State *const L)
{
    const int32_t id = (int32_t)luaL_checkinteger(L, 1);
    if (id < 0 || !Sound_IsAvailable_Direct((SAMPLE_ID)id)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(L, &TYPE_SOUND_SAMPLE_VIEW, M_ResolveSample, id, 0);
    return 1;
}

// trxc.sound.sample_limit()
static int M_L_SoundSampleLimit(lua_State *const L)
{
    lua_pushinteger(L, Sound_GetMaxDirectSampleID() + 1);
    return 1;
}

// trxc.sound.sample_available_count()
static int M_L_SoundSampleAvailableCount(lua_State *const L)
{
    int32_t count = 0;
    const int32_t limit = Sound_GetMaxDirectSampleID() + 1;
    for (int32_t id = 0; id < limit; id++) {
        if (Sound_IsAvailable_Direct((SAMPLE_ID)id)) {
            count++;
        }
    }
    lua_pushinteger(L, count);
    return 1;
}

// trxc.sound.stream_count()
static int M_L_SoundStreamCount(lua_State *const L)
{
    lua_pushinteger(L, Sound_GetActiveSlotCount());
    return 1;
}

// trxc.sound.stream_get(slot)
static int M_L_SoundStreamGet(lua_State *const L)
{
    const int32_t slot = (int32_t)luaL_checkinteger(L, 1);
    if (slot < 0 || slot >= Sound_GetActiveSlotCount()) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(L, &TYPE_SOUND_STREAM_VIEW, M_ResolveStream, slot, 0);
    return 1;
}

// trxc.sound.play(id[, opts])
static int M_L_SoundPlay(lua_State *const L)
{
    const SAMPLE_ID id = (SAMPLE_ID)luaL_checkinteger(L, 1);
    if (!Sound_IsAvailable_Direct(id)) {
        return luaL_error(L, "invalid sound sample: %d", (int)id);
    }
    XYZ_32 pos;
    const XYZ_32 *pos_ptr = nullptr;
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        if (lua_getfield(L, 2, "pos") == LUA_TTABLE) {
            // The blame is the options table, which is what the script wrote.
            pos = LUA_CheckXYZAt(L, -1, 2);
            pos_ptr = &pos;
        }
        lua_pop(L, 1);
    }
    Sound_Effect_Direct(id, pos_ptr, SPM_ALWAYS | SPM_STATIC_POS);
    return 0;
}

// trxc.sound.stop(id)
static int M_L_SoundStop(lua_State *const L)
{
    const SAMPLE_ID id = (SAMPLE_ID)luaL_checkinteger(L, 1);
    Sound_StopEffect_Direct(id);
    return 0;
}

// trxc.sound.stop_all()
static int M_L_SoundStopAll(lua_State *const L)
{
    Sound_StopAll();
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "sample_get", M_L_SoundSampleGet },
    { "sample_limit", M_L_SoundSampleLimit },
    { "sample_available_count", M_L_SoundSampleAvailableCount },
    { "stream_count", M_L_SoundStreamCount },
    { "stream_get", M_L_SoundStreamGet },
    { "play", M_L_SoundPlay },
    { "stop", M_L_SoundStop },
    { "stop_all", M_L_SoundStopAll },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_SOUND_SAMPLE_VIEW, m_SampleMethods);
    LUA_Struct_Register(L, &TYPE_SOUND_STREAM_VIEW, m_StreamMethods);
    LUA_RegisterModule(L, "sound", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
