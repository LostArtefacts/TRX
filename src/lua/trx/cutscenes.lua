require("trx.signal")

local raw = trxc.cutscenes
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field cutscenes trx.cutscenes

---Module for TR4's in-game cutscenes, the animated scenes stored in
---`cutseq.pak` <!--noref: cutseq.pak--> and started by a cutscene trigger. A
---cutscene plays once: the engine remembers which ones have run, and a script
---may consult or rewrite that memory. The cutscene levels of TR1-TR3, which
---the game flow lists and `/cut` plays, are a different thing: see
---`trx.game.cutscenes`.
---@trx.module 14
---@class (exact) trx.cutscenes: table<trx.cutscenes.Num, trx.cutscenes.Cutscene?>
---@trx.readonly actor_count, count, current, frame_num, is_active, is_playing
---@field current trx.cutscenes.Cutscene? The cutscene playing, or `nil` if none is.
---@field frame_num trx.cutscenes.FrameNum? Which frame of the running cutscene is on screen, or `nil` if none is running. A cutscene's actors are animation tracks rather than items, so nothing in it can be triggered or listened to; naming a frame is how a script acts part-way through one, as the original game does.
---@field is_playing boolean Whether a cutscene is on screen.
---@field is_active boolean Whether a cutscene has the screen, including during its fades. Use this to keep an interface off while the cutscene is active.
---@field count integer How many cutscenes this game can play. `0` where it has none, which is every game but TR4 and a TR4 install with no `cutseq.pak` <!--noref: cutseq.pak--> beside its levels.
---@field actor_count integer How many actors the running cutscene has, or `0` if none is running.
---@field fov trx.math.Angle Field of view a cutscene plays at. TR4 uses 11488, against 14560 for ordinary play.
---@field letterbox number Depth of each cinematic bar, as a fraction of the screen height. `0` removes them. A change made while a cutscene plays moves the bars to the new depth.
local M = h.module("cutscenes")

---Cutscene number, as a cutscene trigger names it.
---@trx.base 0
---@alias trx.cutscenes.Num integer

---A frame's number within the cutscene it belongs to.
---@trx.base 0
---@alias trx.cutscenes.FrameNum integer

-- Maps each handle to the cutscene number it stands for. A handle is an empty
-- table, so the number is reachable only through this map.
local nums = setmetatable({}, { __mode = "k" })
local handles = {}

local function num_of(self)
  return nums[self]
end

-- Narrows one of the module's events to a single scene. trx.events is reached
-- at call time, so its module need not load before this one.
local function narrow(self, event_name, callback)
  return trx.events[event_name](function(fired, ...)
    if fired == self then
      callback(self, ...)
    end
  end)
end

---One of the scenes a cutscene trigger can name. A number the pak holds no
---scene for is still one of these, because the engine remembers it as played
---the same way; `trx.cutscenes.Cutscene:play` is what such a number has
---nothing to do.
---@class (exact) trx.cutscenes.Cutscene
---@trx.readonly frame_num, is_playing, num
---@field num trx.cutscenes.Num Which scene this is.
---@field is_played boolean Whether a trigger naming this number has already been answered. True keeps its trigger from firing; writing false lets it run again.
---@field is_playing boolean Whether this scene is the one on screen.
---@field frame_num trx.cutscenes.FrameNum? Which frame of this scene is on screen, or `nil` unless it is the one playing.
local Cutscene = h.class("cutscenes.Cutscene", {
  fields = {
    num = { get = num_of },

    is_played = {
      get = function(self)
        return raw.is_played(num_of(self))
      end,
      set = function(self, value)
        raw.set_played(num_of(self), value and true or false)
      end,
    },

    is_playing = {
      get = function(self)
        return raw.get_current() == num_of(self)
      end,
    },

    frame_num = {
      get = function(self)
        if raw.get_current() ~= num_of(self) then
          return nil
        end
        return raw.get_frame_num()
      end,
    },
  },
})

---@class (exact) trx.cutscenes.Cutscene.play.opts
---@field fade? boolean Whether to fade the scene out before the first frame. A cutscene that opens a level passes false: the original game holds the screen black rather than showing the level for a moment first, and the scene's own fade in follows either way.
---@trx.default fade true

---Plays this scene. Does nothing if one is already playing or the game holds
---no scene for this number.
---
---```lua
---trx.cutscenes[28]:play()
---```
---@param opts? trx.cutscenes.Cutscene.play.opts How to play it.
function Cutscene:play(opts)
  local fade = true
  if opts ~= nil and opts.fade ~= nil then
    fade = opts.fade
  end
  raw.play(num_of(self), fade)
end

---Happens when this scene's first frame is about to show.
---`trx.events.on_cutscene_start`, narrowed to this cutscene.
---@param callback fun(cutscene: trx.cutscenes.Cutscene) What to run when it happens.
---@trx.arg callback.cutscene This cutscene.
---@return trx.events.Listener # The attached handler.
function Cutscene:on_start(callback)
  return narrow(self, "on_cutscene_start", callback)
end

---Happens on every frame of this scene, before the frame is posed.
---
---A cutscene has no items to listen to. Its actors are animation
---tracks, so the frame number is the only thing a script can act on.
---
---`trx.events.on_cutscene_frame`, narrowed to this cutscene.
---
---```lua
---trx.cutscenes[5]:on_frame(function(cutscene, frame_num)
---  if frame_num == 1350 then
---    -- something happens here
---  end
---end)
---```
---@param callback fun(cutscene: trx.cutscenes.Cutscene, frame_num: trx.cutscenes.FrameNum) What to run when it happens.
---@trx.arg callback.cutscene This cutscene.
---@trx.arg callback.frame_num The frame about to be posed.
---@return trx.events.Listener # The attached handler.
function Cutscene:on_frame(callback)
  return narrow(self, "on_cutscene_frame", callback)
end

---Happens once this scene has finished and what it interrupted is back.
---`trx.events.on_cutscene_end`, narrowed to this cutscene.
---@param callback fun(cutscene: trx.cutscenes.Cutscene) What to run when it happens.
---@trx.arg callback.cutscene This cutscene.
---@return trx.events.Listener # The attached handler.
function Cutscene:on_end(callback)
  return narrow(self, "on_cutscene_end", callback)
end

-- Returns one handle per number, so that two ways of reaching the same scene
-- give the same value and compare equal.
local function handle_of(num)
  if
    type(num) ~= "number"
    or num % 1 ~= 0
    or num < 0
    or num >= raw.MAX_TRIGGERS
  then
    return nil
  end
  local handle = handles[num]
  if handle == nil then
    handle = setmetatable({}, Cutscene)
    nums[handle] = num
    handles[num] = handle
  end
  return handle
end

---Indexing the module reaches a cutscene by the number a trigger names it
---with. `#trx.cutscenes` is how many the game can play, and `pairs()` walks
---those in order; the numbers past them are reachable as well, because the
---engine remembers any of them as played.
---
---```lua
---trx.cutscenes[30]:on_frame(function(cutscene, frame_num)
---  trx.log.info("frame " .. frame_num .. " of " .. cutscene.num)
---end)
---```
---@type table<trx.cutscenes.Num, trx.cutscenes.Cutscene?>
---@trx.key The number a cutscene trigger names.
h.container("cutscenes", {
  base = 0,
  get = handle_of,
  count = raw.get_count,
}, M)

---Plays a cutscene, fading the scene out first. Does nothing if one is already
---playing or the game has no cutscene data.
---@deprecated Call `trx.cutscenes.Cutscene:play` instead.
---@param num trx.cutscenes.Num
---@param fade? boolean Whether to fade the scene out before the first frame. Defaults to true. A cutscene that opens a level passes false: the original game holds the screen black rather than showing the level for a moment first, and the scene's own fade in follows either way.
---@type fun(num: trx.cutscenes.Num, fade?: boolean)
M.play = raw.play

---The signals a cutscene speaks through, for a script that would rather hear
---about a change than ask after one.
---@class (exact) trx.cutscenes.signals
---@trx.readonly is_active, is_playing
---@field is_playing trx.signal.Signal Says when a cutscene takes the screen, and when it gives it back.
---@field is_active trx.signal.Signal Signals when a cutscene has the screen, including during its fades.
M.signals = h.namespace("cutscenes.signals")

local cutscene_playing = nil
local cutscene_active = nil

h.properties(M.signals, "cutscenes.signals", {
  is_playing = {
    get = function()
      if cutscene_playing == nil then
        cutscene_playing = trx.signal.polled(function()
          return trx.cutscenes.is_playing
        end)
      end
      return cutscene_playing
    end,
  },
  is_active = {
    get = function()
      if cutscene_active == nil then
        cutscene_active = trx.signal.polled(function()
          return trx.cutscenes.is_active
        end)
      end
      return cutscene_active
    end,
  },
})

---Which of a cutscene's actors. Actor `0` is Lara, who is posed rather than
---drawn as an actor; the cast a scene brings with it starts at `1`.
---@trx.base 0
---@alias trx.cutscenes.ActorNum integer

---Which of an actor's meshes, the root being the first.
---@trx.base 0
---@alias trx.cutscenes.NodeNum integer

h.properties(M, "cutscenes", {
  current = {
    get = function()
      return handle_of(raw.get_current())
    end,
  },
  frame_num = { get = raw.get_frame_num },
  is_playing = { get = raw.is_playing },
  is_active = { get = raw.is_active },
  count = { get = raw.get_count },
  actor_count = { get = raw.get_actor_count },
  fov = { get = raw.get_fov, set = raw.set_fov },
  letterbox = { get = raw.get_letterbox, set = raw.set_letterbox },
})

---Whether an actor is drawn. A scene brings its whole cast on from its first
---frame, so an actor who is only due later is hidden until then, as the
---original game hides one.
---
---It lasts as long as the cutscene, and every actor starts out visible.
---
---```lua
---trx.events.on_cutscene_start(function(num)
---  if num == 9 then
---    trx.cutscenes.set_actor_visible(3, false)
---  end
---end)
---```
---@param actor trx.cutscenes.ActorNum
---@param visible boolean Whether the actor is drawn.
---@type fun(actor: trx.cutscenes.ActorNum, visible: boolean)
M.set_actor_visible = raw.set_actor_visible

---Draws another object's mesh in place of the one an actor's node carries.
---This is how a talking head goes on a body: the speech-head objects hold a
---mouth in each shape, and swapping between them while a line plays is what
---the original game animates speech with.
---
---Raises if this level does not carry the object.
---
---```lua
---trx.cutscenes.set_node_mesh(1, 21, trx.catalog.objects.actor_1_speech_head_1)
---```
---@param actor trx.cutscenes.ActorNum
---@param node trx.cutscenes.NodeNum
---@param object trx.catalog.objects The object to take a mesh from.
---@param mesh_num? integer Which of that object's meshes. Defaults to `0`.
---@type fun(actor: trx.cutscenes.ActorNum, node: trx.cutscenes.NodeNum, object: trx.catalog.objects, mesh_num?: integer)
M.set_node_mesh = raw.set_node_mesh

---Takes the override back off, leaving the mesh the actor's own object gives
---that node.
---@param actor trx.cutscenes.ActorNum
---@param node trx.cutscenes.NodeNum
---@type fun(actor: trx.cutscenes.ActorNum, node: trx.cutscenes.NodeNum)
M.clear_node_mesh = raw.clear_node_mesh

---Whether a cutscene trigger naming this number has already been answered.
---@deprecated Read `trx.cutscenes.Cutscene.is_played` instead.
---@param num trx.cutscenes.Num
---@return boolean # True once it has run, which is what keeps its trigger from firing again.
---@type fun(num: trx.cutscenes.Num): boolean
M.is_played = raw.is_played

---Marks a cutscene as played or unplayed. Marking one as played keeps its
---trigger from firing; unmarking one lets it run again.
---
---A trigger may name a number the game has no cutscene for - TR4 uses 32 to
---ask for a full-motion video - and the engine remembers those the same way,
---so `trx.events.on_cutscene_trigger` hears about each of them once. This is
---what clears that memory, and it takes any number a trigger may carry, not
---only the ones `trx.cutscenes.play` accepts.
---@deprecated Write `trx.cutscenes.Cutscene.is_played` instead.
---@param num trx.cutscenes.Num
---@param played boolean Whether it counts as played.
---@type fun(num: trx.cutscenes.Num, played: boolean)
M.set_played = raw.set_played

---Forgets every cutscene, so all of them may run again.
---@type fun()
M.forget_played = raw.forget_played

---Places Lara where the next cutscene to end leaves her. A cutscene stands
---her at its own origin while it plays and puts her back where it found her
---afterwards; this says to put her somewhere else instead, as the original
---game does for the scenes that carry her along.
---
---It holds for one cutscene, whether named before `trx.cutscenes.play` or
---while the scene runs, and is forgotten once she has been placed.
---
---```lua
---trx.events.on_cutscene_start(function(num)
---  if num == 12 then
---    trx.cutscenes.set_lara_return({ x = 38912, y = 2048, z = 51200 })
---  end
---end)
---```
---@param pos trx.math.Vec3 World position.
---@param rot? trx.math.Angle Facing angle. Defaults to `0`.
---@type fun(pos: trx.math.Vec3, rot?: trx.math.Angle)
M.set_lara_return = raw.set_lara_return

---Gives Lara's shadow another box for the running cutscene. A scene holds one
---box for the whole of it, rather than the box her pose would make, so that
---her shadow keeps a steady size while the scene moves her; this names a
---different one. A wide box is how the original game makes her shadow read
---as the jeep's when she arrives at Karnak.
---
---It holds for one cutscene, whether named before `trx.cutscenes.play` or
---while the scene runs, and the next scene starts from the ordinary box
---again.
---
---```lua
---trx.events.on_cutscene_start(function(num)
---  if num == 12 then
---    trx.cutscenes.set_lara_shadow_bounds({
---      min_x = -600, min_y = -777, min_z = -600,
---      max_x = 600, max_y = 1, max_z = 600,
---    })
---  end
---end)
---```
---@param bounds trx.math.Box The box, in Lara's own frame.
---@type fun(bounds: trx.math.Box)
M.set_lara_shadow_bounds = raw.set_lara_shadow_bounds
