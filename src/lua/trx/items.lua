local raw = trxc.items
local h = require("trx.internal.helpers")

require("trx.math")

local Box = h.class_of("math.Box")
require("trx.query")

---@class trx
---@field items trx.items

---Module for controlling all moveables.
---@trx.module 2
---@class (exact) trx.items: table<trx.items.Num|string, trx.items.Item?>
---@trx.readonly query
---@field query trx.items.ItemQuery The identity query over every item in the level. Narrow it and read it.
local M = h.module("items")

---The animation's number within the object an item is of.
---@trx.base 0
---@alias trx.items.AnimNum integer

---The frame's number within the animation it belongs to.
---@trx.base 0
---@alias trx.items.FrameNum integer

---An animation state, as the object's own animations number them. What a state
---means is the object's business: the numbers of a wolf are not the numbers of
---a door.
---@trx.base 0
---@alias trx.items.AnimState integer

---Item number, matching the numbers level editors show.
---@trx.base 0
---@alias trx.items.Num integer

---<!--noref: earthquake_mode--> The values the `earthquake_mode` item property
---can take. It selects the behavior of the camera shake and sound effects of
---active earthquakes.
---@enum trx.items.EarthquakeMode
local EarthquakeMode = {
  ---Per TR1 - the camera shakes at random, and sound effects earthquake_1 and
  ---earthquake_2 are played at random intervals.
  RANDOM_1 = h.IntegerConstant,
  ---Per TR2 - similar to TR1, with less randomness and only the earthquake_1
  ---sound effect is played.
  RANDOM_2 = h.IntegerConstant,
  ---Per TR3 - the camera shakes on a ramped scale and the earthquake_loop
  ---sound effect plays throughout.
  RAMPED = h.IntegerConstant,
  ---Per TR4 - the camera shakes and the earthquake_loop sound effect plays on
  ---each frame.
  BASIC = h.IntegerConstant,
}
M.EarthquakeMode =
  h.enum("items.EarthquakeMode", "EARTHQUAKE_MODE", EarthquakeMode)

---<!--noref: pickup_mode--> The values the `pickup_mode` item property can
---take. It selects the animation Lara plays when collecting the item.
---@enum trx.items.PickupMode
local PickupMode = {
  ---Picked up off the floor.
  NORMAL = h.IntegerConstant,
  ---Picked up from a low pedestal.
  PLINTH_LOW = h.IntegerConstant,
  ---Picked up from a high pedestal.
  PLINTH_HIGH = h.IntegerConstant,
  ---Hidden behind an object Lara can reach into.
  HIDDEN = h.IntegerConstant,
  ---Pried off the wall using a crowbar.
  CROWBAR = h.IntegerConstant,
  ---Hidden inside a sarcophagus.
  SARCOPHAGUS = h.IntegerConstant,
  ---Similar to PLINTH_HIGH; invokes Lara's extra animation as in Tomb of
  ---Qualopec.
  PLINTH_SCION = h.IntegerConstant,
}
M.PickupMode = h.enum("items.PickupMode", "PICKUP_MODE", PickupMode)

---<!--noref: scaled_spikes_mode--> The values the `scaled_spikes_mode` item
---property can take. It determines how spikes behave when triggered.
---@enum trx.items.ScaledSpikesMode
local ScaledSpikesMode = {
  ---Spikes will extend, wait a brief period, retract, and then the loop will
  ---repeat.
  LOOPING = h.IntegerConstant,
  ---Spikes will extend and remain as-is indefinitely.
  EXTENDED = h.IntegerConstant,
  ---Spikes will extend, wait a brief period, retract, and then stop.
  ONE_SHOT = h.IntegerConstant,
}
M.ScaledSpikesMode =
  h.enum("items.ScaledSpikesMode", "SCALED_SPIKES_MODE", ScaledSpikesMode)

---<!--noref: switch_mode--> The values the `switch_mode` item property can
---take. It selects the animation Lara plays when interacting with the item.
---@enum trx.items.SwitchMode
local SwitchMode = {
  ---A regular/classic wall lever.
  NORMAL = h.IntegerConstant,
  ---Lara reaches in to activate.
  HIDDEN_REACH = h.IntegerConstant,
  ---Lara reaches in to collect a pickup.
  HIDDEN_PICKUP = h.IntegerConstant,
  ---A single-use button that requires a shove to activate.
  SHOVE = h.IntegerConstant,
}
M.SwitchMode = h.enum("items.SwitchMode", "SWITCH_MODE", SwitchMode)

---The kind of trigger `trx.items.Item:trigger` fires, matching the trigger
---types a level editor offers. Most are forward triggers that differ only in
---what trips them in a level; from a script they behave alike, and `TRIGGER`
---is the one to reach for.
---@enum trx.items.TriggerType
local TriggerType = {
  ---A plain trigger: sets the code bits and, once they are all set, starts the
  ---item.
  TRIGGER = h.IntegerConstant,
  ---Takes the trigger back, clearing the code bits. The item is left running
  ---so it can stand itself down, which is how a door animates shut.
  ANTITRIGGER = h.IntegerConstant,
  ---Toggles the code bits, so firing it a second time takes the trigger back.
  SWITCH = h.IntegerConstant,
  ---A forward trigger a heavy object trips. A falling block reads this to know
  ---it was set off by weight.
  HEAVY = h.IntegerConstant,
  ---A switch a heavy object trips.
  HEAVY_SWITCH = h.IntegerConstant,
}
M.TriggerType = h.enum("items.TriggerType", "ITEM_TRIGGER_KIND", TriggerType)

---<!--noref: loop_sound--> The values the `loop_sound` item property can take.
---It selects the sound a waterfall loops while it runs.
---@enum trx.items.WaterfallSound
local WaterfallSound = {
  ---The waterfall runs silently.
  NONE = h.IntegerConstant,
  ---A pouring sand loop.
  SAND = h.IntegerConstant,
  ---A running water loop.
  WATER = h.IntegerConstant,
}
M.WaterfallSound =
  h.enum("items.WaterfallSound", "WATERFALL_SOUND", WaterfallSound)

-- Item handles are bare userdata. Their metatable is populated by the h.handle
-- declaration below, and by nothing else: a member of the C ITEM struct that is
-- not named here is not reachable from a script at all.

local function make_properties(item)
  return setmetatable({}, {
    __index = function(_, key)
      if type(key) ~= "string" then
        return nil
      end
      return item:get_property(key)
    end,
    __newindex = function(_, key, value)
      item:set_property(key, value)
    end,
    __pairs = function()
      local names = item:get_property_names()
      local i = 0
      return function()
        i = i + 1
        local name = names[i]
        if name == nil then
          return nil
        end
        return name, item:get_property(name)
      end
    end,
  })
end

-- on_trigger narrows the global event to this one item. trx.events is reached
-- at call time, so its module need not load before this one.
local function item_hook(event_name)
  return function(item, callback)
    return trx.events[event_name](function(fired, ...)
      if fired == item then
        callback(fired, ...)
      end
    end)
  end
end

-- What a trigger carries, which both the hook and the per-item hook hand over.
-- A plain table the engine builds, so its keys are entries it holds rather than
-- accessors.

---What a trigger carried when it fired.
---@trx.record
---@class trx.items.Trigger
---@field type trx.items.TriggerType The kind of trigger it was.
---@field mask integer The code bits it set, `1` to `31`.
---@field timer trx.game.Seconds How long it keeps the item going.
---@field one_shot boolean Whether it fires only the once.

---@class (exact) trx.items.Item.trigger.opts
---@field type? trx.items.TriggerType A plain `TRIGGER` by default.
---@field mask? integer Which of the five code bits to set, `1` to `31`, all of them by default. Pass fewer to act as one of several triggers a puzzle is waiting on.
---@field timer? trx.game.Seconds How long it should keep the item going. `0` means until something takes the trigger back. A timer of exactly `1` is a single frame, not a second, matching the level format.
---@field one_shot? boolean Never let it fire again.
---@trx.default timer 0

---@class (exact) trx.items.Item.die.opts.gibs
---@field flame? boolean Trail fire, and burn where the part lands.
---@field smoke? boolean Trail smoke, and smoke where the part lands.
---@field blast? boolean Burst where the part lands, or where it reaches Lara.
---@field blood? boolean Trail blood.
---@trx.default flame false
---@trx.default smoke false
---@trx.default blast false
---@trx.default blood false

---@class (exact) trx.items.Item.die.opts
---@field explode? boolean Whether to burst the meshes as it dies.
---@field gibs? trx.items.Item.die.opts.gibs Sets the effects for the flying body parts. TR1 and TR2 support `blast`. TR3 supports `flame` and `smoke`. <!--noref: blast--><!--noref: flame--><!--noref: smoke-->
---@field flame_variant? integer Flame color for body parts that burn, as `trx.fx.sparks.fire_flame` defines it: `0` orange, `2` pale, and `254` green.
---@field sender? trx.items.Item Item to credit the death to. Pass `trx.lara.item` to include the kill in Lara's level statistics. Without it, the kill counts for nobody.
---@trx.default explode false
---@trx.default flame_variant 0

---@class (exact) trx.items.Item.shatter.opts
---@field gibs? table Sets the effects for the flying body parts, as `trx.items.Item.die.opts.gibs` takes it.
---@field mesh_bits? integer Which meshes to burst, a bit to a mesh. `-1` bursts them all. Use the complement of the meshes to spare for a narrower set.
---@field speed? integer The fastest a part is thrown out, and `fall_speed` the fastest it drops. `0` takes the usual speed. <!--noref: fall_speed-->
---@field fall_speed? integer The fastest a part drops.
---@field damage? integer Damage a flying body part deals to Lara.
---@field flame_variant? integer Flame color for body parts that burn, as `trx.fx.sparks.fire_flame` defines it.
---@trx.default mesh_bits -1
---@trx.default speed 0
---@trx.default fall_speed 0
---@trx.default damage 0
---@trx.default flame_variant 0

---An item, also known as a moveable.
---@class (exact) trx.items.Item
---@trx.readonly is_alive, is_ally, is_hostile, is_in_play, is_killed,
---  is_present, is_simulated, is_targetable, is_triggered, max_hit_points,
---  num, object_id, room_num, touch_bits, was_hit
---@field pos trx.math.Vec3 World position. Updating this also updates `trx.items.Item.room` and `trx.items.Item.room_num`.
---@field rot trx.math.Rot Orientation.
---@field anim_num trx.items.AnimNum
---@field frame_num trx.items.FrameNum Negative values count back from the end.
---@field num trx.items.Num An item handed over by a query can say where it lives.
---@field room_num trx.rooms.Num The room containing this item. Set `trx.items.Item.pos` to move the item between rooms.
---@field hit_points integer Current hit points. Raising this above the maximum also raises the `max_hit_points` entry of `trx.items.Item.properties`. <!--noref: max_hit_points-->
---@field max_hit_points integer Maximum hit points. Set the `max_hit_points` entry of `trx.items.Item.properties` to change it. <!--noref: max_hit_points-->
---@field name string Unique item name, or `nil`. Assigning a name already in use raises an error.
---@field object_id trx.catalog.objects The item's object type.
---@field is_visible boolean Whether the item is drawn. It can be present in the world but not visible, like an ambush enemy waiting to appear.
---@field is_finished boolean Whether the item has finished its run - a creature that died, or a one-shot trigger that fired. It stays in the level but no longer acts.
---@field is_present boolean Whether the item is in the world at all: linked in its room, so drawn and collidable in principle. Managed by the engine.
---@field timer trx.game.Frames How long the item's trigger keeps it going. `0` runs it until something takes the trigger back; `-1` means it has run out; anything else counts down. `trx.items.Item:trigger` takes its own timer as a `trx.game.Seconds`.
---@field is_triggered boolean Whether the item's trigger currently says go. This is what a door, a switch or an alarm reads to decide whether to act; a creature ignores it and goes by whether it is running.
---
---  It is a verdict on `trx.items.Item.trigger_mask`, `trx.items.Item.timer`
---  and `trx.items.Item.is_reversed` together, not a field of its own.
---@field trigger_mask integer The five code bits, counted the way a level editor counts them: `1` to `31`. The trigger only says go once every bit is set, which is how a level makes several triggers agree before anything happens. A lone trigger carries all of them.
---@field is_reversed boolean Whether the item's trigger is inverted, so it runs until triggered rather than once triggered. This is how a level ships something already on.
---@field speed integer Forward speed.
---@field fall_speed integer Vertical speed.
---@field gravity boolean Whether gravity applies to this item.
---@field collidable boolean Whether Lara can collide with this item.
---@field is_alive boolean Whether the item is a living creature with hit points remaining.
---@field is_targetable boolean Whether Lara's auto-aim can lock onto the item right now.
---@field is_killed boolean Whether the item has already been killed.
---@field is_one_shot boolean Whether the item's trigger has been spent and will never fire again.
---@field is_hostile boolean Whether this item is a creature currently hostile to Lara.
---@field is_ally boolean Whether this item is a creature that fights on Lara's side. An ally is shown in its own colour where an enemy would be.
---@field is_simulated boolean Whether the item's control routine runs each frame. Call `trx.items.Item:activate` to start it.
---@field is_in_play boolean Whether the item is live: simulated, visible and not finished - the state a targetable enemy is in. A read-only composite of the axes.
---@field was_hit boolean Whether the item was hit during the current frame.
---@field mesh_bits integer Bitmask of which of the item's meshes are drawn.
---@field touch_bits integer Bitmask of which of the item's meshes Lara is touching.
---@field anim_state trx.items.AnimState The state the item is in.
---@field goal_anim_state trx.items.AnimState The state the item is transitioning towards.
---@field room trx.rooms.Room The room containing this item.
---@field bounds trx.math.Box The item's bounding box for the frame it is on. The numbers are in the item's own frame, so they say how far the model reaches around `trx.items.Item.pos` before `trx.items.Item.rot` turns it, and they change as the item animates.
---@field joint_count integer How many joints the item's model is built from. A joint number runs from `0` up to one less than this.
---@field properties table Typed, object-specific item properties. Writing here overrides the object's default for this item only; reads fall back to the object. Iterable with `pairs()`. See [Objects](docs/trx/OBJECTS.md).
local Item = h.handle("items.Item", "ITEM", {
  fields = {
    pos = "pos",
    rot = "rot",
    anim_num = "relative_anim_num",
    frame_num = "relative_frame_num",
    num = "item_num",
    room_num = "room_num",
    hit_points = "hit_points",
    max_hit_points = "max_hit_points",
    name = "name",
    object_id = "object_id",
    is_visible = "is_visible",
    is_finished = "is_finished",
    is_present = "is_present",
    timer = "timer",
    is_triggered = "is_triggered",
    trigger_mask = "trigger_mask",
    is_reversed = "is_reversed",
    speed = "speed",
    fall_speed = "fall_speed",
    gravity = "gravity",
    collidable = "is_collidable",
    is_alive = "is_alive",
    is_targetable = "is_targetable",
    is_killed = "is_killed",
    is_one_shot = "is_one_shot",
    is_hostile = "is_hostile",
    is_ally = "is_ally",
    is_simulated = "is_simulated",
    is_in_play = "is_in_play",
    was_hit = "hit_status",
    mesh_bits = "mesh_bits",
    touch_bits = "touch_bits",
    anim_state = "current_anim_state",
    goal_anim_state = "goal_anim_state",
    -- Deliberately not exposed: box_num, floor, next_item, next_simulated, gen,
    -- anim_num, frame_num, prev_frame_num, ai_bits, ai_tag, after_death and the
    -- render flags. They are engine internals, not a contract.
  },
  writable = {
    "pos",
    "rot",
    "anim_num",
    "frame_num",
    "hit_points",
    "name",
    "is_visible",
    "is_finished",
    "timer",
    "trigger_mask",
    "is_reversed",
    "speed",
    "fall_speed",
    "gravity",
    "collidable",
    "is_one_shot",
    "mesh_bits",
    "anim_state",
    "goal_anim_state",
  },
  extensions = {
    room = function(item)
      return trx.rooms[item.room_num]
    end,
    bounds = function(item)
      return setmetatable(raw.get_bounds(item), Box)
    end,
    joint_count = raw.joint_count,
    properties = make_properties,
  },
})

---Brings the item to life, exactly as tripping a trigger on it would: its
---control routine starts running, and a creature also gets its AI, without
---which it would stand there and ignore Lara.
---
---Objects with no control routine cannot be activated, and an item that is
---already active is left alone.
function Item:activate() end

---Stops the item: its control routine no longer runs, and a creature loses its
---AI and stands down. The item stays where it is and keeps its hit points, so
---this is not a way of getting rid of it - use `trx.items.Item:destroy` for
---that.
---
---A trigger can still bring it back, and so can `trx.items.Item:activate`.
function Item:deactivate() end

---Fires a trigger at the item, exactly as a floor trigger in the level would:
---sets the code bits, and once they are all set, starts the item running.
---
---This is the one to reach for on anything a level would trigger - a door, a
---switch, an alarm - because those read their trigger before they act, and
---merely activating one leaves it running but doing nothing. Pass
---`type = trx.items.TriggerType.ANTITRIGGER` to take the trigger back instead.
---
---```lua
---trx.items[12]:trigger()
---```
---
---```lua
---trx.items[12]:trigger({ timer = 3, one_shot = true })
---```
---
---```lua
---trx.items[12]:trigger({ type = trx.items.TriggerType.ANTITRIGGER })
---```
---@param opts? trx.items.Item.trigger.opts What the trigger carries.
function Item:trigger(opts) end

local on_trigger = item_hook("on_trigger")

---Happens every time a trigger is aimed at this item, of any kind.
---`trx.events.on_trigger`, narrowed to this item.
---
---```lua
---trx.items[12]:on_trigger(function(item, trigger)
---  trx.log.info("triggered with mask " .. trigger.mask)
---end)
---```
---@param callback fun(item: trx.items.Item, trigger: trx.items.Trigger) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@trx.arg callback.trigger What the trigger carried.
---@return trx.events.Listener # The attached handler.
function Item:on_trigger(callback)
  return on_trigger(self, callback)
end

local on_hit = item_hook("on_hit")

---Happens when this item takes damage. `trx.events.on_hit`, narrowed to this
---item.
---
---```lua
---trx.items[12]:on_hit(function(item, damage)
---  trx.log.info("the item lost " .. damage .. " hit points")
---end)
---```
---@param callback fun(item: trx.items.Item, damage: integer) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@trx.arg callback.damage Hit points taken, before clamping to zero.
---@return trx.events.Listener # The attached handler.
function Item:on_hit(callback)
  return on_hit(self, callback)
end

local on_kill = item_hook("on_kill")

---Happens when damage takes this item's hit points to zero.
---`trx.events.on_kill`, narrowed to this item.
---
---```lua
---trx.items[12]:on_kill(function(item)
---  trx.log.info("the item is down")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_kill(callback)
  return on_kill(self, callback)
end

-- The item-lifecycle methods share a shape: a callback taking this item, over
-- the matching trx.events hook narrowed to it. Only the wording differs.
local lifecycle = {}
for _, event_name in ipairs({
  "on_show",
  "on_hide",
  "on_finish",
  "on_enter_sim",
  "on_leave_sim",
  "on_activate",
  "on_deactivate",
  "on_destroy",
  "on_enter_world",
  "on_leave_world",
}) do
  lifecycle[event_name] = item_hook(event_name)
end

---Happens when this item becomes visible during play. `trx.events.on_show`,
---narrowed to this item.
---
---```lua
---trx.items[12]:on_show(function(item)
---  trx.log.info("the item appeared")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_show(callback)
  return lifecycle.on_show(self, callback)
end

---Happens when this item becomes hidden during play. `trx.events.on_hide`,
---narrowed to this item.
---
---```lua
---trx.items[12]:on_hide(function(item)
---  trx.log.info("the item vanished")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_hide(callback)
  return lifecycle.on_hide(self, callback)
end

---Happens when this item finishes its run during play. `trx.events.on_finish`,
---narrowed to this item.
---
---```lua
---trx.items[12]:on_finish(function(item)
---  trx.log.info("the item finished its run")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_finish(callback)
  return lifecycle.on_finish(self, callback)
end

---Happens when this item starts being simulated during play.
---`trx.events.on_enter_sim`, narrowed to this item.
---
---```lua
---trx.items[12]:on_enter_sim(function(item)
---  trx.log.info("the item started running")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_enter_sim(callback)
  return lifecycle.on_enter_sim(self, callback)
end

---Happens when this item stops being simulated during play.
---`trx.events.on_leave_sim`, narrowed to this item.
---
---```lua
---trx.items[12]:on_leave_sim(function(item)
---  trx.log.info("the item stopped running")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_leave_sim(callback)
  return lifecycle.on_leave_sim(self, callback)
end

---Happens when this item is activated through the lifecycle front door during
---play. `trx.events.on_activate`, narrowed to this item.
---
---```lua
---trx.items[12]:on_activate(function(item)
---  trx.log.info("the item was activated")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_activate(callback)
  return lifecycle.on_activate(self, callback)
end

---Happens when this item is deactivated through the lifecycle front door
---during play. `trx.events.on_deactivate`, narrowed to this item.
---
---```lua
---trx.items[12]:on_deactivate(function(item)
---  trx.log.info("the item was deactivated")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_deactivate(callback)
  return lifecycle.on_deactivate(self, callback)
end

---Happens as this item is removed from the game during play. It can still be
---read from the handler, but not after. `trx.events.on_destroy`, narrowed to
---this item.
---
---```lua
---trx.items[12]:on_destroy(function(item)
---  trx.log.info("the item was removed")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_destroy(callback)
  return lifecycle.on_destroy(self, callback)
end

---Happens when this item enters the world during play, such as a runtime
---spawn. `trx.events.on_enter_world`, narrowed to this item.
---
---```lua
---trx.items[12]:on_enter_world(function(item)
---  trx.log.info("the item entered the world")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_enter_world(callback)
  return lifecycle.on_enter_world(self, callback)
end

---Happens when this item leaves the world during play.
---`trx.events.on_leave_world`, narrowed to this item.
---
---```lua
---trx.items[12]:on_leave_world(function(item)
---  trx.log.info("the item left the world")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens to this item.
---@trx.arg callback.item This item.
---@return trx.events.Listener # The attached handler.
function Item:on_leave_world(callback)
  return lifecycle.on_leave_world(self, callback)
end

---Removes the item from the game. Any other handle to it becomes stale.
function Item:destroy() end

---Whether the handle still refers to a live item. Reading or writing a field
---on a stale handle raises an error rather than silently operating on an
---unrelated item, so check this for a handle held across time.
---
---```lua
---local wolf = trx.items.query:of_object(trx.catalog.objects.wolf):first()
---trx.events.after_control(function()
---  if wolf:is_valid() and wolf.hit_points <= 0 then
---    trx.log.info("the wolf is down")
---  end
---end)
---```
---@return boolean # False once the item it named is gone.
function Item:is_valid() end

---Runs the object's creature death handling: the corpse stays, and `explode`
---<!--noref: explode--> bursts its meshes as a rocket or grenade would. For
---creatures; `trx.items.Item:destroy` simply removes any item from the game.
---
---```lua
---trx.items[12]:die({
---  explode = true,
---  gibs = { flame = true, smoke = true },
---  sender = trx.lara.item,
---})
---```
---@param opts? trx.items.Item.die.opts How the creature dies.
function Item:die(opts) end

---Hurts the item the way a weapon does, and reports through
---`trx.events.on_hit`, and `trx.events.on_kill` where the blow takes the last
---hit point. Writing `trx.items.Item.hit_points` reports neither. Without
---`sender`, the kill counts for the environment rather than Lara.
---<!--noref: sender-->
---
---```lua
---local lara = trx.lara.item
---lara:take_damage(lara.hit_points)
---```
---@param damage integer Hit points to take.
---@param sender? trx.items.Item Item to credit the blow to. Pass `trx.lara.item` to include the kill in Lara's level statistics.
function Item:take_damage(damage, sender) end

---Bursts the item's meshes into flying debris, the visual `trx.items.Item:die`
---produces with `trx.items.Item.die.opts.explode`, on its own. It does not
---kill or remove the item.
---
---```lua
---trx.items[12]:shatter({ gibs = { blast = true }, damage = 5 })
---```
---@param opts? trx.items.Item.shatter.opts How the meshes come apart.
function Item:shatter(opts) end

---Where one of the item's joints has reached, for the frame it is on. The
---position follows the item as it moves and animates, so a script can hang
---something off a hand or a muzzle without naming a place in the world.
---
---Raises if the model has no such joint.
---
---```lua
---local muzzle = actor:joint_pos(12, { x = 0, y = 0, z = 180 })
---```
---@param joint integer Which joint, from `0` to `trx.items.Item.joint_count` less one.
---@param offset? trx.math.Vec3 Offset from the joint, in the joint's own axes. Defaults to the joint itself.
---@return trx.math.Vec3 # World position.
function Item:joint_pos(joint, offset)
  offset = offset or { x = 0, y = 0, z = 0 }
  local x, y, z = raw.joint_pos(self, joint, offset.x, offset.y, offset.z)
  return { x = x, y = y, z = z }
end

---Distance from this item to a world position.
---@param pos trx.math.Vec3 World position.
---@return trx.math.Distance # Measured between the two positions.
function Item:distance_to(pos) end

---Reads an object property, falling back to the object's default. Prefer
---`item.properties.<name>`.
---@param name string Which property, as the object declares it.
---@return any? # The value, of the type the property is declared with.
function Item:get_property(name) end

---Overrides an object property for this item. Prefer
---`item.properties.<name> = ...`.
---@param name string Which property, as the object declares it.
---@param value any What to write, of the type the property is declared with.
function Item:set_property(name, value) end

---Names of every property this item's object declares.
---@return string[]
function Item:get_property_names() end

---Retrieves an item by number or by name.
---
---```lua
---local item = trx.items[0]
---item.name = "lara"
---local lara = trx.items["lara"]
---```
---@param key trx.items.Num An item's unique name reaches it as well.
---@return trx.items.Item? # The item, or `nil` where nothing answers to the key.
---@type fun(key: trx.items.Num): trx.items.Item?
M.get = raw.get

---@class (exact) trx.items.spawn.opts
---@field activate? boolean Bring the item to life, enabling AI for creatures.

---Creates a new item of the given object type at the given position.
---
---```lua
---local wolf = trx.items.spawn(
---  trx.catalog.objects.wolf, trx.lara.item.pos, 0, { activate = true })
---```
---@param object_id trx.catalog.objects Object type to spawn.
---@param pos trx.math.Vec3 World position. Must lie inside the level.
---@param angle_y? trx.math.Angle Facing angle.
---@param opts? trx.items.spawn.opts How to spawn it.
---@trx.default angle_y 0
---@return trx.items.Item? # `nil` if the item pool is exhausted.
---@type fun(object_id: trx.catalog.objects, pos: trx.math.Vec3, angle_y?: trx.math.Angle, opts?: trx.items.spawn.opts): trx.items.Item?
M.spawn = raw.spawn

---Returns the total number of allocated items. Same as `#trx.items`.
---@return integer # How many slots the level holds, live or not.
---@type fun(): integer
M.count = raw.count

-- Every item the level holds, each by its number.
local function enumerate()
  local out = {}
  for i = 0, raw.count() - 1 do
    local item = raw.get(i)
    if item ~= nil then
      out[#out + 1] = { i, item }
    end
  end
  return out
end

-- An object named by string resolves through the object query, so `of_object`
-- takes a name the same way a player would. An id passes straight through.
-- trx.objects is reached at call time, not required: it is up long before a
-- console line runs, and leaving it out keeps a script that only queries items
-- from dragging the object surface in behind it.
local function resolve_object(key)
  if type(key) == "number" then
    return key
  end
  return trx.objects.query:by_name(key):ids()[1]
end

-- A spatial answer as a predicate: the numbers come back as a list, and a
-- query asks after one item at a time.
local function found_in(nums)
  local set = {}
  for _, num in ipairs(nums) do
    set[num] = true
  end
  return function(num)
    return set[num] == true
  end
end

-- One of an item's own true-or-false axes, as a narrowing.
local function axis_narrowing(field)
  return trx.query.narrowing(function()
    return function(_i, item)
      return item[field]
    end
  end)
end

---A `trx.query.Query` over the items a level holds, with the narrowings below
---on top of the ones every query has. Items answer to no names of their own,
---so `trx.items.ItemQuery:of_object` is how a name reaches them.
---@class (exact) trx.items.ItemQuery: trx.query.Query
local ItemQuery = h.class("items.ItemQuery", { extends = "query.Query" })

local simulated = axis_narrowing("is_simulated")

---The item is being simulated: its control routine runs every frame.
---@return trx.query.Query # The narrowed query.
function ItemQuery:simulated()
  return simulated(self)
end

local present = axis_narrowing("is_present")

---The item is in the world, whether or not anything is simulating it.
---@return trx.query.Query # The narrowed query.
function ItemQuery:present()
  return present(self)
end

local visible = axis_narrowing("is_visible")

---The item is drawn.
---@return trx.query.Query # The narrowed query.
function ItemQuery:visible()
  return visible(self)
end

local finished = axis_narrowing("is_finished")

---The item has run its course.
---@return trx.query.Query # The narrowed query.
function ItemQuery:finished()
  return finished(self)
end

local in_play = axis_narrowing("is_in_play")

---The item is part of the game rather than set aside.
---@return trx.query.Query # The narrowed query.
function ItemQuery:in_play()
  return in_play(self)
end

local alive = axis_narrowing("is_alive")

---The item still has hit points.
---@return trx.query.Query # The narrowed query.
function ItemQuery:alive()
  return alive(self)
end

local targetable = axis_narrowing("is_targetable")

---Lara's guns can lock onto the item.
---@return trx.query.Query # The narrowed query.
function ItemQuery:targetable()
  return targetable(self)
end

local of_object = trx.query.narrowing(function(key)
  local object_id = resolve_object(key)
  return function(_i, item)
    return object_id ~= nil and item.object_id == object_id
  end
end)

---The item is of the given object, named the way a player would name it or by
---its id.
---
---```lua
---trx.items.query:of_object("wolf"):simulated():matches()
---```
---@param key any Object id, or a name `trx.objects.query` resolves.
---@return trx.query.Query # The narrowed query.
function ItemQuery:of_object(key)
  return of_object(self, key)
end

local in_room = trx.query.narrowing(function(room_num)
  return function(_i, item)
    return item.room_num == room_num
  end
end)

---The item is in the given room.
---@param room_num trx.rooms.Num
---@return trx.query.Query # The narrowed query.
function ItemQuery:in_room(room_num)
  return in_room(self, room_num)
end

local in_box = trx.query.narrowing(function(min, max)
  return found_in(raw.in_box(min, max))
end)

---The item stands inside a world-space box. The corners may come in any order.
---
---An item is tested by its position, the point it stands at, rather than by
---the box it fills. Position is all this asks after, so the rest of the query
---says what else the item must be: `trx.items.query:in_box(min,
---max):present()` asks for the ones that are in the world as well.
---
---```lua
---local guards = trx.items.query
---  :in_box({ x = 51200, y = -2048, z = 30720 }, { x = 53248, y = 0, z = 32768 })
---  :present()
---  :matches()
---```
---@param min trx.math.Vec3 One corner of the box.
---@param max trx.math.Vec3 The opposite corner.
---@return trx.query.Query # The narrowed query.
function ItemQuery:in_box(min, max)
  return in_box(self, min, max)
end

local in_sphere = trx.query.narrowing(function(centre, radius)
  return found_in(raw.in_sphere(centre, radius))
end)

---The item stands within a radius of a point. As with
---`trx.items.ItemQuery:in_box`, the item's position is the whole of the test.
---@param centre trx.math.Vec3 Middle of the sphere.
---@param radius trx.math.Distance How far out it reaches.
---@return trx.query.Query # The narrowed query.
function ItemQuery:in_sphere(centre, radius)
  return in_sphere(self, centre, radius)
end

local item_query = trx.query.new({
  enumerate = enumerate,
  id_of = function(i)
    return i
  end,
}, ItemQuery)

h.properties(M, "items", {
  query = {
    get = function()
      return item_query
    end,
  },
})

---Indexing the module reaches an item, and `#trx.items` is how many the level
---has. `pairs()` walks them in order, keyed by the item number.
---
---```lua
---for num, item in pairs(trx.items) do
---  trx.log.info(item.object_id)
---end
---```
---@type table<trx.items.Num|string, trx.items.Item?>
---@trx.key An item's unique name reaches it as well.
h.container("items", {
  base = 0,
  by_name = true,
  get = raw.get,
  count = raw.count,
}, M)
