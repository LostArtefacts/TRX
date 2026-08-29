#include <trx/core/memory.h>
#include <trx/game/lua/common.h>
#include <trx/game/lua/field.h>
#include <trx/game/lua/registry.h>
#include <trx/game/lua/struct.h>
#include <trx/game/lua/utils.h>
#include <trx/game/music/common.h>

#include <lauxlib.h>

// A stream handle addresses a slot, and reads through this snapshot the resolve
// refreshes. Its members are plain integers so the field reflection can name
// them; the enums they stand for are declared on the Lua side.
typedef struct {
    int32_t track_id;
    int32_t mode;
    double timestamp;
} MUSIC_STREAM_VIEW;

// A track handle addresses a track by id, and exists only while the track is
// available: a handle to a track the loaded level does not carry goes stale.
typedef struct {
    int32_t id;
} MUSIC_TRACK_VIEW;

static MUSIC_STREAM_VIEW m_StreamViews[1 + MUSIC_MAX_OVERLAY_TRACKS];

static const FIELD_DESC m_StreamFields[] = {
    FIELD_RO(MUSIC_STREAM_VIEW, track_id),
    FIELD_RO(MUSIC_STREAM_VIEW, mode),
    FIELD_RO(MUSIC_STREAM_VIEW, timestamp),
};
static MUSIC_TRACK_VIEW m_TrackView;

static const FIELD_DESC m_TrackFields[] = {
    FIELD_RO(MUSIC_TRACK_VIEW, id),
};

TYPE_DEFINE(MUSIC_STREAM_VIEW, m_StreamFields)

static void *M_ResolveStream(const LUA_STRUCT_REF *const ref)
{
    const int32_t slot = ref->handle.id;
    if (slot < 0 || slot >= Music_GetStreamSlotCount()) {
        return nullptr;
    }
    MUSIC_STREAM_STATE state;
    if (!Music_GetStreamSlotState(slot, &state)) {
        return nullptr;
    }
    m_StreamViews[slot] = (MUSIC_STREAM_VIEW) {
        .track_id = state.track_id,
        .mode = state.mode,
        .timestamp = state.timestamp,
    };
    return &m_StreamViews[slot];
}

// Hands back the stream a play landed in, by the slot the engine reported, or
// nil when the track is not playing - a delayed or deferred track is marked but
// not yet playing.
static void M_PushPlayedStream(lua_State *const L, const int32_t slot)
{
    if (slot < 0) {
        lua_pushnil(L);
    } else {
        LUA_Struct_Push(
            L, &TYPE_MUSIC_STREAM_VIEW, M_ResolveStream,
            (TRX_HANDLE) { .id = slot });
    }
}

// stream:stop()
static int M_L_MusicStreamStop(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_MUSIC_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Music_StopStream(ref->handle.id);
    return 0;
}

// stream:pause()
static int M_L_MusicStreamPause(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_MUSIC_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Music_PauseStream(ref->handle.id);
    return 0;
}

// stream:unpause()
static int M_L_MusicStreamUnpause(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_MUSIC_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    Music_UnpauseStream(ref->handle.id);
    return 0;
}

// stream:seek(timestamp)
static int M_L_MusicStreamSeek(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_MUSIC_STREAM_VIEW);
    LUA_Struct_Deref(L, ref);
    const double timestamp = luaL_checknumber(L, 2);
    lua_pushboolean(L, IGNORE(Music_SeekStream(ref->handle.id, timestamp)));
    return 1;
}

static const luaL_Reg m_StreamMethods[] = {
    { "stop", M_L_MusicStreamStop },
    { "pause", M_L_MusicStreamPause },
    { "unpause", M_L_MusicStreamUnpause },
    { "seek", M_L_MusicStreamSeek },
    { nullptr, nullptr },
};

// trxc.music.stream_count()
static int M_L_MusicStreamCount(lua_State *const L)
{
    lua_pushinteger(L, Music_GetStreamSlotCount());
    return 1;
}

// trxc.music.stream_get(slot)
static int M_L_MusicStreamGet(lua_State *const L)
{
    const int32_t slot = (int32_t)luaL_checkinteger(L, 1);
    if (slot < 0 || slot >= Music_GetStreamSlotCount()) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_MUSIC_STREAM_VIEW, M_ResolveStream,
        (TRX_HANDLE) { .id = slot });
    return 1;
}

// trxc.music.get_track()
static int M_L_MusicGetTrack(lua_State *const L)
{
    const MUSIC_SLOT track = Music_GetCurrentPlayingTrack();
    if (track < 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, (lua_Integer)track);
    }
    return 1;
}

// trxc.music.get_looped_track()
static int M_L_MusicGetLoopedTrack(lua_State *const L)
{
    const MUSIC_SLOT track = Music_GetCurrentLoopedTrack();
    if (track < 0) {
        lua_pushnil(L);
    } else {
        lua_pushinteger(L, (lua_Integer)track);
    }
    return 1;
}

TYPE_DEFINE(MUSIC_TRACK_VIEW, m_TrackFields)

static void *M_ResolveTrack(const LUA_STRUCT_REF *const ref)
{
    const int32_t id = ref->handle.id;
    if (id < 0 || !Music_IsTrackAvailableBySlot((MUSIC_SLOT)id)) {
        return nullptr;
    }
    m_TrackView.id = id;
    return &m_TrackView;
}

// track:play([opts]) -> stream or nil
static int M_L_MusicTrackPlay(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_MUSIC_TRACK_VIEW);
    LUA_Struct_Deref(L, ref);
    MUSIC_PLAY_MODE mode = MPM_ONCE;
    if (!lua_isnoneornil(L, 2)) {
        luaL_checktype(L, 2, LUA_TTABLE);
        lua_getfield(L, 2, "mode");
        if (!lua_isnil(L, -1)) {
            mode = (MUSIC_PLAY_MODE)luaL_checkinteger(L, -1);
        }
        lua_pop(L, 1);
    }
    M_PushPlayedStream(L, Music_PlayBySlot((MUSIC_SLOT)ref->handle.id, mode));
    return 1;
}

// track:path()
static int M_L_MusicTrackPath(lua_State *const L)
{
    LUA_STRUCT_REF *const ref =
        LUA_Struct_CheckRef(L, 1, &TYPE_MUSIC_TRACK_VIEW);
    LUA_Struct_Deref(L, ref);
    char *path = Music_GetTrackPath((MUSIC_SLOT)ref->handle.id);
    if (path == nullptr) {
        lua_pushnil(L);
    } else {
        lua_pushstring(L, path);
        Memory_FreePointer(&path);
    }
    return 1;
}

static const luaL_Reg m_TrackMethods[] = {
    { "play", M_L_MusicTrackPlay },
    { "path", M_L_MusicTrackPath },
    { nullptr, nullptr },
};

// trxc.music.track_get(id) -> track handle or nil
static int M_L_MusicTrackGet(lua_State *const L)
{
    const int32_t id = (int32_t)luaL_checkinteger(L, 1);
    if (id < 0 || !Music_IsTrackAvailableBySlot((MUSIC_SLOT)id)) {
        lua_pushnil(L);
        return 1;
    }
    LUA_Struct_Push(
        L, &TYPE_MUSIC_TRACK_VIEW, M_ResolveTrack, (TRX_HANDLE) { .id = id });
    return 1;
}

// trxc.music.track_limit()
static int M_L_MusicTrackLimit(lua_State *const L)
{
    lua_pushinteger(L, Music_GetTrackLimit());
    return 1;
}

// trxc.music.track_available_count()
static int M_L_MusicTrackAvailableCount(lua_State *const L)
{
    int32_t count = 0;
    const int32_t limit = Music_GetTrackLimit();
    for (int32_t id = 0; id < limit; id++) {
        if (Music_IsTrackAvailableBySlot((MUSIC_SLOT)id)) {
            count++;
        }
    }
    lua_pushinteger(L, count);
    return 1;
}

// trxc.music.pause()
static int M_L_MusicPause(lua_State *const L)
{
    Music_Pause();
    return 0;
}

// trxc.music.unpause()
static int M_L_MusicUnpause(lua_State *const L)
{
    Music_Unpause();
    return 0;
}

// trxc.music.stop()
static int M_L_MusicStop(lua_State *const L)
{
    Music_Stop();
    return 0;
}

static const luaL_Reg m_Module[] = {
    { "get_track", M_L_MusicGetTrack },
    { "get_looped_track", M_L_MusicGetLoopedTrack },
    { "track_get", M_L_MusicTrackGet },
    { "track_limit", M_L_MusicTrackLimit },
    { "track_available_count", M_L_MusicTrackAvailableCount },
    { "pause", M_L_MusicPause },
    { "unpause", M_L_MusicUnpause },
    { "stop", M_L_MusicStop },
    { "stream_count", M_L_MusicStreamCount },
    { "stream_get", M_L_MusicStreamGet },
    { nullptr, nullptr },
};

static void M_Create(lua_State *const L)
{
    LUA_Struct_Register(L, &TYPE_MUSIC_STREAM_VIEW, m_StreamMethods);
    LUA_Struct_Register(L, &TYPE_MUSIC_TRACK_VIEW, m_TrackMethods);
    LUA_RegisterModule(L, "music", m_Module);
}

REGISTER_LUA_CAPI(.create = M_Create)
