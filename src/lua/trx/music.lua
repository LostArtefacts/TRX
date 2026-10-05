local raw = trxc.music
local h = require("trx.internal.helpers")

---@class trx
---@field music trx.music

---Module for playing and controlling the soundtrack.
---@trx.module 23
---@class (exact) trx.music
---@trx.readonly current_track, looped_track
---@field current_track trx.music.Track? The track playing now, or `nil` when nothing plays.
---@field looped_track trx.music.Track? The ambient track that resumes once the current one-shot finishes, or `nil` when none is set.
local M = h.module("music")

---Track number, in the numbering the loaded level carries. Not a
---`trx.catalog.music` name, which is the soundtrack's own.
---@trx.base 0
---@alias trx.music.TrackNum integer

---Which of the soundtrack's slots: 1 is the main stream, 2 onwards the overlays.
---@trx.base 1
---@alias trx.music.StreamNum integer

---How a track is played. Pass one as `trx.music.play.opts.mode`.
---@enum trx.music.PlayMode
local PlayMode = {
  ---Plays the track once. When it finishes, any active looped track resumes
  ---from its start.
  ONCE = h.IntegerConstant,
  ---Plays the track continuously. It becomes the ambient track.
  LOOP = h.IntegerConstant,
  ---Plays the track once, but does not retrigger it if it is already playing.
  NO_REPEAT = h.IntegerConstant,
  ---Marks the track for later playback rather than starting it now.
  DELAY = h.IntegerConstant,
  ---Plays the track on top of the current one.
  OVERLAY = h.IntegerConstant,
}
M.PlayMode = h.enum("music.PlayMode", "MUSIC_PLAY_MODE", PlayMode)

---@class (exact) trx.music.play.opts
---@field mode? trx.music.PlayMode Plays once by default.

---One of the soundtrack's playing streams: the main stream, or an overlay.
---Reach them through `trx.music.streams`. A handle to a slot that is not
---playing goes stale, so reading a field or calling a method on it raises;
---check `trx.music.Stream:is_valid` first.
---@class (exact) trx.music.Stream
---@trx.readonly mode, timestamp, track_num
---@field mode trx.music.PlayMode How the track is playing.
---@field timestamp trx.game.Seconds How far into the track the stream is.
---@field track_num trx.music.TrackNum The track this stream is playing.
local Stream = h.handle("music.Stream", "MUSIC_STREAM_VIEW", {
  fields = { track_num = "track_id", mode = "mode", timestamp = "timestamp" },
})

---Whether the slot is still playing. A stream that has finished, or been
---stopped, leaves its handle stale.
---@return boolean # False once the slot has gone quiet.
function Stream:is_valid() end

---Pauses this stream.
function Stream:pause() end

---Resumes this stream.
function Stream:unpause() end

---Seeks this stream to a timestamp.
---@param timestamp trx.game.Seconds Where to seek to.
---@return boolean # Whether the seek took.
function Stream:seek(timestamp) end

---Stops this stream. Stopping the main stream lets a deferred ambient loop
---resume; an overlay just ends.
function Stream:stop() end

---A track the current level carries. Reach them through `trx.music.tracks`,
---or as `trx.music.current_track`. A handle to a track the loaded level does
---not carry goes stale, so `trx.music.Track:is_valid` answers whether it is
---still there.
---@class (exact) trx.music.Track
---@trx.readonly num
---@field num trx.music.TrackNum
local Track = h.handle("music.Track", "MUSIC_TRACK_VIEW", {
  fields = { num = "id" },
})

---Whether the loaded level still carries this track.
---@return boolean # False once a level change has replaced the tracks.
function Track:is_valid() end

---Plays this track.
---@param opts? trx.music.play.opts How to play it.
---@return trx.music.Stream? # The stream it started, or `nil` if none did.
function Track:play(opts) end

---Resolves the track's file path.
---@return string? # `nil` when there is no file, e.g. a CD-audio soundtrack.
function Track:path() end

-- One lazy view apiece: indexing and iterating reach into C a handle at a time,
-- so neither builds a list up front.

---The soundtrack's streams: `[1]` is the main stream, `[2]` onwards the
---overlay slots. A slot that is not playing still answers, with a stale
---handle.
---@type table<trx.music.StreamNum, trx.music.Stream?>
M.streams = h.container("music.streams", {
  base = 1,
  get = function(n)
    return raw.stream_get(n - 1)
  end,
  count = raw.stream_count,
})

---The tracks the current level carries. A level does not carry every number,
---so indexing one it lacks is `nil` and iterating passes it by.
---@type table<trx.music.TrackNum, trx.music.Track?>
M.tracks = h.container("music.tracks", {
  base = 0,
  get = raw.track_get,
  count = raw.track_available_count,
  limit = raw.track_limit,
})

h.properties(M, "music", {
  current_track = {
    get = function()
      local id = raw.get_track()
      return id ~= nil and raw.track_get(id) or nil
    end,
  },
  looped_track = {
    get = function()
      local id = raw.get_looped_track()
      return id ~= nil and raw.track_get(id) or nil
    end,
  },
})

---Plays a track by catalog id, mapping it to the level's own track. A game
---that does not carry the track plays nothing.
---
---```lua
---trx.music.play(trx.catalog.music.SECRET)
---trx.music.play(trx.catalog.music.SECRET, { mode = trx.music.PlayMode.LOOP })
---```
---@param id trx.catalog.music Track to play. To reach a track by the level's own slot, play it through a handle: `trx.music.tracks[slot]:play()`.
---@param opts? trx.music.play.opts How to play it.
---@return trx.music.Stream? # The stream it started, or `nil` if none did.
function M.play(id, opts)
  opts = opts or {}
  local slot = trx.catalog.to_slot(trx.catalog.Context.MUSIC, id)
  local track = slot ~= nil and M.tracks[slot] or nil
  return track ~= nil and track:play({ mode = opts.mode or M.PlayMode.ONCE })
    or nil
end

---Pauses the music.
---@type fun()
M.pause = raw.pause

---Resumes paused music.
---@type fun()
M.unpause = raw.unpause

---Stops all music.
---@type fun()
M.stop = raw.stop
