local raw = trxc.events
local h = require("trx.internal.helpers")

-- The event types, reflected out of ENUM_MAP as any other enum is, so no number
-- is written twice. Not declared as an enum, and so not public: the named hooks
-- below are the whole surface, and a script never has to name a type.
local types = {}
for _, constant in ipairs(trxc.enum.values("LUA_EVENT_TYPE")) do
  types[constant.name] = constant.value
end

---@class (partial) trx
---@field events trx.events

---Lua scripts can listen for game events by attaching a handler to one of the
---hooks below. Attaching returns a listener id, which `trx.events.detach`
---takes.
---
---A handler attached from a level script is detached automatically when the
---level ends; one attached from a global script lives for the whole session.
---
---An event that carries a default the script may take over says so in its
---description; a handler answers such an event by returning true, and the
---default then stands down. Every other event ignores what its handlers
---return.
---@trx.module 1
---@class (partial,exact) trx.events
local M = h.module("events")

---A flip effect number, as a level editor numbers them. Not the id space of
---`trx.rooms.flip_effect`, which takes `trx.catalog.flip_effects` names.
---@trx.base 0
---@alias trx.events.FlipEffectNum integer

-- What every hook hands back. The engine keys a listener by a number, and the
-- number is the module's business rather than a script's: a listener is worth
-- holding on to, comparing and detaching, and worth nothing else.

---An attached handler. Every hook hands one back, and holding it is what makes
---the handler detachable later. A listener is spent once detached, and a level
---change spends every one a level script attached.
---@class (exact) trx.events.Listener
---@trx.readonly id
---@field id integer The number the engine keys the handler by. Two listeners of the same handler carry the same one; it is never handed out twice within a session.
---@field private _id integer
local Listener = h.class("events.Listener", {
  fields = {
    id = {
      get = function(self)
        return rawget(self, "_id")
      end,
    },
  },
})

---Stops the handler, which fires no more from here on. `trx.events.detach`
---does the same to a listener held elsewhere.
---@return boolean # Whether the handler was still attached.
function Listener:detach()
  return raw.detach(rawget(self, "_id"))
end

-- A listener carries the engine's number and nothing a script can reach.
local function listener_of(id)
  return setmetatable({ _id = id }, Listener) --[[@as trx.events.Listener]]
end

-- Each hook is a plain function closing over its event type.
local function hook(event_type)
  assert(event_type ~= nil, "events: the engine has no such event type")
  return function(callback)
    return listener_of(raw.attach(event_type, callback))
  end
end

-- trx.cutscenes is reached at call time, so its module need not load before
-- this one.
local function cutscene_hook(event_type)
  assert(event_type ~= nil, "events: the engine has no such event type")
  return function(callback)
    return listener_of(raw.attach(event_type, function(num, ...)
      callback(trx.cutscenes[num], ...)
    end))
  end
end

-- The item-lifecycle hooks share a shape: one Item argument, and a per-item
-- narrowing under trx.items.Item:on_*.
local function item_hook(event_type)
  return function(callback)
    return listener_of(raw.attach(event_type, function(item_num)
      callback(trx.items[item_num])
    end))
  end
end

---Happens as a level starts running, before its first frame is drawn. By then
---the level file is loaded, its items are set up and any savegame state has
---been applied, so this is where a script sets object properties, declares
---allies, changes room state and plays sound effects. Every kind of level
---fires it: a played level, a cutscene and the attract demo alike. The title
---screen has `trx.events.on_title_start` instead.
---
---Which level is starting is `trx.game.current_level`, whose
---`trx.game.Level.num` and `trx.game.Level.type` say where it counts and what
---kind it is. A level script already knows both, which is why the handler is
---not handed them.
---
---```lua
---trx.events.on_game_start(function(is_save)
---  trx.log.info(trx.game.current_level.title .. " is up")
---end)
---```
---@param callback fun(is_save: boolean) The function to run.
---@trx.arg callback.is_save Whether the level is being resumed from a savegame rather than started fresh. A cutscene and a demo are never resumed, and always report false.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(is_save: boolean)): trx.events.Listener
M.on_game_start = hook(types.GAME_START)

---Happens when the title screen comes up, once its level is loaded and its
---items are set up. The handler takes no arguments. `trx.events.on_game_start`
---does not fire for the title level.
---
---A title that shows a picture rather than playing its level behind the menu
---does not run its logic, so `trx.events.before_control` and
---`trx.events.after_control` handlers attached here never fire there. This
---says the menu is up; it does not promise a scene playing behind it.
---
---```lua
---trx.events.on_title_start(function()
---  trx.log.info("the menu is up")
---end)
---```
---@param callback function The function to run.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_title_start = hook(types.TITLE_START)

---Happens as the engine lets go of a level, before the handlers a level script
---attached are detached and before the world the script was written against is
---taken apart. This is where a script hands back what it set up while the
---level it set it up in is still there to read. The handler takes no
---arguments.
---
---A level change fires it for the outgoing level, and so does leaving the game
---for the title screen or for the desktop. Re-running a level's script without
---changing level fires it for the run being replaced. The unload that opens
---the first level of a session has nothing to let go of and stays quiet.
---
---```lua
---trx.events.on_level_unload(function()
---  trx.log.info("packing up")
---end)
---```
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_level_unload = hook(types.LEVEL_UNLOAD)

---Happens once for every tick the game runs, whatever is on screen: while a
---level is played, while a menu is open, over a cutscene and through a fade.
---
---This is the clock a script keeps its own state on. It is not the world
---stepping - `trx.events.before_control` is that, and it happens only while a
---level is running - and it is not a frame reaching the screen, which happens
---twice as often while frames are interpolated.
---@param callback function Called once per tick.
---@return integer # The listener id.
---@type fun(callback: function): integer
M.on_tick = hook(types.TICK)

---Happens as a key goes down, before the game reads it as an action. The
---handler takes the name of the key.
---
---A key is named by the character the player's layout prints, so the key
---labelled 5 arrives as `"5"` on every layout, and a key with a label rather
---than a character keeps the spelling the window system gives it in lower
---case, such as `"escape"` and `"left shift"`.
---
---Use this where a script needs the key itself, such as one reading a
---passcode. Use `trx.input.signals.pressed` for a game action, which respects
---what the player bound it to and answers for a controller as well.
---
---Holding a key down fires it once, and the repeats that follow arrive through
---`trx.events.on_key_repeat`.
---
---It stays quiet while a rebind is reading the keyboard. It fires while the
---console is open. Check `trx.console.is_open` where that matters. A key still
---held as the keyboard comes back fires again then, although
---`trx.input.is_key_pressed` reports nothing for it.
---
---```lua
---local typed = ""
---
---trx.events.on_key_down(function(key)
---  if key:match("^%d$") then
---    typed = typed .. key
---  elseif key == "return" then
---    trx.log.info("entered " .. typed)
---    typed = ""
---  end
---end)
---```
---@param callback function Called with the name of the key.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_key_down = hook(types.KEY_DOWN)

---Happens as the window system repeats a key the player is holding. The
---handler takes the name of the key, named as `trx.events.on_key_down` names
---it.
---
---The first press arrives through `trx.events.on_key_down` instead, and the
---repeats follow at whatever rate the player's system repeats at. A text field
---takes both, so that a held arrow keeps moving the caret; anything acting on
---a press once takes `trx.events.on_key_down` alone.
---
---It stays quiet while a rebind is reading the keyboard, and fires while the
---console is open as `trx.events.on_key_down` does.
---@param callback function Called with the name of the key.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_key_repeat = hook(types.KEY_REPEAT)

---Happens as a key comes up. The handler takes the name of the key, named as
---`trx.events.on_key_down` names it.
---
---It stays quiet while a rebind is reading the keyboard, and fires while the
---console is open as `trx.events.on_key_down` does. A key held as the game
---takes the keyboard comes up at that moment, so every press a script was told
---about still has its release.
---@param callback function Called with the name of the key.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_key_up = hook(types.KEY_UP)

---Happens as the player composes text, and carries the characters composed.
---The handler takes a string.
---
---Use this for text fields instead of `trx.events.on_key_down`. It carries
---characters with the player's modifiers and keyboard layout. It also carries
---text composed by an input method. One event can carry several characters.
---
---Editing keys carry no character and arrive through `trx.events.on_key_down`.
---Pasting goes through `trx.ui.clipboard`.
---
---It fires while the console is open, and carries what the player types there:
---the console is a script reading the keyboard rather than the game taking it.
---Check `trx.console.is_open` where a script must leave that text alone.
---
---```lua
---trx.events.on_text_input(function(text)
---  trx.log.info("composed " .. text)
---end)
---```
---@param callback function Called with the characters composed.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_text_input = hook(types.TEXT_INPUT)

---Happens as a line reaches the console, and carries the line. The handler
---takes a string.
---
---This is every message the console shows the player, whoever wrote it: a
---command reporting what it did, the engine reporting a failure, and
---`trx.console.log.info` and the rest. A message only written to the log file
---stays quiet here.
---
---Use this where a script draws a console of its own, or keeps the last few
---messages on screen.
---
---```lua
---trx.events.on_console_log(function(line)
---  trx.log.info("the console said " .. line)
---end)
---```
---@param callback function Called with the line.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_console_log = hook(types.CONSOLE_LOG)

---Happens as the console opens. The handler takes nothing.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_console_open = hook(types.CONSOLE_OPEN)

---Happens as the console closes. The handler takes nothing.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_console_close = hook(types.CONSOLE_CLOSE)

---Happens as the console drops the lines it holds, which `trx.console.clear`
---does. The handler takes nothing.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_console_clear = hook(types.CONSOLE_CLEAR)

---Happens as a controller button goes down, before the game reads it as an
---action. The handler takes the name of the button.
---
---A button keeps the name SDL gives it, such as `"a"`, `"dpup"` and
---`"leftshoulder"`, because a pad prints a different label on the same button
---depending on who made it. The `trx.input` module lists every name.
---
---Use `trx.input.signals.pressed` for a game action, which respects what the
---player bound it to and answers for the keyboard as well.
---
---It stays quiet while a rebind is reading the pad, and fires while the
---console is open as `trx.events.on_key_down` does. A button still held as the
---pad comes back fires again then, although `trx.input.is_button_pressed`
---reports nothing for it.
---@param callback function Called with the name of the button.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_button_down = hook(types.BUTTON_DOWN)

---Happens as a controller button comes up. The handler takes the name of the
---button, named as `trx.events.on_button_down` names it.
---
---It stays quiet while a rebind is reading the pad, and fires while the
---console is open as `trx.events.on_key_down` does. A button held as the game
---takes the pad comes up at that moment, so every press a script was told
---about still has its release.
---@param callback function Called with the name of the button.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_button_up = hook(types.BUTTON_UP)

---Fires once for each of the nine on-screen UI regions on every drawn frame.
---The callback receives the current `trx.ui.Region`.
---
---Use this event to reserve layout space, not to draw. Call
---`trx.ui.primitive.reserve` during this event, then draw into the assigned
---box later during `trx.events.on_ui_paint`. `trx.ui.regions.place` handles
---both steps for widgets.
---
---This event fires anywhere the game draws UI, including fades, FMVs, and
---normal gameplay. It follows the frame rate, not the game clock.
---
---```lua
---local slot = nil
---
---trx.events.on_ui_draw(function(region)
---  if region == trx.ui.Region.TOP_CENTER then
---    local w, h = trx.ui.primitive.measure_text("hello")
---    slot = trx.ui.primitive.reserve(region, w, h)
---  end
---end)
---
---trx.events.on_ui_paint(function()
---  local x, y = trx.ui.primitive.slot_box(slot)
---  if x ~= nil then
---    trx.ui.primitive.text("hello", x, y)
---  end
---end)
---```
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_ui_draw = hook(types.UI_DRAW)

---Fires after UI layout and before drawing. Reservation boxes are available
---during this event.
---
---Use this event to draw into space reserved earlier during
---`trx.events.on_ui_draw`. Primitive drawing calls are available during this
---event only.
---@param callback function Called once per painted scene.
---@return integer # The listener id.
---@type fun(callback: function): integer
M.on_ui_paint = hook(types.UI_PAINT)

---Fires on every drawn frame, after the engine interface has drawn. The
---callback receives nothing.
---
---This is `trx.events.on_ui_paint` for the layer above the engine interface. A
---script draws here where its work must cover the interface rather than sit
---under it, such as a console or a text field. The reservation boxes are the
---same ones `trx.events.on_ui_draw` asked for.
---
---`trx.ui.regions.place` picks the layer for a widget, so a script building
---with widgets has no reason to take this.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_ui_paint_over = hook(types.UI_PAINT_OVER)

---Fires on every drawn frame, after the rooms and everything standing in them,
---and before the interface.
---
---`trx.scene` draws during this event and raises anywhere else. It follows the
---frame rate, not the game clock.
---
---```lua
---trx.events.on_scene_paint(function()
---  trx.scene.sphere(trx.lara.item.pos, 512, "00ff00")
---end)
---```
---@param callback function Called once per drawn scene.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.on_scene_paint = hook(types.SCENE_PAINT)

---Happens just after Lara picks up an item.
---
---```lua
---trx.events.on_pickup(function(item_num)
---  trx.log.info(trx.items[item_num].object_id)
---end)
---```
---@param callback fun(item_num: trx.items.Num) What to run when it happens.
---@trx.arg callback.item_num The item that was picked up.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item_num: trx.items.Num)): trx.events.Listener
M.on_pickup = hook(types.PICKUP)

---Happens when the game asks the interface to announce an object.
---
---This is not the same as Lara picking something up: the gameflow handing her
---an item, and the scion and the puzzle items she assembles, all announce
---themselves the same way. A script drawing the announcement listens for this
---rather than for `trx.events.on_pickup`, so that nothing it should show goes
---unannounced.
---@param callback fun(object: trx.catalog.objects) What to run when it happens.
---@trx.arg callback.object The object to announce.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(object: trx.catalog.objects)): trx.events.Listener
M.on_show_pickup = hook(types.SHOW_PICKUP)

---Happens on every logical game frame, before the main game logic runs. The
---handler takes no arguments.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.before_control = hook(types.BEFORE_CONTROL)

---Happens on every logical game frame, after the main game logic runs. The
---handler takes no arguments.
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
M.after_control = hook(types.AFTER_CONTROL)

---Claims a `trx.events.FlipEffectNum` and happens whenever a level runs it,
---whether from a floor trigger or an animation command. Place an ordinary
---flipeffect trigger in a level editor - pad, heavy, switch and antitrigger
---all work - pick one nothing uses, and handle it here from the level's
---script.
---
---A claimed number belongs to the script for the rest of the level: its stock
---engine effect does not run, even if the handler is later detached. Unclaimed
---numbers are unaffected.
---
---Unlike the other hooks, this happens at effect execution time, in the middle
---of a game frame.
---
---```lua
---trx.events.on_flip_effect(62, function(timer, item_num)
---  trx.log.info("flipeffect 62 ran with timer " .. timer)
---end)
---```
---@param effect_num trx.events.FlipEffectNum The one to claim.
---@param callback fun(timer: integer, item_num: trx.items.Num) What to run when it happens.
---@trx.arg callback.timer A floor trigger's timer field, free for the level to use as a parameter. 0 for an animation command, which carries no timer.
---@trx.arg callback.item_num The item that ran the effect: Lara for a pad trigger, the activating object for a heavy trigger, the animating item for an animation command.
---@return trx.events.Listener # The attached handler.
function M.on_flip_effect(effect_num, callback)
  return listener_of(raw.attach(types.FLIP_EFFECT, callback, effect_num))
end

---Happens when an item changes rooms during play, which a cutscene or the
---attract demo is not. `trx.rooms.Room:on_enter` and `trx.rooms.Room:on_exit`
---are this same event, narrowed to one room.
---
---```lua
---trx.events.on_room_change(function(item, old_room_num, new_room_num)
---  trx.log.info(item.object_id .. " moved to room " .. new_room_num)
---end)
---```
---@param callback fun(item: trx.items.Item, old_room_num: trx.rooms.Num, new_room_num: trx.rooms.Num) What to run when it happens.
---@trx.arg callback.item The item that changed rooms.
---@trx.arg callback.old_room_num -1 if it had none.
---@trx.arg callback.new_room_num -1 if it left the world.
---@return trx.events.Listener # The attached handler.
function M.on_room_change(callback)
  return listener_of(
    raw.attach(
      types.ROOM_CHANGE,
      function(item_num, old_room_num, new_room_num)
        callback(trx.items[item_num], old_room_num, new_room_num)
      end
    )
  )
end

---Happens every time a trigger is aimed at an item - a floor trigger in the
---level, the `/trigger` console command, or `trx.items.Item:trigger` from a
---script - of any kind, an antitrigger included. It is the raw trigger, not a
---state change: a floor pad fires it every frame Lara stands on it, and a
---partial trigger fires it too. A cutscene or the attract demo does not.
---
---The handler runs after the trigger has been applied, so the item already
---reflects it, and changes the handler makes to the item are not overwritten.
---
---`trx.items.Item:on_trigger` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_trigger(function(item, trigger)
---  if trigger.type == trx.items.TriggerType.ANTITRIGGER then
---    trx.log.info(item.object_id .. " was antitriggered")
---  end
---end)
---```
---@param callback fun(item: trx.items.Item, trigger: trx.items.Trigger) What to run when it happens.
---@trx.arg callback.item The item the trigger was aimed at.
---@trx.arg callback.trigger What the trigger carried.
---@return trx.events.Listener # The attached handler.
function M.on_trigger(callback)
  return listener_of(
    raw.attach(types.TRIGGER, function(item_num, kind, mask, timer, one_shot)
      callback(trx.items[item_num], {
        type = kind,
        mask = mask,
        timer = timer,
        one_shot = one_shot,
      })
    end)
  )
end

---Happens when an item becomes visible during play - drawn and in the world,
---taking part in collision and targeting. It is the change that fires, not the
---state: an item already visible does not fire it again, and only a live level
---does, not a cutscene or the attract demo.
---
---`trx.items.Item:on_show` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_show(function(item)
---  trx.log.info(item.object_id .. " appeared")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that became visible.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_show = item_hook(types.SHOW)

---Happens when an item becomes hidden during play - drawn and in the world,
---taking part in collision and targeting. It is the change that fires, not the
---state: an item already hidden does not fire it again, and only a live level
---does, not a cutscene or the attract demo.
---
---`trx.items.Item:on_hide` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_hide(function(item)
---  trx.log.info(item.object_id .. " vanished")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that became hidden.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_hide = item_hook(types.HIDE)

---Happens when an item finishes its run during play - a trap that has sprung,
---a switch thrown, a one-shot object spent. It is the change that fires, once,
---and only a live level does, not a cutscene or the attract demo.
---
---`trx.items.Item:on_finish` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_finish(function(item)
---  trx.log.info(item.object_id .. " finished its run")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that finished.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_finish = item_hook(types.FINISH)

---Happens when an item starts being simulated during play - its control
---routine begins running each frame. Every path that starts an item fires it:
---a trigger, a switch, a respawn, a cheat. A trigger also fires
---`trx.events.on_activate`, which this does not.
---
---`trx.items.Item:on_enter_sim` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_enter_sim(function(item)
---  trx.log.info(item.object_id .. " started running")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that started being simulated.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_enter_sim = item_hook(types.ENTER_SIM)

---Happens when an item stops being simulated during play - its control routine
---no longer runs. It keeps its place and its state; it merely stops.
---
---`trx.items.Item:on_leave_sim` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_leave_sim(function(item)
---  trx.log.info(item.object_id .. " stopped running")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that stopped being simulated.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_leave_sim = item_hook(types.LEAVE_SIM)

---Happens when an item is activated through the lifecycle front door during
---play - the path a level trigger takes. Switches, respawns and cheats start
---an item without it, firing only `trx.events.on_enter_sim`; watch that one
---for a start of any cause.
---
---`trx.items.Item:on_activate` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_activate(function(item)
---  trx.log.info(item.object_id .. " was activated")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that was activated.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_activate = item_hook(types.ACTIVATE)

---Happens when a running item is deactivated through the lifecycle front door
---during play - the path an antitrigger takes. It fires only when the item was
---actually running.
---
---`trx.items.Item:on_deactivate` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_deactivate(function(item)
---  trx.log.info(item.object_id .. " was deactivated")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that was deactivated.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_deactivate = item_hook(types.DEACTIVATE)

---Happens as an item is removed from the game during play - a creature cleared
---away, a pickup taken, an object that has run its course. The item can still
---be read from the handler, which runs before the removal completes, but a
---handle kept past the handler goes stale.
---
---`trx.items.Item:on_destroy` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_destroy(function(item)
---  trx.log.info(item.object_id .. " was removed")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item being removed. Valid only for the duration of the handler.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_destroy = item_hook(types.DESTROY)

---Happens when an item enters the world during play - a runtime spawn, such as
---a creature an emitter releases or an item a script creates. The level's own
---items do not fire it as they load; only an arrival during a live level
---counts.
---
---`trx.items.Item:on_enter_world` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_enter_world(function(item)
---  trx.log.info(item.object_id .. " entered the world")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that entered the world.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_enter_world = item_hook(types.ENTER_WORLD)

---Happens when an item leaves the world during play - unlinked from its room,
---no longer drawn or collidable. It need not be destroyed; a destroyed item
---leaves the world on its way out, and fires this first.
---
---`trx.items.Item:on_leave_world` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_leave_world(function(item)
---  trx.log.info(item.object_id .. " left the world")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that left the world.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(item: trx.items.Item)): trx.events.Listener
M.on_leave_world = item_hook(types.LEAVE_WORLD)

---Happens when an item takes damage, Lara included. It is the raw damage that
---fires, before the item's hit points are clamped, so a fatal blow reports the
---whole amount the attacker dealt. A death that does not go through damage - a
---script writing `trx.items.Item.hit_points`, or `trx.items.Item:destroy` -
---does not report.
---
---`trx.items.Item:on_hit` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_hit(function(item, damage)
---  trx.log.info(item.object_id .. " lost " .. damage .. " hit points")
---end)
---```
---@param callback fun(item: trx.items.Item, damage: integer) What to run when it happens.
---@trx.arg callback.item The item that took the damage.
---@trx.arg callback.damage Hit points taken, before clamping to zero.
---@return trx.events.Listener # The attached handler.
function M.on_hit(callback)
  return listener_of(raw.attach(types.HIT, function(item_num, damage)
    callback(trx.items[item_num], damage)
  end))
end

---Happens when damage takes an item's hit points to zero, Lara included. It is
---the same blow `trx.events.on_hit` reports, which fires first. A death that
---does not go through damage - a script writing `trx.items.Item.hit_points`,
---or `trx.items.Item:destroy` - does not report.
---
---Some bosses fall and get back up: Willard is knocked out, Natla plays dead
---before her second stage, and the dragon lies still until Lara takes the
---dagger. Each stage brings their hit points to zero, so they report once per
---stage rather than once per boss, and the dragon reports once more for the
---dagger that ends it.
---
---`trx.items.Item:on_kill` is this same event, narrowed to one item.
---
---```lua
---trx.events.on_kill(function(item)
---  trx.log.info(item.object_id .. " is down")
---end)
---```
---@param callback fun(item: trx.items.Item) What to run when it happens.
---@trx.arg callback.item The item that was brought down.
---@return trx.events.Listener # The attached handler.
function M.on_kill(callback)
  return listener_of(raw.attach(types.KILL, function(item_num)
    callback(trx.items[item_num])
  end))
end

---Happens when a cutscene trigger fires, before the engine acts on it. A
---handler answers the trigger by returning true - having played a cutscene of
---its own, run something else, or decided nothing should run. If no handler
---answers, the engine plays the cutscene the trigger names.
---
---A trigger Lara stands on fires every frame, so this happens only for a
---cutscene that has not run yet and while none is playing. Asking counts as
---running it, however it ended, so the same handler is not asked again on the
---next frame. Clear the mark by writing `trx.cutscenes.Cutscene.is_played` to
---hear about one again.
---
---The number a trigger names need not be one the game has a cutscene for - TR4
---uses 32 to ask for a full-motion video. Those reach a handler too, and the
---engine has nothing of its own to do about them.
---
---```lua
----- only in the throne room; a flyby stands in for it elsewhere
---trx.events.on_cutscene_trigger(function(cutscene_num)
---  if cutscene_num ~= 27 then
---    return false
---  end
---  if trx.lara.item.room_num == 55 then
---    trx.cutscenes[27]:play()
---  else
---    trx.camera.play_flyby(3)
---  end
---  return true
---end)
---```
---@param callback fun(cutscene_num: trx.cutscenes.Num): boolean? What to run when it happens.
---@trx.arg callback.cutscene_num The number the trigger names, which the game need not have a cutscene for.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(cutscene_num: trx.cutscenes.Num): boolean?): trx.events.Listener
M.on_cutscene_trigger = hook(types.CUTSCENE_TRIGGER)

---Happens when a TR4 cutscene's first frame is about to show, after the fade
---out.
---@param callback fun(cutscene: trx.cutscenes.Cutscene) What to run when it happens.
---@trx.arg callback.cutscene The cutscene starting.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(cutscene: trx.cutscenes.Cutscene)): trx.events.Listener
M.on_cutscene_start = cutscene_hook(types.CUTSCENE_START)

---Happens on every frame of a TR4 cutscene, before the frame is posed.
---
---A cutscene has no items to listen to. Its actors are animation tracks, so
---the frame number is the only thing a script can act on. The original game
---keys its own cutscene events to frame numbers as well.
---
---```lua
---trx.events.on_cutscene_frame(function(cutscene, frame_num)
---  if cutscene.num == 5 and frame_num == 1350 then
---    -- something happens here
---  end
---end)
---```
---@param callback fun(cutscene: trx.cutscenes.Cutscene, frame_num: trx.cutscenes.FrameNum) What to run when it happens.
---@trx.arg callback.cutscene The cutscene the frame belongs to.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(cutscene: trx.cutscenes.Cutscene, frame_num: trx.cutscenes.FrameNum)): trx.events.Listener
M.on_cutscene_frame = cutscene_hook(types.CUTSCENE_FRAME)

---Happens once a TR4 cutscene has finished and the scene it interrupted is
---back. This is where a script decides what follows.
---
---```lua
---trx.events.on_cutscene_end(function(cutscene)
---  trx.log.info("cutscene " .. cutscene.num .. " finished")
---end)
---```
---@param callback fun(cutscene: trx.cutscenes.Cutscene) What to run when it happens.
---@trx.arg callback.cutscene The cutscene that finished.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(cutscene: trx.cutscenes.Cutscene)): trx.events.Listener
M.on_cutscene_end = cutscene_hook(types.CUTSCENE_END)

---Happens when a flyby sequence reaches its last camera and hands the view
---back. A sequence that a cutscene or the player interrupts does not fire it.
---
---```lua
---trx.events.on_flyby_end(function(sequence_num)
---  trx.camera.play_flyby(sequence_num)
---end)
---```
---@param callback fun(sequence_num: trx.camera.SequenceNum) What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: fun(sequence_num: trx.camera.SequenceNum)): trx.events.Listener
M.on_flyby_end = hook(types.FLYBY_END)

---Removes a previously attached handler, which stops firing immediately.
---`trx.events.Listener:detach` does the same to one held in hand.
---
---```lua
---local listener = trx.events.before_control(function()
---  -- handle control loop event
---end)
---trx.events.detach(listener)
---```
---@param listener trx.events.Listener What the hook handed back when the handler was attached.
---@return boolean # Whether the handler was still attached. `false` means it had already been detached, or the level it belonged to has ended.
function M.detach(listener)
  return listener:detach()
end
