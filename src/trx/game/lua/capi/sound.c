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

// A stream handle addresses an active-sound slot; when the slot falls silent,
// the handle goes stale.
typedef struct {
    int32_t sample_id;
} SOUND_STREAM_VIEW;

static SOUND_SAMPLE_VIEW m_SampleView;

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
    const int32_t id = ref->handle.id;
    if (id < 0 || !Sound_IsAvailableBySlot((SAMPLE_SLOT)id)) {
        return nullptr;
    }
    const SAMPLE_INFO *const sample = Sound_GetSample((SAMPLE_SLOT)id);
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
    SAMPLE_SLOT sample_id;
    if (!Sound_ResolveActiveSlot(ref->handle, &sample_id)) {
        return nullptr;
    }
    m_StreamView.sample_id = sample_id;
    return &m_StreamView;
}

// Hands back the voice a play landed in, by the slot the engine reported, or
// nil when it did not play.
static void M_PushPlayedStream(lua_State *const L, const int32_t slot)
{
    if (slot < 0) {
        lua_pushnil(L);
    } else {
        LUA_Struct_Push(
            L, &TYPE_SOUND_STREAM_VIEW, M_ResolveStream,
            Sound_GetActiveSlotHandle(slot));
    }
}

// Reads the optional position table shared by the play entrypoints.
static const XYZ_32 *M_ReadPlayPos(lua_State *const L, XYZ_32 *const pos)
{
    if (lua_gettop(L) >= 2 && lua_istable(L, 2)) {
        if (lua_getfield(L, 2, "pos") == LUA_TTABLE) {
            *pos = LUA_CheckXYZAt(L, -1, 2);
            lua_pop(L, 1);
            return pos;
        }
        lua_pop(L, 1);
    }
    return nullptr;
}

// sample:play([opts]) -> stream or nil
static int M_L_SoundSamplePlay(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_SAMPLE_VIEW);
    LUA_Struct_Deref(L, ref);
    XYZ_32 pos;
    const XYZ_32 *const pos_ptr = M_ReadPlayPos(L, &pos);
    M_PushPlayedStream(
        L,
        Sound_EffectBySlot(
            (SAMPLE_SLOT)ref->handle.id, pos_ptr, SPM_ALWAYS | SPM_STATIC_POS));
    return 1;
}

// sample:stop()
static int M_L_SoundSampleStop(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_SAMPLE_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_StopEffectBySlot((SAMPLE_SLOT)ref->handle.id);
    return 0;
}

static const luaL_Reg m_SampleMethods[] = {
    { "play", M_L_SoundSamplePlay },
    { "stop", M_L_SoundSampleStop },
    { nullptr, nullptr },
};

// stream:stop()
static int M_L_SoundStreamStop(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_StopActiveSlot(ref->handle.id);
    return 0;
}

// stream:pause()
static int M_L_SoundStreamPause(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_PauseActiveSlot(ref->handle.id);
    return 0;
}

// stream:unpause()
static int M_L_SoundStreamUnpause(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_SOUND_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Sound_UnpauseActiveSlot(ref->handle.id);
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
    if (id < 0 || !Sound_IsAvailableBySlot((SAMPLE_SLOT)id)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_SOUND_SAMPLE_VIEW, M_ResolveSample, (TRX_HANDLE) { .id = id });
    return 1;
}

// trxc.sound.sample_limit()
static int M_L_SoundSampleLimit(lua_State *const L)
{
    lua_pushinteger(L, Sound_GetMaxSlot() + 1);
    return 1;
}

// trxc.sound.sample_available_count()
static int M_L_SoundSampleAvailableCount(lua_State *const L)
{
    int32_t count = 0;
    const int32_t limit = Sound_GetMaxSlot() + 1;
    for (int32_t id = 0; id < limit; id++) {
        if (Sound_IsAvailableBySlot((SAMPLE_SLOT)id)) {
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
    LUA_Struct_Push(
        L, &TYPE_SOUND_STREAM_VIEW, M_ResolveStream,
        Sound_GetActiveSlotHandle(slot));
    return 1;
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
