local raw = trxc.input
local h = require("trx.internal.helpers")

require("trx.events")
require("trx.signal")

---@class trx
---@field input trx.input

---Module for reading input and working with player bindings.
---
---Scripts ask about roles, not physical keys. A role is a game action such as
---jumping, drawing a weapon, or opening a menu. The key or button that
---triggers it depends on the player's device and layout.
---
---Use `\{input ...}` in text to draw the binding for a role.
---
---A few functions read the keyboard and the controller themselves, for a
---script that needs the key rather than the action, such as one reading a
---passcode.
---
---A key that prints a character is named by the character the player's layout
---prints, so the key labelled 5 is `"5"` on every layout. A key with a label
---rather than a character keeps the spelling the window system gives it, in
---lower case: `"escape"`, `"return"`, `"left shift"`, `"f5"`, `"keypad 5"`.
---
---While a rebind is reading a device, hardware reads report no input. The
---presses are not saved. `trx.input.is_reserved` reports this state. Key and
---button names remain available.
---
---`trx.input.grab` gives a script exclusive input. Use `trx.console.is_open`
---when a script must ignore text entered in the console.
---
---A controller button or axis keeps the name SDL gives it, because a pad
---prints a different label on the same button depending on who made it. The
---buttons are `"a"`, `"b"`, `"x"`, `"y"`, `"back"`, `"guide"`, `"start"`,
---`"leftstick"`, `"rightstick"`, `"leftshoulder"`, `"rightshoulder"`,
---`"dpup"`, `"dpdown"`, `"dpleft"`, `"dpright"`, `"misc1"`, `"paddle1"` to
---`"paddle4"` and `"touchpad"`. The axes are `"leftx"`, `"lefty"`, `"rightx"`,
---`"righty"`, `"lefttrigger"` and `"righttrigger"`.
---@trx.module 42 Input
---@class (exact) trx.input
---@trx.readonly backend, is_listening, layout
---@field backend trx.input.Backend The current input source.
---@field layout trx.input.Layout The current layout for the current input source.
---@field is_listening boolean Whether script input capture is on.
local M = h.module("input")

---A game action the player can bind to a key or button. In text, `\{input
---...}` draws its current binding.
---@trx.bulk
---@enum trx.input.Role
local Role = {
  FORWARD = "",
  BACK = "",
  LEFT = "",
  RIGHT = "",
  STEP_LEFT = "",
  STEP_RIGHT = "",
  SLOW = "",
  CROUCH = "",
  JUMP = "",
  ACTION = "",
  DRAW = "",
  LOOK = "",
  ROLL = "",
  SPRINT = "",
  OPTION = "",
  CHANGE_TARGET = "",
  ENTER_CONSOLE = "",
  MENU_CONFIRM = "",
  MENU_BACK = "",
  MENU_LEFT = "",
  MENU_UP = "",
  MENU_DOWN = "",
  MENU_RIGHT = "",
  MENU_SKIP = "",
  MENU_TAB_LEFT = "",
  MENU_TAB_RIGHT = "",
  MENU_SHOW_INFO = "",
  MENU_FINE_ADJUST = "",
  MENU_COARSE_ADJUST = "",
  FLY_CHEAT = "",
  ITEM_CHEAT = "",
  LEVEL_SKIP_CHEAT = "",
  TURBO_CHEAT = "",
  FAST_FORWARD_CHEAT = "",
  SLOW_MOTION_CHEAT = "",
  SAVE = "",
  LOAD = "",
  QUICK_SAVE = "",
  QUICK_LOAD = "",
  SCREENSHOT = "",
  TOGGLE_FPS_COUNTER = "",
  TOGGLE_FULLSCREEN = "",
  EQUIP_PISTOLS = "",
  EQUIP_SHOTGUN = "",
  EQUIP_MAGNUMS = "",
  EQUIP_AUTOS = "",
  EQUIP_DESERT_EAGLE = "",
  EQUIP_UZIS = "",
  EQUIP_HARPOON = "",
  EQUIP_M16 = "",
  EQUIP_MP5 = "",
  EQUIP_GRENADE_LAUNCHER = "",
  EQUIP_ROCKET_LAUNCHER = "",
  USE_SMALL_MEDI = "",
  USE_BIG_MEDI = "",
  USE_FLARE = "",
  USE_BINOCULARS = "",
  PAUSE = "",
  TOGGLE_PHOTO_MODE = "",
  TOGGLE_UI = "",
  TOGGLE_BILINEAR_FILTER = "",
  CYCLE_LIGHTING_MODEL = "",
  CHANGE_OUTFIT = "",
  TOGGLE_TRAPEZOID_FILTER = "",
  TOGGLE_WIREFRAME = "",
  TOGGLE_TEXTURES = "",
  SWITCH_UPSCALING = "",
  SWITCH_BORDERS = "",
  RESET_BINDINGS = "",
  UNBIND_KEY = "",
  CAMERA_FORWARD = "",
  CAMERA_BACK = "",
  CAMERA_LEFT = "",
  CAMERA_RIGHT = "",
  CAMERA_UP = "",
  CAMERA_DOWN = "",
  CAMERA_RESET = "",
}
M.Role = h.enum("input.Role", "INPUT_ROLE", Role)

---An input source, such as keyboard, controller, or touch.
---@enum trx.input.Backend
local Backend = {
  KEYBOARD = "The keyboard, with the mouse.",
  CONTROLLER = "A game controller.",
  TOUCH = "The on-screen controls.",
}
M.Backend = h.enum("input.Backend", "INPUT_BACKEND", Backend)

---Where the player lands after skipping a scene. It decides which roles a skip
---holds inactive.
---@enum trx.input.SkipContext
local SkipContext = {
  TO_SCREEN = "A menu or another screen follows. Every skip role is held.",
  TO_GAME = "Gameplay follows. Action stays active, so a held action reaches Lara.",
  IN_GAME = "The scene plays during gameplay. Look stays active for the camera.",
}
M.SkipContext = h.enum("input.SkipContext", "INPUT_SKIP_CONTEXT", SkipContext)

---A saved set of bindings for one input source. The default layout is
---read-only; the custom layouts belong to the player.
---@enum trx.input.Layout
local Layout = {
  DEFAULT = "The bindings the game ships with.",
  CUSTOM_1 = "The player's first layout.",
  CUSTOM_2 = "The player's second layout.",
  CUSTOM_3 = "The player's third layout.",
}
M.Layout = h.enum("input.Layout", "INPUT_LAYOUT", Layout)

---Which of the two bindings a role can use.
---@trx.base 1
---@alias trx.input.Slot integer

---The slot, input source, and layout of a role binding.
---@trx.record
---@class trx.input.Binding
---@field slot? trx.input.Slot Binding slot. Defaults to the first.
---@field backend? trx.input.Backend Input source. Defaults to the current one.
---@field layout? trx.input.Layout Layout. Defaults to the current one.
---@trx.default slot 1

local function binding_args(opts_or_slot, backend, layout)
  if type(opts_or_slot) == "table" then
    return opts_or_slot.slot, opts_or_slot.backend, opts_or_slot.layout
  end
  return opts_or_slot, backend, layout
end

local function layout_args(opts_or_backend, layout)
  if type(opts_or_backend) == "table" then
    return opts_or_backend.backend, opts_or_backend.layout
  end
  return opts_or_backend, layout
end

-------------------------------------------------------------------------------
-- what the player is doing
-------------------------------------------------------------------------------

---Whether a role is active right now.
---
---This stays true while the player holds the bound key or button. Use it for
---actions that continue while held.
---@param role trx.input.Role The role to ask about.
---@return boolean # Whether the role is active.
---@type fun(role: trx.input.Role): boolean
M.is_held = raw.is_held

---Whether a role became active this frame.
---
---This is true for one frame only. Use it for actions that happen once per
---press.
---@param role trx.input.Role The role to ask about.
---@return boolean # Whether the role was pressed.
---@type fun(role: trx.input.Role): boolean
M.is_pressed = raw.is_pressed

---Whether any role is active right now.
---
---Use this to wait for the player to let go before reading input for something
---else, such as a rebind.
---@return boolean # Whether anything is held.
---@type fun(): boolean
M.is_anything_held = raw.is_anything_held

---Keeps a handled role inactive until the player releases it.
---
---Use this after a script handles a press, so the same press does not reach
---other input code or fire again while held.
---@param role trx.input.Role The role to take.
---@type fun(role: trx.input.Role)
M.hold_off = raw.hold_off

---Keeps each role that can skip a scene inactive until the player releases it.
---
---Use this after a script ends a scene on a skip press, so the same press does
---not act on what comes after the scene.
---@param context trx.input.SkipContext Where the player lands after the skip.
---@type fun(context: trx.input.SkipContext)
M.hold_off_skip = raw.hold_off_skip

-------------------------------------------------------------------------------
-- Read the keyboard as hardware.
-------------------------------------------------------------------------------

---Whether the game has the keyboard and the pad rather than the player.
---
---This is true while a rebind is reading input. Every hardware read reports
---nothing then, so a script that would otherwise answer an empty keypad can
---tell the two apart.
---
---A script holding the devices with `trx.input.grab` is not this, and
---`trx.input.is_grabbed` reports that instead. The console is one such script.
---@return boolean # Whether the game has the devices.
---@type fun(): boolean
M.is_reserved = raw.is_reserved

---Whether a key is down right now.
---
---This reads the keyboard rather than the player's bindings, so it answers for
---the key itself and says nothing about a controller. Prefer
---`trx.input.is_held` for a game action: it follows what the player bound and
---works on every device.
---
---Reports false while `trx.input.is_reserved` is true. A name no key on the
---player's layout carries raises.
---@param key string The key to ask about.
---@return boolean # Whether the key is down.
---@type fun(key: string): boolean
M.is_key_held = raw.is_key_held

---Whether a key went down in this frame.
---
---This is true for one frame only. `trx.events.on_key_down` reports the same
---presses without a script naming the keys it cares about in advance.
---
---Reports false while `trx.input.is_reserved`, and a press that arrived then
---is not kept for afterwards. A key still held as the game gives the keyboard
---back fires `trx.events.on_key_down` but reports no press here.
---
---A name no key on the player's layout carries raises.
---@param key string The key to ask about.
---@return boolean # Whether the key went down.
---@type fun(key: string): boolean
M.is_key_pressed = raw.is_key_pressed

---Whether the player's layout has a key of that name.
---
---Use this to check a name a mod's own settings supplied, rather than letting
---`trx.input.is_key_held` raise on it.
---@param key string The name to check.
---@return boolean # Whether a key carries it.
---@type fun(key: string): boolean
M.is_key_known = raw.is_key_known

-------------------------------------------------------------------------------
-- Read the controller as hardware.
-------------------------------------------------------------------------------

---Whether a controller button is down right now.
---
---This reads the pad rather than the player's bindings, so it answers for the
---button itself. Prefer `trx.input.is_held` for a game action: it follows what
---the player bound and works on every device.
---
---Reports false while `trx.input.is_reserved`. A name SDL does not know
---raises.
---@param button string The button to ask about.
---@return boolean # Whether the button is down.
---@type fun(button: string): boolean
M.is_button_held = raw.is_button_held

---Whether a controller button went down in this frame.
---
---This is true for one frame only. `trx.events.on_button_down` reports the
---same presses without a script naming the buttons it cares about in advance.
---
---Reports false while `trx.input.is_reserved`, and a press that arrived then
---is not kept for afterwards. A button still held as the game gives the pad
---back fires `trx.events.on_button_down` but reports no press here.
---
---A name SDL does not know raises.
---@param button string The button to ask about.
---@return boolean # Whether the button went down.
---@type fun(button: string): boolean
M.is_button_pressed = raw.is_button_pressed

---Whether a controller button carries that name.
---@param button string The name to check.
---@return boolean # Whether a button carries it.
---@type fun(button: string): boolean
M.is_button_known = raw.is_button_known

---Where a controller axis stands, from -1 to 1.
---
---A stick reaches -1 left or up and 1 right or down. A trigger runs from 0 at
---rest to 1 held down. An axis reads 0 while the pad is unplugged and while
---`trx.input.is_reserved`.
---
---A name SDL does not know raises.
---@param axis string The axis to read.
---@return number # Where the axis stands.
---@type fun(axis: string): number
M.axis = raw.axis

---Whether a controller axis carries that name.
---@param axis string The name to check.
---@return boolean # Whether an axis carries it.
---@type fun(axis: string): boolean
M.is_axis_known = raw.is_axis_known

-------------------------------------------------------------------------------
-- Hold roles inactive.
-------------------------------------------------------------------------------

local suppressed = {}
local epoch = 0

---A set of roles a script holds inactive.
---@class (exact) trx.input.Suppression
local Suppression = h.class("input.Suppression")

---Gives the roles back to the player.
---
---A role stays inactive while any other suppression still names it.
---@return boolean # Whether the suppression was still holding anything.
function Suppression:release()
  return rawget(self, "_release")()
end

---Holds roles inactive until the returned suppression is released.
---
---The game does not act on the role, and `trx.input.is_held` and
---`trx.input.signals` report it inactive as well. Use this to take an action
---away for as long as a script needs it gone, such as while the player works a
---puzzle.
---
---Only the roles named are affected. A suppressed movement role still moves
---the menu cursor, so a script that wants both suppresses both.
---
---Suppressions are released when the level unloads.
---
---```lua
---local held = trx.input.suppress(
---  trx.input.Role.JUMP,
---  trx.input.Role.ROLL
---)
---
---local function on_puzzle_solved()
---  held:release()
---end
---```
---@param ... trx.input.Role The roles to hold inactive.
---@return trx.input.Suppression # The running suppression.
function M.suppress(...)
  local roles = table.pack(...)
  if roles.n == 0 then
    error("at least one role is required", 2)
  end

  local handle = setmetatable({}, Suppression)
  local own_epoch = epoch
  for i = 1, roles.n do
    local role = roles[i]
    local count = (suppressed[role] or 0) + 1
    suppressed[role] = count
    if count == 1 then
      raw.suppress(role, true)
    end
  end

  rawset(handle, "_release", function()
    if own_epoch ~= epoch then
      return false
    end
    own_epoch = -1
    for i = 1, roles.n do
      local role = roles[i]
      local count = suppressed[role] - 1
      suppressed[role] = count > 0 and count or nil
      if count == 0 then
        raw.suppress(role, false)
      end
    end
    return true
  end)
  return handle
end

---Whether a role is held inactive by any suppression.
---@param role trx.input.Role The role to ask about.
---@return boolean # Whether the role is held.
---@type fun(role: trx.input.Role): boolean
M.is_suppressed = raw.is_suppressed

-------------------------------------------------------------------------------
-- what the player is on
-------------------------------------------------------------------------------

h.properties(M, "input", {
  backend = { get = raw.backend },
  layout = { get = raw.layout },
})

---Whether an input source is enabled.
---
---Disabled sources are not read or shown in the controls dialog.
---@param backend trx.input.Backend The input source to ask about.
---@return boolean # Whether the input source is enabled.
---@type fun(backend: trx.input.Backend): boolean
M.is_backend_enabled = raw.is_backend_enabled

-------------------------------------------------------------------------------
-- what a role is called and what it is bound to
-------------------------------------------------------------------------------

---The name the game shows for a role, in the player's language.
---@param role trx.input.Role The role to name.
---@return string # The name of the role.
---@type fun(role: trx.input.Role): string
M.role_name = raw.role_name

---The name the game shows for a layout, in the player's language.
---@param layout? trx.input.Layout The layout to name. Defaults to the current one.
---@return string # The name of the layout.
---@type fun(layout?: trx.input.Layout): string
M.layout_name = raw.layout_name

---Text for the key or button bound to a role.
---
---This is the text drawn by `\{input ...}`: a glyph when one exists, otherwise
---a key name. Empty bindings return nil.
---
---Use `trx.input.Binding` to choose a slot, input source, or layout without
---placeholder nils. Positional arguments still work.
---@param role trx.input.Role The role to ask about.
---@param opts? trx.input.Slot|trx.input.Binding Binding to read. Defaults to the first slot on the current source and layout.
---@param backend? trx.input.Backend The input source to read. Defaults to the current one.
---@param layout? trx.input.Layout The layout to read. Defaults to the current one.
---@return string? # The key text, or nil if the binding is empty.
function M.key_name(role, opts, backend, layout)
  return raw.key_name(role, binding_args(opts, backend, layout))
end

---Whether `trx.input.key_name` has text to draw for a role binding.
---
---Use this to hide prompts for unbound roles.
---
---Use `trx.input.Binding` to choose a slot, input source, or layout without
---placeholder nils. Positional arguments still work.
---@param role trx.input.Role The role to ask about.
---@param opts? trx.input.Slot|trx.input.Binding Binding to read. Defaults to the first slot on the current source and layout.
---@param backend? trx.input.Backend The input source to read. Defaults to the current one.
---@param layout? trx.input.Layout The layout to read. Defaults to the current one.
---@return boolean # Whether the binding has text to draw.
function M.has_glyph(role, opts, backend, layout)
  return raw.key_name(role, binding_args(opts, backend, layout)) ~= nil
end

-------------------------------------------------------------------------------
-- changing what a role is bound to
-------------------------------------------------------------------------------

---Whether the player can change a role's binding.
---
---Roles reserved by the game cannot be rebound and do not count as conflicts.
---@param role trx.input.Role The role to ask about.
---@return boolean # Whether it can be bound.
---@type fun(role: trx.input.Role): boolean
M.is_rebindable = raw.is_rebindable

---Whether the player can leave a role without a binding.
---@param role trx.input.Role The role to ask about.
---@return boolean # Whether it can be left unbound.
---@type fun(role: trx.input.Role): boolean
M.is_unbindable = raw.is_unbindable

---Whether another role uses the same binding in the same layout.
---
---Use `trx.input.Binding` to choose an input source or layout without
---placeholder nils. Positional arguments still work.
---@param role trx.input.Role The role to ask about.
---@param opts? trx.input.Backend|trx.input.Binding Binding to check. Defaults to the current source and layout.
---@param layout? trx.input.Layout The layout to read. Defaults to the current one.
---@return boolean # Whether the binding is used twice.
function M.is_conflicted(role, opts, layout)
  return raw.is_conflicted(role, layout_args(opts, layout))
end

---Turns script input capture on or off.
---
---While capture is on, scripts can read or bind input without the game acting
---on the same input. Turn capture off as soon as the input is handled.
---@param enabled boolean Whether script input capture is enabled.
---@type fun(enabled: boolean)
M.listen = raw.listen

---Runs a function with script input capture on.
---
---Restores the previous capture state after the function returns or raises an
---error. Returns the function's results.
---@param fn function Function to run while input is captured.
---@return any # What the function returned.
function M.with_listen(fn)
  local was_listening = raw.is_listening()
  raw.listen(true)
  local result = table.pack(pcall(fn))
  raw.listen(was_listening)
  if not result[1] then
    error(result[2], 0)
  end
  return table.unpack(result, 2, result.n)
end

h.properties(M, "input", {
  is_listening = { get = raw.is_listening },
})

---Binds a role to the key or button the player is holding.
---
---Returns false if no input is held. Call it each frame while waiting for
---input, with `trx.input.listen` on or from inside `trx.input.with_listen`.
---The default layout is read-only.
---
---Use `trx.input.Binding` to choose a slot, input source, or layout without
---placeholder nils. Positional arguments still work.
---@param role trx.input.Role The role to bind.
---@param opts? trx.input.Slot|trx.input.Binding Binding to write. Defaults to the first slot on the current source and layout.
---@param backend? trx.input.Backend The input source to bind on. Defaults to the current one.
---@param layout? trx.input.Layout The layout to write. Defaults to the current one.
---@return boolean # Whether a key was taken.
function M.bind_pressed(role, opts, backend, layout)
  return raw.bind_pressed(role, binding_args(opts, backend, layout))
end

---A binding capture waiting for the player to press something.
---@class (exact) trx.input.Capture
local Capture = h.class("input.Capture")

---Stops the capture and leaves the binding as it was.
---
---Turning capture off is part of this, so a script that gives up does not have
---to do it itself.
---@return boolean # Whether the capture was still running.
function Capture:cancel()
  return rawget(self, "_finish")(false)
end

---Binds a role to the next key or button the player presses.
---
---The capture spans frames: it waits for the player to let go of what is
---already down, turns capture on, and takes the first press after that. The
---previous capture state is restored when it lands or when the capture is
---cancelled.
---
---Use this instead of `trx.input.listen` and `trx.input.bind_pressed`, which
---only answer for the frame they run on. The default layout is read-only.
---
---Use `trx.input.Binding` to choose a slot, input source, or layout without
---placeholder nils.
---@param role trx.input.Role The role to bind.
---@param opts? trx.input.Slot|trx.input.Binding Binding to write. Defaults to the first slot on the current source and layout.
---@param done? fun(bound: boolean) Called when the capture ends.
---@trx.arg done.bound Whether a key was taken.
---@return trx.input.Capture # The running capture.
function M.capture(role, opts, done)
  local slot, backend, layout = binding_args(opts)
  backend = backend or raw.backend()
  layout = layout or raw.layout(backend)
  -- Both raise from bind_pressed too, but a capture reads it a frame later,
  -- where the error would reach the script from a tick instead of from here.
  if layout == M.Layout.DEFAULT then
    error("the default layout cannot be changed", 2)
  end
  if not raw.is_rebindable(role) then
    error("the role cannot be rebound", 2)
  end

  local capture = setmetatable({}, Capture)
  local was_listening = raw.is_listening()
  local waiting_for_release = true
  local listener

  local function finish(bound)
    if listener == nil then
      return false
    end
    listener:detach()
    listener = nil
    raw.listen(was_listening)
    if done ~= nil then
      done(bound)
    end
    return true
  end

  listener = trx.signal.tick:on(function()
    if waiting_for_release then
      if raw.is_anything_held() then
        return
      end
      waiting_for_release = false
      raw.listen(true)
    elseif raw.bind_pressed(role, slot, backend, layout) then
      finish(true)
    end
  end)

  rawset(capture, "_finish", finish)
  return capture
end

---Clears one role binding.
---
---The default layout is read-only, and roles reserved by the game cannot be
---left unbound.
---
---Use `trx.input.Binding` to choose a slot, input source, or layout without
---placeholder nils. Positional arguments still work.
---@param role trx.input.Role The role to unbind.
---@param opts? trx.input.Slot|trx.input.Binding Binding to clear. Defaults to the first slot on the current source and layout.
---@param backend? trx.input.Backend The input source to write. Defaults to the current one.
---@param layout? trx.input.Layout The layout to write. Defaults to the current one.
function M.unbind(role, opts, backend, layout)
  return raw.unbind(role, binding_args(opts, backend, layout))
end

---Restores a custom layout to the default bindings.
---
---Use `trx.input.Binding` to choose an input source or layout without
---placeholder nils. Positional arguments still work.
---@param opts? trx.input.Backend|trx.input.Binding Layout to reset. Defaults to the current source and layout.
---@param layout? trx.input.Layout The layout to write. Defaults to the current one.
function M.reset_layout(opts, layout)
  return raw.reset_layout(layout_args(opts, layout))
end

-------------------------------------------------------------------------------
-- the same answers as signals
-------------------------------------------------------------------------------

---Input roles as signals.
---
---Each role has one shared signal, so several consumers of the same role use
---one read per tick.
---@class (exact) trx.input.signals
M.signals = h.namespace("input.signals")

-------------------------------------------------------------------------------
-- Take the devices from the game.
-------------------------------------------------------------------------------

local grabs = 0
local grab_epoch = 0

---The devices a script holds, taken from the game.
---@class (exact) trx.input.Grab
local Grab = h.class("input.Grab")

---Gives the devices back to the game.
---
---The game reads them again once every grab is released.
---@return boolean # Whether the grab was still holding the devices.
function Grab:release()
  return rawget(self, "_release")()
end

---Takes the keyboard and the pad from the game until the returned grab is
---released.
---
---The game stops responding to the devices while the script can still read
---them. Use this for text fields, consoles, and passcode boxes. Release the
---grab when the script no longer needs the devices.
---
---`trx.input.suppress` removes one action. A grab takes both devices. The game
---still has priority while `trx.input.is_reserved` is true. Grabs are released
---when the level unloads.
---
---```lua
---local grab = trx.input.grab()
---
---if typed == passcode then
---  grab:release()
---end
---```
---@return trx.input.Grab # The running grab.
function M.grab()
  local handle = setmetatable({}, Grab)
  local own_epoch = grab_epoch
  grabs = grabs + 1
  if grabs == 1 then
    raw.hold(true)
  end

  rawset(handle, "_release", function()
    if own_epoch ~= grab_epoch then
      return false
    end
    own_epoch = -1
    grabs = grabs - 1
    if grabs == 0 then
      raw.hold(false)
    end
    return true
  end)
  return handle
end

---Whether a script holds the devices.
---
---This reports what `trx.input.grab` took, and says nothing about
---`trx.input.is_reserved`, which is the game holding them instead.
---@return boolean # Whether a script holds the devices.
---@type fun(): boolean
M.is_grabbed = raw.is_held_by_script

-- One signal per role, kept for as long as the game runs: a role a level asked
-- about is a role the next level can ask about too.
local held = {}
local pressed = {}
local followed = {}
local ticking = false

local function shared(cache, role, read)
  local existing = cache[role]
  if existing ~= nil then
    return existing
  end
  -- The signal is kept for as long as the game runs, so the read behind it has
  -- to be too: `trx.signal.polled` scopes its read to the level that first
  -- asked, and would leave every later reader holding a signal that never moves
  -- again. One listener drives every role, so following one more role costs one
  -- more read rather than one more listener.
  local created = trx.signal.new(read(role))
  cache[role] = created
  followed[#followed + 1] = { signal = created, role = role, read = read }
  if not ticking then
    ticking = true
    trx.signal.tick:on(function()
      for i = 1, #followed do
        local entry = followed[i]
        entry.signal:set(entry.read(entry.role))
      end
    end)
  end
  return created
end

---A signal for whether a role is active.
---
---It is true while the player holds the bound key or button. It changes when
---the role becomes active and when it stops.
---@param role trx.input.Role The role to follow.
---@return trx.signal.Signal # The role's signal.
function M.signals.held(role)
  return shared(held, role, raw.is_held)
end

---A signal for when a role becomes active.
---
---It is true for one tick only, so listeners run once per press.
---@param role trx.input.Role The role to follow.
---@return trx.signal.Signal # The role's signal.
function M.signals.pressed(role)
  return shared(pressed, role, raw.is_pressed)
end

trx.events.on_level_unload(function()
  suppressed = {}
  grabs = 0
  epoch = epoch + 1
  grab_epoch = grab_epoch + 1
  raw.clear_suppressed()
  raw.hold(false)
end)
