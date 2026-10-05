local raw = trxc.sound
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field sound trx.sound

---Module for playing sound effects.
---@trx.module 22
---@class (exact) trx.sound
local M = h.module("sound")

---Sample number, in the numbering the loaded level carries. Not a
---`trx.catalog.samples` name, which is the sound bank's own.
---@trx.base 0
---@alias trx.sound.SampleNum integer

---@class (exact) trx.sound.play.opts
---@field pos? trx.math.Vec3 A world position to play from, which applies pan and volume. Omit to play at full volume.

---Which of the voices playing now, counted in the order the engine holds them.
---@trx.base 1
---@alias trx.sound.StreamNum integer

---A sound sample the current level carries. Reach them through
---`trx.sound.samples`. A handle to a sample the loaded level does not carry
---goes stale, so `trx.sound.Sample:is_valid` answers whether it is still
---there.
---@class (exact) trx.sound.Sample
---@trx.readonly num, pitch, randomness, range, volume
---@field num trx.sound.SampleNum
---@field volume integer The sample's base volume.
---@field range integer How far the sample carries.
---@field randomness integer How much the sample's playback is randomized.
---@field pitch integer The sample's base pitch.
local Sample = h.handle("sound.Sample", "SOUND_SAMPLE_VIEW", {
  fields = {
    num = "id",
    volume = "volume",
    range = "range",
    randomness = "randomness",
    pitch = "pitch",
  },
})

---Whether the loaded level still carries this sample.
---@return boolean # False once a level change has replaced the samples.
function Sample:is_valid()
  return h.native()
end

---Plays this sample.
---@param opts? trx.sound.play.opts How to play it.
---@return trx.sound.Stream? # The voice it started, or `nil` if none did.
function Sample:play(opts)
  return h.native()
end

---Stops every voice playing this sample.
function Sample:stop()
  return h.native()
end

---One of the sound effects playing now. Reach them through
---`trx.sound.streams`. A handle to a voice that has fallen silent goes stale,
---so check `trx.sound.Stream:is_valid` first.
---@class (exact) trx.sound.Stream
---@trx.readonly sample_num
---@field sample_num trx.sound.SampleNum The sample this voice is playing.
local Stream = h.handle("sound.Stream", "SOUND_STREAM_VIEW", {
  fields = { sample_num = "sample_id" },
})

---Whether this voice is still playing.
---@return boolean # False once the voice has fallen silent.
function Stream:is_valid()
  return h.native()
end

---Pauses this voice.
function Stream:pause()
  return h.native()
end

---Resumes this voice.
function Stream:unpause()
  return h.native()
end

---Stops this voice.
function Stream:stop()
  return h.native()
end

---The samples the current level carries. A level does not carry every number,
---so indexing one it lacks is `nil` and iterating passes it by.
---@type table<trx.sound.SampleNum, trx.sound.Sample?>
M.samples = h.container("sound.samples", {
  base = 0,
  get = raw.sample_get,
  count = raw.sample_available_count,
  limit = raw.sample_limit,
})

---The sound effects playing now. A slot that is silent still answers, with a
---stale handle.
---@type table<trx.sound.StreamNum, trx.sound.Stream?>
M.streams = h.container("sound.streams", {
  base = 1,
  get = function(n)
    return raw.stream_get(n - 1)
  end,
  count = raw.stream_count,
})

---Plays a sound effect by catalog id, mapping it to the level's own sample. A
---game that does not carry the sample plays nothing.
---
---```lua
---trx.sound.play(trx.catalog.samples.LARA_NO)
---trx.sound.play(trx.catalog.samples.LARA_NO, { pos = { x = 100, y = 200, z = 50 } })
---```
---@param id trx.catalog.samples Sample to play. To reach a sample by the level's own slot, play it through a handle: `trx.sound.samples[slot]:play()`.
---@param opts? trx.sound.play.opts How to play it.
---@return trx.sound.Stream? # The voice it started, or `nil` if none did.
function M.play(id, opts)
  local slot = trx.catalog.to_slot(trx.catalog.Context.SAMPLES, id)
  local sample = slot ~= nil and M.samples[slot] or nil
  return sample ~= nil and sample:play(opts) or nil
end

---Stops a sound effect by catalog id.
---@param id trx.catalog.samples Sample to stop. To reach a sample by the level's own slot, stop it through a handle: `trx.sound.samples[slot]:stop()`.
function M.stop(id)
  local slot = trx.catalog.to_slot(trx.catalog.Context.SAMPLES, id)
  local sample = slot ~= nil and M.samples[slot] or nil
  if sample ~= nil then
    sample:stop()
  end
end

---Stops every sound effect currently playing.
---@type fun()
M.stop_all = raw.stop_all
