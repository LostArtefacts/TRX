local raw = trxc.input
local api = trx.api

require("trx.signal")

api.module("input", {
  order = 41,
  title = "Input",
  description = [[
Module for reading input and working with player bindings.

Scripts ask about roles, not physical keys. A role is a game action such as
jumping, drawing a weapon, or opening a menu. The key or button that triggers it
depends on the player's device and layout.

Use `\{input ...}` in text to draw the binding for a role.]],
})

api.enum("input.Role", {
  backing = "INPUT_ROLE",
  bulk = true,
  description = "A game action the player can bind to a key or button. In text, "
    .. "`\\{input ...}` draws its current binding.",
})

api.enum("input.Backend", {
  backing = "INPUT_BACKEND",
  description = "An input source, such as keyboard, controller, or touch.",
  values = {
    KEYBOARD = "The keyboard, with the mouse.",
    CONTROLLER = "A game controller.",
    TOUCH = "The on-screen controls.",
  },
})

local Layout = api.enum("input.Layout", {
  backing = "INPUT_LAYOUT",
  description = [[
A saved set of bindings for one input source. The default layout is read-only;
the custom layouts belong to the player.]],
  values = {
    DEFAULT = "The bindings the game ships with.",
    CUSTOM_1 = "The player's first layout.",
    CUSTOM_2 = "The player's second layout.",
    CUSTOM_3 = "The player's third layout.",
  },
})

api.number("input.Slot", {
  base = 1,
  description = "Which of the two bindings a role can use.",
})

api.type("input.Binding", {
  record = true,
  description = "The slot, input source, and layout of a role binding.",
  fields = {
    slot = {
      type = "input.Slot",
      optional = true,
      default = 1,
      description = "Binding slot. Defaults to the first.",
    },
    backend = {
      type = "input.Backend",
      optional = true,
      description = "Input source. Defaults to the current one.",
    },
    layout = {
      type = "input.Layout",
      optional = true,
      description = "Layout. Defaults to the current one.",
    },
  },
})

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

api.define("input.is_held", {
  description = [[
Whether a role is active right now.

This stays true while the player holds the bound key or button. Use it for
actions that continue while held.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
  },
  returns = { type = "boolean", description = "Whether the role is active." },
  impl = raw.is_held,
})

api.define("input.is_pressed", {
  description = [[
Whether a role became active this frame.

This is true for one frame only. Use it for actions that happen once per press.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
  },
  returns = { type = "boolean", description = "Whether the role was pressed." },
  impl = raw.is_pressed,
})

api.define("input.is_anything_held", {
  description = [[
Whether any role is active right now.

Use this to wait for the player to let go before reading input for something
else, such as a rebind.]],
  returns = { type = "boolean", description = "Whether anything is held." },
  impl = raw.is_anything_held,
})

api.define("input.hold_off", {
  description = [[
Keeps a handled role inactive until the player releases it.

Use this after a script handles a press, so the same press does not reach other
input code or fire again while held.]],
  params = {
    { name = "role", type = "input.Role", description = "The role to take." },
  },
  impl = raw.hold_off,
})

-------------------------------------------------------------------------------
-- what the player is on
-------------------------------------------------------------------------------

api.property("input.backend", {
  type = "input.Backend",
  description = "The current input source.",
  get = raw.backend,
})

api.property("input.layout", {
  type = "input.Layout",
  description = "The current layout for the current input source.",
  get = raw.layout,
})

api.define("input.is_backend_enabled", {
  description = [[
Whether an input source is enabled.

Disabled sources are not read or shown in the controls dialog.]],
  params = {
    {
      name = "backend",
      type = "input.Backend",
      description = "The input source to ask about.",
    },
  },
  returns = {
    type = "boolean",
    description = "Whether the input source is enabled.",
  },
  impl = raw.is_backend_enabled,
})

-------------------------------------------------------------------------------
-- what a role is called and what it is bound to
-------------------------------------------------------------------------------

api.define("input.role_name", {
  description = "The name the game shows for a role, in the player's language.",
  params = {
    { name = "role", type = "input.Role", description = "The role to name." },
  },
  returns = { type = "string", description = "The name of the role." },
  impl = raw.role_name,
})

api.define("input.layout_name", {
  description = "The name the game shows for a layout, in the player's language.",
  params = {
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to name. Defaults to the current one.",
      optional = true,
    },
  },
  returns = { type = "string", description = "The name of the layout." },
  impl = raw.layout_name,
})

api.define("input.key_name", {
  description = [[
Text for the key or button bound to a role.

This is the text drawn by `\{input ...}`: a glyph when one exists, otherwise a
key name. Empty bindings return nil.

Use `trx.input.Binding` to choose a slot, input source, or layout without
placeholder nils. Positional arguments still work.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
    {
      name = "opts",
      type = { "input.Slot", "input.Binding" },
      description = "Binding to read. Defaults to the first slot on the current source and layout.",
      optional = true,
    },
    {
      name = "backend",
      type = "input.Backend",
      description = "The input source to read. Defaults to the current one.",
      optional = true,
    },
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to read. Defaults to the current one.",
      optional = true,
    },
  },
  returns = {
    type = "string",
    nullable = true,
    description = "The key text, or nil if the binding is empty.",
  },
  impl = function(role, opts_or_slot, backend, layout)
    return raw.key_name(role, binding_args(opts_or_slot, backend, layout))
  end,
})

api.define("input.has_glyph", {
  description = [[
Whether `trx.input.key_name` has text to draw for a role binding.

Use this to hide prompts for unbound roles.

Use `trx.input.Binding` to choose a slot, input source, or layout without
placeholder nils. Positional arguments still work.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
    {
      name = "opts",
      type = { "input.Slot", "input.Binding" },
      description = "Binding to read. Defaults to the first slot on the current source and layout.",
      optional = true,
    },
    {
      name = "backend",
      type = "input.Backend",
      description = "The input source to read. Defaults to the current one.",
      optional = true,
    },
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to read. Defaults to the current one.",
      optional = true,
    },
  },
  returns = {
    type = "boolean",
    description = "Whether the binding has text to draw.",
  },
  impl = function(role, opts_or_slot, backend, layout)
    return raw.key_name(role, binding_args(opts_or_slot, backend, layout))
      ~= nil
  end,
})

-------------------------------------------------------------------------------
-- changing what a role is bound to
-------------------------------------------------------------------------------

api.define("input.is_rebindable", {
  description = [[
Whether the player can change a role's binding.

Roles reserved by the game cannot be rebound and do not count as conflicts.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
  },
  returns = { type = "boolean", description = "Whether it can be bound." },
  impl = raw.is_rebindable,
})

api.define("input.is_unbindable", {
  description = "Whether the player can leave a role without a binding.",
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
  },
  returns = {
    type = "boolean",
    description = "Whether it can be left unbound.",
  },
  impl = raw.is_unbindable,
})

api.define("input.is_conflicted", {
  description = [[
Whether another role uses the same binding in the same layout.

Use `trx.input.Binding` to choose an input source or layout without placeholder
nils. Positional arguments still work.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to ask about.",
    },
    {
      name = "opts",
      type = { "input.Backend", "input.Binding" },
      description = "Binding to check. Defaults to the current source and layout.",
      optional = true,
    },
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to read. Defaults to the current one.",
      optional = true,
    },
  },
  returns = {
    type = "boolean",
    description = "Whether the binding is used twice.",
  },
  impl = function(role, opts_or_backend, layout)
    return raw.is_conflicted(role, layout_args(opts_or_backend, layout))
  end,
})

api.define("input.listen", {
  description = [[
Turns script input capture on or off.

While capture is on, scripts can read or bind input without the game acting on
the same input. Turn capture off as soon as the input is handled.]],
  params = {
    {
      name = "enabled",
      type = "boolean",
      description = "Whether script input capture is enabled.",
    },
  },
  impl = raw.listen,
})

api.define("input.with_listen", {
  description = [[
Runs a function with script input capture on.

Restores the previous capture state after the function returns or raises an
error. Returns the function's results.]],
  params = {
    {
      name = "fn",
      type = "function",
      description = "Function to run while input is captured.",
    },
  },
  returns = { type = "any", description = "What the function returned." },
  impl = function(fn)
    local was_listening = raw.is_listening()
    raw.listen(true)
    local result = table.pack(xpcall(fn, debug.traceback))
    raw.listen(was_listening)
    if not result[1] then
      error(result[2], 0)
    end
    return table.unpack(result, 2, result.n)
  end,
})

api.property("input.is_listening", {
  type = "boolean",
  description = "Whether script input capture is on.",
  get = raw.is_listening,
})

api.define("input.bind_pressed", {
  description = [[
Binds a role to the key or button the player is holding.

Returns false if no input is held. Call it each frame while waiting for input,
with `trx.input.listen` on or from inside `trx.input.with_listen`. The default
layout is read-only.

Use `trx.input.Binding` to choose a slot, input source, or layout without
placeholder nils. Positional arguments still work.]],
  params = {
    { name = "role", type = "input.Role", description = "The role to bind." },
    {
      name = "opts",
      type = { "input.Slot", "input.Binding" },
      description = "Binding to write. Defaults to the first slot on the current source and layout.",
      optional = true,
    },
    {
      name = "backend",
      type = "input.Backend",
      description = "The input source to bind on. Defaults to the current one.",
      optional = true,
    },
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to write. Defaults to the current one.",
      optional = true,
    },
  },
  returns = { type = "boolean", description = "Whether a key was taken." },
  impl = function(role, opts_or_slot, backend, layout)
    return raw.bind_pressed(role, binding_args(opts_or_slot, backend, layout))
  end,
})

local Capture = api.type("input.Capture", {
  description = "A binding capture waiting for the player to press something.",
  methods = {
    cancel = {
      description = [[
Stops the capture and leaves the binding as it was.

Turning capture off is part of this, so a script that gives up does not have to
do it itself.]],
      returns = {
        type = "boolean",
        description = "Whether the capture was still running.",
      },
      impl = function(self)
        return rawget(self, "_finish")(false)
      end,
    },
  },
})

api.define("input.capture", {
  description = [[
Binds a role to the next key or button the player presses.

The capture spans frames: it waits for the player to let go of what is already
down, turns capture on, and takes the first press after that. The previous
capture state is restored when it lands or when the capture is cancelled.

Use this instead of `trx.input.listen` and `trx.input.bind_pressed`, which only
answer for the frame they run on. The default layout is read-only.

Use `trx.input.Binding` to choose a slot, input source, or layout without
placeholder nils.]],
  params = {
    { name = "role", type = "input.Role", description = "The role to bind." },
    {
      name = "opts",
      type = { "input.Slot", "input.Binding" },
      description = "Binding to write. Defaults to the first slot on the current source and layout.",
      optional = true,
    },
    {
      name = "done",
      type = "function",
      optional = true,
      description = "Called when the capture ends.",
      params = {
        {
          name = "bound",
          type = "boolean",
          description = "Whether a key was taken.",
        },
      },
    },
  },
  returns = {
    type = "input.Capture",
    description = "The running capture.",
  },
  impl = function(role, opts_or_slot, done)
    local slot, backend, layout = binding_args(opts_or_slot)
    backend = backend or raw.backend()
    layout = layout or raw.layout(backend)
    -- Both raise from bind_pressed too, but a capture reads it a frame later,
    -- where the error would reach the script from a tick instead of from here.
    if layout == Layout.DEFAULT then
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
  end,
})

api.define("input.unbind", {
  description = [[
Clears one role binding.

The default layout is read-only, and roles reserved by the game cannot be left
unbound.

Use `trx.input.Binding` to choose a slot, input source, or layout without
placeholder nils. Positional arguments still work.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to unbind.",
    },
    {
      name = "opts",
      type = { "input.Slot", "input.Binding" },
      description = "Binding to clear. Defaults to the first slot on the current source and layout.",
      optional = true,
    },
    {
      name = "backend",
      type = "input.Backend",
      description = "The input source to write. Defaults to the current one.",
      optional = true,
    },
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to write. Defaults to the current one.",
      optional = true,
    },
  },
  impl = function(role, opts_or_slot, backend, layout)
    return raw.unbind(role, binding_args(opts_or_slot, backend, layout))
  end,
})

api.define("input.reset_layout", {
  description = [[
Restores a custom layout to the default bindings.

Use `trx.input.Binding` to choose an input source or layout without placeholder
nils. Positional arguments still work.]],
  params = {
    {
      name = "opts",
      type = { "input.Backend", "input.Binding" },
      description = "Layout to reset. Defaults to the current source and layout.",
      optional = true,
    },
    {
      name = "layout",
      type = "input.Layout",
      description = "The layout to write. Defaults to the current one.",
      optional = true,
    },
  },
  impl = function(opts_or_backend, layout)
    return raw.reset_layout(layout_args(opts_or_backend, layout))
  end,
})

-------------------------------------------------------------------------------
-- the same answers as signals
-------------------------------------------------------------------------------

api.namespace("input.signals", {
  description = [[
Input roles as signals.

Each role has one shared signal, so several consumers of the same role use one
read per tick.]],
})

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

api.define("input.signals.held", {
  description = [[
A signal for whether a role is active.

It is true while the player holds the bound key or button. It changes when the
role becomes active and when it stops.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to follow.",
    },
  },
  returns = { type = "signal.Signal", description = "The role's signal." },
  impl = function(role)
    return shared(held, role, raw.is_held)
  end,
})

api.define("input.signals.pressed", {
  description = [[
A signal for when a role becomes active.

It is true for one tick only, so listeners run once per press.]],
  params = {
    {
      name = "role",
      type = "input.Role",
      description = "The role to follow.",
    },
  },
  returns = { type = "signal.Signal", description = "The role's signal." },
  impl = function(role)
    return shared(pressed, role, raw.is_pressed)
  end,
})
