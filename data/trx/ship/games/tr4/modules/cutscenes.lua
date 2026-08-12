-- Scripting helpers for TR4's in-game cutscenes.
--
-- A cutscene is animation tracks rather than items, so a script has only the
-- frame to act on one by. register() takes what a level has to say about a
-- single scene and hangs it off the cutscene events:
--
--   local cutscenes = require("tr4.cutscenes")
--
--   cutscenes.register(7, {
--     on_start = function() end,
--     frames = { [675] = function() end },
--     on_frame = function(frame) end,
--     on_end = function() end,
--     chain = 9,
--     chat = {
--       { lara = true, ranges = { { 675, 877 } } },
--       {
--         actor = 1,
--         node = 21,
--         ranges = { { 396, 654 } },
--         speech_heads = { trx.catalog.objects.actor_1_speech_head_1 },
--       },
--     },
--   })

local M = {}

local LARA_SPEECH_HEADS = {
  trx.catalog.objects.lara_speech_head_1,
  trx.catalog.objects.lara_speech_head_2,
  trx.catalog.objects.lara_speech_head_3,
  trx.catalog.objects.lara_speech_head_4,
}

-- A speaker's head is swapped every other frame rather than every one, which
-- is the rate the original game talks at.
local SWAP_EVERY = 2

-- Some of those swaps leave the mouth shut, so a line reads as speech rather
-- than a steady chatter.
local CLOSED_MOUTH_CHANCE = 0.25

-- Takes every item of an object out of the level: invisible, spent so no
-- trigger brings it back, and stopped. A cutscene actor has a level item
-- standing in for it, and this is how the original engine clears one out of
-- the way. A creature left running asks for its AI again on its next frame
-- and is made visible along with it.
function M.hide_items(object)
  for _, item in ipairs(trx.items.query:of_object(object):matches()) do
    item.is_visible = false
    item.is_one_shot = true
    item:deactivate()
  end
end

-- Plays a cutscene as the level opens. Some scenes are named by no floor
-- trigger at all: the original game starts them from the level's own entry in
-- the game flow, and a level script is where TRX says so. One that has already
-- run is left alone, so coming back to the level does not play it again.
function M.play_on_start(num)
  trx.events.on_game_start(function()
    if trx.cutscenes.count > 0 and not trx.cutscenes.is_played(num) then
      trx.cutscenes.play(num, false)
    end
  end)
end

-- The nodes Von Croy wears from the swap object rather than his own.
local VON_CROY_SWAP_NODES = { 7, 18 }

-- Dresses a cutscene actor as Von Croy. A scene draws its cast from the plain
-- meshes of the objects it names, and his are the ones the level swapped out.
function M.dress_von_croy(actor)
  for _, node in ipairs(VON_CROY_SWAP_NODES) do
    trx.cutscenes.set_node_mesh(
      actor,
      node,
      trx.catalog.objects.mesh_swap_1,
      node
    )
  end
end

local function is_speaking(ranges, frame)
  for _, range in ipairs(ranges) do
    if frame > range[1] and frame < range[2] then
      return true
    end
  end
  return false
end

-- A level that never loaded a speech head cannot be given one, so the ones it
-- carries are what a speaker draws from, and a speaker left with none says its
-- lines with the head it already has.
local function loaded_heads(heads)
  local kept = {}
  for _, head in ipairs(heads or {}) do
    if trx.objects[head].loaded then
      kept[#kept + 1] = head
    end
  end
  return kept
end

local function prepare_chat(chat)
  local speakers = {}
  for _, speaker in ipairs(chat) do
    local heads =
      loaded_heads(speaker.lara and LARA_SPEECH_HEADS or speaker.speech_heads)
    if #heads > 0 then
      speakers[#speakers + 1] = {
        lara = speaker.lara,
        actor = speaker.actor,
        node = speaker.node,
        ranges = speaker.ranges,
        heads = heads,
      }
    end
  end
  return speakers
end

-- A speech head object is a whole body with one node's mesh redrawn, so the
-- mesh to take off it is the one sitting at the same node as the one being
-- replaced, not its first.
local LARA_HEAD_NODE = 14

local function set_head(speaker)
  if speaker.lara then
    trx.lara.set_mesh(
      trx.lara.Mesh.HEAD,
      trx.random.choice(speaker.heads),
      LARA_HEAD_NODE
    )
  else
    trx.cutscenes.set_node_mesh(
      speaker.actor,
      speaker.node,
      trx.random.choice(speaker.heads),
      speaker.node
    )
  end
end

local function clear_head(speaker)
  if speaker.lara then
    trx.lara.clear_mesh(trx.lara.Mesh.HEAD)
  else
    trx.cutscenes.clear_node_mesh(speaker.actor, speaker.node)
  end
end

local function control_chat(state, frame)
  state.tick = ((state.tick or 0) + 1) % SWAP_EVERY
  for _, speaker in ipairs(state.speakers) do
    if not is_speaking(speaker.ranges, frame) then
      clear_head(speaker)
    elseif state.tick == 0 then
      if trx.random.chance(CLOSED_MOUTH_CHANCE) then
        clear_head(speaker)
      else
        set_head(speaker)
      end
    end
  end
end

-- Describes one cutscene. Every entry is optional: a scene with nothing to say
-- about it needs no registration at all.
function M.register(num, def)
  local state = {}

  trx.events.on_cutscene_start(function(started_num)
    if started_num ~= num then
      return
    end
    state = { speakers = def.chat and prepare_chat(def.chat) or {} }
    if def.on_start then
      def.on_start()
    end
  end)

  -- The last frame stays on screen while the scene fades out, so a frame is
  -- acted on the once rather than for as long as it is shown.
  trx.events.after_control(function()
    local frame = trx.cutscenes.frame_num
    if trx.cutscenes.current ~= num or frame == state.frame then
      return
    end
    state.frame = frame

    local handler = def.frames and def.frames[frame]
    if handler then
      handler(frame)
    end
    if def.on_frame then
      def.on_frame(frame)
    end
    if state.speakers then
      control_chat(state, frame)
    end
  end)

  trx.events.on_cutscene_end(function(ended_num)
    if ended_num ~= num then
      return
    end
    for _, speaker in ipairs(state.speakers or {}) do
      if speaker.lara then
        clear_head(speaker)
      end
    end
    if def.on_end then
      def.on_end()
    end
    -- Played from the end event, where the scene it follows is torn down and
    -- the screen is still black, so the two run as one.
    if def.chain then
      trx.cutscenes.play(def.chain)
    end
  end)
end

return M
