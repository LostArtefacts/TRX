-- The controls dialog of the inventory ring: a list of the input sources to
-- pick from, and an editor of the bindings of the one picked. The editor has a
-- bar of layouts, a bar of groups of roles, and a row for each role of the
-- group, with its two bindings and its name. The player binds a key by picking
-- a binding and pressing the key.
--
-- Usage:
--   local controls = require("common.ui.settings.controls")
--   local layer, close = controls.open({ push = push, finish = finish })

local accent = require("common.ui.accent")
local ammo = require("common.ui.ammo")
local metrics = require("common.ui.metrics")
local scroll = require("common.ui.scroll")
local timer = require("common.timer")
local value = require("common.ui.value")
local hold_button = require("common.ui.widgets.hold_button")
local requester = require("common.ui.widgets.requester")
local tab_switch = require("common.ui.widgets.tab_switch")
local window = require("common.ui.widgets.window")

local M = {}

local Role = trx.input.Role
local Backend = trx.input.Backend
local Layout = trx.input.Layout

local MIN_KEY_WIDTH = 80
local KEY_PAD = 6
local BAR_SPACING = 4
local HEADER_SPACING = 5
local FOOTER_SPACING = 5
local BUTTON_SPACING = 40
local MEASURE_SLACK = 1.05
local MIN_VISIBLE_ROWS = 3
local MAX_VISIBLE_ROWS = 15
local BLINK_PERIOD = 10
local SLOTS = 2

local LAYOUTS =
  { Layout.DEFAULT, Layout.CUSTOM_1, Layout.CUSTOM_2, Layout.CUSTOM_3 }

-- Where the dialogs go: the list of sources lower than the editor, as in the
-- original games.
local CHOOSER_PLACE = { y = 2 / 3 }
local EDITOR_PLACE = { y = 0.55 }

-- The input sources, each with its name, its spelling in the settings, and
-- the setting that holds its layout. The names are read as the dialog opens,
-- so that they follow the player's language.
local BACKENDS = {
  {
    backend = Backend.KEYBOARD,
    name = function()
      return trx.locale.get("general/settings/controls/backend/keyboard")
    end,
    setting = "keyboard",
    layout = "input.keyboard_layout",
  },
  {
    backend = Backend.CONTROLLER,
    name = function()
      return trx.locale.get("general/settings/controls/backend/controller")
    end,
    setting = "controller",
    layout = "input.controller_layout",
  },
  {
    backend = Backend.TOUCH,
    name = function()
      return trx.locale.get("general/settings/controls/backend/touch")
    end,
    setting = "touch",
    layout = "input.touch_layout",
    touch = true,
  },
}

-- The roles the player can bind, in the groups that the editor shows them in,
-- each group with its name.
local GROUPS = {
  {
    title = function()
      return trx.locale.get("general/settings/controls/tabs/basics")
    end,
    roles = {
      Role.FORWARD,
      Role.BACK,
      Role.LEFT,
      Role.RIGHT,
      Role.JUMP,
      Role.STEP_LEFT,
      Role.STEP_RIGHT,
      Role.ROLL,
      Role.SLOW,
      Role.SPRINT,
      Role.CROUCH,
      Role.ACTION,
      Role.DRAW,
      Role.LOOK,
    },
  },
  {
    title = function()
      return trx.locale.get("general/settings/controls/tabs/items")
    end,
    roles = {
      Role.USE_FLARE,
      Role.USE_SMALL_MEDI,
      Role.USE_BIG_MEDI,
      Role.EQUIP_PISTOLS,
      Role.EQUIP_SHOTGUN,
      Role.EQUIP_MAGNUMS,
      Role.EQUIP_AUTOS,
      Role.EQUIP_DESERT_EAGLE,
      Role.EQUIP_UZIS,
      Role.EQUIP_HARPOON,
      Role.EQUIP_M16,
      Role.EQUIP_MP5,
      Role.EQUIP_ROCKET_LAUNCHER,
      Role.EQUIP_GRENADE_LAUNCHER,
      Role.EQUIP_REVOLVER,
      Role.EQUIP_CROSSBOW,
      Role.CYCLE_AMMO,
    },
  },
  {
    title = function()
      return trx.locale.get("general/settings/controls/tabs/misc")
    end,
    roles = {
      Role.CHANGE_TARGET,
      Role.CAMERA_UP,
      Role.CAMERA_DOWN,
      Role.CAMERA_LEFT,
      Role.CAMERA_RIGHT,
      Role.CAMERA_FORWARD,
      Role.CAMERA_BACK,
      Role.USE_BINOCULARS,
      Role.CHANGE_OUTFIT,
      Role.FLY_CHEAT,
      Role.ITEM_CHEAT,
      Role.LEVEL_SKIP_CHEAT,
      Role.TURBO_CHEAT,
      Role.FAST_FORWARD_CHEAT,
      Role.SLOW_MOTION_CHEAT,
    },
  },
  {
    title = function()
      return trx.locale.get("general/settings/controls/tabs/system")
    end,
    roles = {
      Role.OPTION,
      Role.SAVE,
      Role.LOAD,
      Role.QUICK_SAVE,
      Role.QUICK_LOAD,
      Role.PAUSE,
      Role.TOGGLE_FPS_COUNTER,
      Role.TOGGLE_FULLSCREEN,
      Role.ENTER_CONSOLE,
      Role.TOGGLE_PHOTO_MODE,
      Role.TOGGLE_UI,
      Role.TOGGLE_BILINEAR_FILTER,
      Role.TOGGLE_TRAPEZOID_FILTER,
      Role.SWITCH_UPSCALING,
      Role.SWITCH_BORDERS,
      Role.TOGGLE_WIREFRAME,
      Role.TOGGLE_TEXTURES,
      Role.CYCLE_LIGHTING_MODEL,
    },
  },
}

local CHEATS = {
  [Role.FLY_CHEAT] = true,
  [Role.ITEM_CHEAT] = true,
  [Role.LEVEL_SKIP_CHEAT] = true,
  [Role.TURBO_CHEAT] = true,
  [Role.FAST_FORWARD_CHEAT] = true,
  [Role.SLOW_MOTION_CHEAT] = true,
}

local WEAPONS = {
  [Role.USE_FLARE] = true,
  [Role.EQUIP_PISTOLS] = true,
  [Role.EQUIP_SHOTGUN] = true,
  [Role.EQUIP_MAGNUMS] = true,
  [Role.EQUIP_AUTOS] = true,
  [Role.EQUIP_DESERT_EAGLE] = true,
  [Role.EQUIP_UZIS] = true,
  [Role.EQUIP_HARPOON] = true,
  [Role.EQUIP_M16] = true,
  [Role.EQUIP_MP5] = true,
  [Role.EQUIP_ROCKET_LAUNCHER] = true,
  [Role.EQUIP_GRENADE_LAUNCHER] = true,
  [Role.EQUIP_REVOLVER] = true,
  [Role.EQUIP_CROSSBOW] = true,
}

local width_of = metrics.text_width

-- The input sources that the player can pick.
local function available_backends()
  local result = {}
  for _, entry in ipairs(BACKENDS) do
    if
      (not entry.touch or trx.config.get("input.enable_touch_controls"))
      and trx.input.is_backend_enabled(entry.backend)
    then
      result[#result + 1] = entry
    end
  end
  return result
end

-- The roles whose action the game does not offer, such as a weapon this game
-- does not have or a cheat while cheats are off. Their rows are dimmed.
local function unusable_roles()
  local result = {}
  for role in pairs(WEAPONS) do
    result[role] = true
  end
  result[Role.CYCLE_AMMO] = true
  for _, weapon in ipairs(trx.weapons.all) do
    local role = weapon.kind == trx.weapons.Kind.FLARE and Role.USE_FLARE
      or weapon.equip_key
    if role ~= nil then
      result[role] = not weapon.is_available
    end
    if weapon.is_available and ammo.is_choosable(weapon) then
      result[Role.CYCLE_AMMO] = false
    end
  end
  local cheats = trx.config.get("gameplay.enable_cheats")
  for role in pairs(CHEATS) do
    result[role] = not cheats
  end
  result[Role.USE_BINOCULARS] =
    not trx.config.get("gameplay.enable_binoculars")
  return result
end

local function dim(text, unusable)
  return unusable and ("\\{dim}%s\\{/dim}"):format(text) or text
end

local function framed(on)
  return on and trx.ui.FrameStyle.SELECTED or false
end

-------------------------------------------------------------------------------
-- the editor
-------------------------------------------------------------------------------

---@alias common.ui.controls.Focus "layout"|"group"|"inputs"

---@class common.ui.controls.Editor
---@field widget trx.ui.Widget The dialog.
---@field private backend trx.input.Backend
---@field private layouts common.ui.TabSwitch
---@field private groups common.ui.TabSwitch
---@field private scroll common.ui.Scroll
---@field private focus trx.signal.Signal
---@field private slot trx.signal.Signal
---@field private capturing trx.signal.Signal
---@field private capture? trx.input.Capture
---@field private settling boolean
---@field private reset common.ui.HoldButton
---@field private unbind common.ui.HoldButton
---@field private blinking common.timer.Timer
---@field private listeners trx.signal.Listener[]
local Editor = {}
Editor.__index = Editor

-- The layout on the bar, which the editor binds keys in.
---@return trx.input.Layout
function Editor:layout()
  return LAYOUTS[self.layouts.active:get()]
end

-- The role under the cursor, or nil.
---@return trx.input.Role?
function Editor:selected_role()
  local index = self.scroll.selected:get()
  if self.focus:get() ~= "inputs" or index == nil then
    return nil
  end
  return GROUPS[self.groups.active:get()].roles[index]
end

-- Starts reading the key for the binding under the cursor.
function Editor:listen(role)
  if not trx.input.is_rebindable(role) then
    return
  end
  self.capturing:set(true)
  self.capture = trx.input.capture(role, {
    slot = self.slot:get(),
    backend = self.backend,
    layout = self:layout(),
  }, function()
    self.capture = nil
    self.capturing:set(false)
    self.settling = true
    self:show_buttons()
  end)
end

-- Reads the hold buttons, and acts on a completed hold.
-- Which hold buttons the editor offers now: the one that resets the layout,
-- and the one that unbinds the binding under the cursor. It offers neither
-- while it reads a key, or on the default layout.
---@return boolean reset
---@return boolean unbind
function Editor:offered()
  local open = self:layout() ~= Layout.DEFAULT and self.capture == nil
  local role = self:selected_role()
  return open,
    open and role ~= nil and trx.input.can_unbind(role, {
      slot = self.slot:get(),
      backend = self.backend,
      layout = self:layout(),
    })
end

-- Shows the hold buttons that the editor offers now, on the same frame as
-- whatever changed them.
function Editor:show_buttons()
  local reset, unbind = self:offered()
  self.reset:set_enabled(reset)
  self.unbind:set_enabled(unbind)
end

function Editor:control_buttons(keys)
  local can_reset, can_unbind = self:offered()
  local sample = trx.game.tr_version == 1 and trx.catalog.samples.MENU_GAMEBOY
    or trx.catalog.samples.MENU_SPINOUT
  if self.reset:control(keys, can_reset) then
    trx.sound.play(sample)
    trx.input.reset_layout({ backend = self.backend, layout = self:layout() })
  end
  if self.unbind:control(keys, can_unbind) then
    local role = self:selected_role()
    trx.sound.play(sample)
    ---@type trx.input.Binding
    local binding = {
      slot = self.slot:get(),
      backend = self.backend,
      layout = self:layout(),
    }
    trx.input.unbind(role --[[@as trx.input.Role]], binding)
  end
end

-- Moves the cursor onto the rows, on the first or the last.
function Editor:enter(last)
  self.focus:set("inputs")
  self.scroll:select(last and self.scroll.count:get() or 1)
end

-- Reads the keys. Returns `"exit"` where the player closes the dialog, and
-- `"back"` where the player backs out to the list of sources.
---@param keys trx.ui.LayerKeys
---@return "exit"|"back"|nil
function Editor:control(keys)
  self:control_buttons(keys)
  local result = self:navigate(keys)
  self:show_buttons()
  return result
end

-- Moves the cursor and starts reading a key for the keys pressed.
---@param keys trx.ui.LayerKeys
---@return "exit"|"back"|nil
---@private
function Editor:navigate(keys)
  if self.capture ~= nil then
    return nil
  end
  -- The key that was bound stays down for a moment, and moves nothing.
  if self.settling then
    if trx.input.is_anything_held() then
      return nil
    end
    self.settling = false
  end

  local focus = self.focus:get()
  local custom = self:layout() ~= Layout.DEFAULT
  local wraps = scroll.wraps()
  if focus ~= "inputs" and keys:pressed(Role.MENU_CONFIRM) then
    return "exit"
  elseif keys:pressed(Role.MENU_BACK) then
    return "back"
  end

  if focus == "layout" then
    if self.layouts:control(keys) then
      return nil
    elseif keys:pressed(Role.MENU_DOWN) then
      self.focus:set("group")
    elseif keys:pressed(Role.MENU_UP) and custom and wraps then
      self:enter(true)
    end
  elseif focus == "group" then
    if self.groups:control(keys) then
      return nil
    elseif keys:pressed(Role.MENU_DOWN) and custom then
      self:enter(false)
    elseif keys:pressed(Role.MENU_UP) then
      self.focus:set("layout")
    end
  else
    local role = self:selected_role()
    if keys:pressed(Role.MENU_CONFIRM) then
      if role ~= nil then
        self:listen(role)
      end
    elseif
      not keys:held(Role.MENU_LEFT)
      and not keys:held(Role.MENU_RIGHT)
      and self.groups:control(keys, false)
    then
      return nil
    elseif keys:pressed(Role.MENU_LEFT) then
      self.slot:set(scroll.step(self.slot:get(), -1, SLOTS) or self.slot:get())
    elseif keys:pressed(Role.MENU_RIGHT) then
      self.slot:set(scroll.step(self.slot:get(), 1, SLOTS) or self.slot:get())
    elseif keys:pressed(Role.MENU_UP) then
      if not self.scroll:select_prev(false) then
        self.focus:set("group")
      end
    elseif keys:pressed(Role.MENU_DOWN) then
      if not self.scroll:select_next(false) and wraps then
        self.focus:set("layout")
      end
    end
  end
  return nil
end

-- Where the player was in the editor: the bar or the rows the cursor is on,
-- the group, the row under it and the first row in view, and the binding.
---@class common.ui.controls.Place
---@field focus common.ui.controls.Focus
---@field group integer
---@field first integer
---@field selected integer?
---@field slot integer

---@return common.ui.controls.Place
function Editor:remember()
  return {
    focus = self.focus:get(),
    group = self.groups.active:get(),
    first = self.scroll.first:get(),
    selected = self.scroll.selected:get(),
    slot = self.slot:get(),
  }
end

-- Puts the player back where `Editor:remember` found them. The rows of the
-- default layout take no cursor, so a cursor left on them goes to the bar of
-- groups.
---@param place common.ui.controls.Place
function Editor:restore(place)
  self.groups.active:set(place.group)
  self.scroll.first:set(place.first)
  self.scroll:select(place.selected)
  self.slot:set(place.slot)
  local focus = place.focus
  if focus == "inputs" and self:layout() == Layout.DEFAULT then
    focus = "group"
  end
  self.focus:set(focus)
  self:show_buttons()
end

-- Stops reading keys and following the settings, once the dialog has closed.
function Editor:close()
  if self.capture ~= nil then
    self.capture:cancel()
  end
  self.blinking:cancel()
  for _, listener in ipairs(self.listeners) do
    listener:detach()
  end
end

-- How many rows fit on the screen, within the bounds the originals keep.
local function visible_rows()
  local line = metrics.line_height()
  local around = window.chrome_height({ title = true, hints = true })
    + 2 * line
    + BAR_SPACING
    + HEADER_SPACING
    + line
    + FOOTER_SPACING
  local room = trx.ui.safe_area.height / trx.ui.text_scale
  local rows = math.floor((room - around) / line)
  return math.max(MIN_VISIBLE_ROWS, math.min(rows, MAX_VISIBLE_ROWS))
end

-- The size of the editor: how wide its columns are, how wide the window is,
-- how many rows it shows, and whether the bar of groups shows the active
-- group alone. The key column takes its width from the longest binding that
-- any layout holds, so that a combination such as Alt+Return does not run into
-- the column beside it.
local function lay_out(backend, titles, footer_w)
  local label_w, key_w = 0.0, MIN_KEY_WIDTH
  for _, group in ipairs(GROUPS) do
    for _, role in ipairs(group.roles) do
      label_w = math.max(label_w, width_of(trx.input.role_name(role)))
      for _, layout in ipairs(LAYOUTS) do
        for slot = 1, SLOTS do
          local name = trx.input.key_name(role, {
            slot = slot,
            backend = backend,
            layout = layout,
          })
          if name ~= nil then
            key_w = math.max(key_w, width_of(name) + KEY_PAD)
          end
        end
      end
    end
  end

  local _, layout_w = tab_switch.widths(titles.layouts)
  local groups_w, group_w = tab_switch.widths(titles.groups)
  local rest = math.max(SLOTS * key_w + label_w, footer_w, layout_w)
  local available = (
    trx.ui.safe_area.width / trx.ui.text_scale - window.chrome_width()
  ) / MEASURE_SLACK
  local collapsed = groups_w > math.max(rest, available)
  return {
    label = label_w,
    key = key_w,
    width = math.max(rest, collapsed and group_w or groups_w) * MEASURE_SLACK,
    rows = visible_rows(),
    collapsed = collapsed,
  }
end

-- A cell of a row: a text in a column of a given width, framed while the
-- cursor is on it.
local function cell(width, text, style, blank)
  return trx.ui.widgets.Resize({
    scaled = true,
    w = width,
    child = trx.ui.widgets.Stack({
      children = {
        trx.ui.widgets.Blank({
          when = blank,
          child = trx.ui.widgets.Frame({
            style = style,
            child = trx.ui.widgets.Label({ text = text }),
          }),
        }),
      },
    }),
  })
end

-- One of the rows on screen, showing whichever role sits at its place in the
-- active group as it scrolls. `view` holds what every row shows.
local function row_widget(index, view, size, blink)
  local row = view:map(function(rows)
    return rows[index]
  end)
  local function part(read)
    return row:map(function(r)
      return r and read(r) or false
    end)
  end
  local hidden = trx.signal.combine(row, blink, function(r, on)
    return r and r.conflicted and on or false
  end)
  local key_w = size:map(function(s)
    return s.key
  end)
  local children = {}
  for slot = 1, SLOTS do
    children[slot] = cell(
      key_w,
      part(function(r)
        return r.keys[slot]
      end),
      part(function(r)
        return framed(r.slot == slot)
      end),
      hidden
    )
  end
  children[SLOTS + 1] = cell(
    size:map(function(s)
      return s.label
    end),
    part(function(r)
      return accent.text(r.label)
    end),
    part(function(r)
      return framed(r.focused)
    end),
    false
  )
  return trx.ui.widgets.Blank({
    visible = size:map(function(s)
      return index <= s.rows
    end),
    when = row:map(function(r)
      return r == false
    end),
    child = trx.ui.widgets.Stack({
      orientation = trx.ui.Orientation.HORIZONTAL,
      v_align = trx.ui.VAlign.CENTER,
      children = children,
    }),
  })
end

---@class common.ui.controls.Editor.settings
---@field backend trx.input.Backend The input source whose bindings the editor changes.
---@field layout_setting string The setting that holds the layout of that source.

-- Creates an editor of the bindings of one input source.
---@param settings common.ui.controls.Editor.settings
---@return common.ui.controls.Editor
function M.editor(settings)
  local backend = settings.backend
  local changed, follow_changed = value.mirror(trx.config.signals.changed)
  local layout, follow_layout = value.mirror(trx.ui.signals.layout)
  local blink = trx.signal.new(false)

  local titles = { layouts = {}, groups = {} }
  for i, at in ipairs(LAYOUTS) do
    titles.layouts[i] = trx.input.layout_name(at)
  end
  for i, group in ipairs(GROUPS) do
    titles.groups[i] = group.title()
  end

  local current = trx.config.get(settings.layout_setting)
  local self = setmetatable({
    backend = backend,
    layouts = tab_switch.new({
      titles = titles.layouts,
      active = current + 1,
      mode = tab_switch.Mode.FOCUSED,
      single = true,
    }),
    groups = tab_switch.new({ titles = titles.groups }),
    scroll = scroll.new({ count = #GROUPS[1].roles }),
    focus = trx.signal.new("layout"),
    slot = trx.signal.new(1),
    capturing = trx.signal.new(false),
    settling = false,
    reset = hold_button.new({
      words = trx.locale.get("general/actions/reset_defaults"),
      role = Role.RESET_BINDINGS,
    }),
    unbind = hold_button.new({
      words = trx.locale.get("general/actions/unbind"),
      role = Role.UNBIND_KEY,
    }),
    blinking = timer.every(BLINK_PERIOD, function()
      blink:set(not blink:get())
    end, { clock = timer.Clock.UI }),
  }, Editor)

  local footer = trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    align = trx.ui.HAlign.DISTRIBUTE,
    spacing = BUTTON_SPACING,
    children = { self.reset.widget, self.unbind.widget },
  })

  local size = trx.signal.combine(layout, changed, function()
    local footer_w = footer:measure() / trx.ui.text_scale
    return lay_out(backend, titles, footer_w)
  end)

  self.listeners = {
    follow_changed,
    follow_layout,
    self.layouts.active:on(function(active)
      trx.config.set(settings.layout_setting, LAYOUTS[active])
    end),
    self.groups.active:on(function(active)
      self.scroll:set_count(#GROUPS[active].roles)
    end),
    size:on(function(s)
      self.scroll:set_visible(s.rows)
      self.groups.single:set(s.collapsed)
    end),
    self.focus:on(function(focus)
      self.layouts.mode:set(
        focus == "layout" and tab_switch.Mode.FOCUSED or tab_switch.Mode.IDLE
      )
    end),
  }
  self.scroll:set_visible(size:get().rows)
  self.groups.single:set(size:get().collapsed)

  local groups_mode = trx.signal.combine(
    self.focus,
    self.capturing,
    function(focus, capturing)
      if focus == "group" then
        return tab_switch.Mode.FOCUSED
      elseif focus == "inputs" and not capturing then
        return tab_switch.Mode.ARROWS
      end
      return tab_switch.Mode.IDLE
    end
  )
  self.groups.mode:set(groups_mode:get())
  self.listeners[#self.listeners + 1] = groups_mode:on(function(mode)
    self.groups.mode:set(mode)
  end)

  -- What every row on screen shows, worked out together as the cursor, the
  -- group, the layout or a binding moves.
  local view = trx.signal.combine(
    self.groups.active,
    self.layouts.active,
    self.scroll.first,
    self.scroll.selected,
    self.focus,
    self.slot,
    self.capturing,
    changed,
    function(group, layout_index, first, selected, focus, slot, capturing)
      local at = LAYOUTS[layout_index]
      local empty = at ~= Layout.DEFAULT and "—" or ""
      local unusable = unusable_roles()
      ---@type trx.input.Role[]
      local roles = GROUPS[group].roles
      local rows = {}
      for i = 1, MAX_VISIBLE_ROWS do
        local index = first + i - 1
        local role = roles[index]
        if role == nil then
          rows[i] = false
        else
          local on = focus == "inputs" and selected == index
          local keys = {}
          for s = 1, SLOTS do
            local name = trx.input.key_name(role, {
              slot = s,
              backend = backend,
              layout = at,
            })
            keys[s] = name ~= nil and dim(name, unusable[role]) or empty
          end
          rows[i] = {
            label = dim(trx.input.role_name(role), unusable[role]),
            keys = keys,
            focused = on and not capturing,
            slot = on and slot or nil,
            conflicted = trx.input.is_conflicted(role, {
              backend = backend,
              layout = at,
            }),
          }
        end
      end
      return rows
    end
  )

  local rows = {}
  for i = 1, MAX_VISIBLE_ROWS do
    rows[i] = row_widget(i, view, size, blink)
  end

  self.widget = trx.ui.widgets.Fit({
    child = trx.ui.widgets.Stack({
      spacing = FOOTER_SPACING,
      align = trx.ui.HAlign.SPAN,
      children = {
        window.new({
          title = accent.text(
            trx.locale.get("general/settings/controls/customize")
          ),
          scroll = self.scroll,
          reserve_scroll = true,
          header = trx.ui.widgets.Stack({
            spacing = HEADER_SPACING,
            align = trx.ui.HAlign.SPAN,
            children = {
              trx.ui.widgets.Stack({
                spacing = BAR_SPACING,
                align = trx.ui.HAlign.SPAN,
                children = { self.layouts.widget, self.groups.widget },
              }),
              trx.ui.widgets.Spacer({}),
            },
          }),
          content = trx.ui.widgets.Resize({
            scaled = true,
            w = size:map(function(s)
              return s.width
            end),
            child = trx.ui.widgets.Stack({ children = rows }),
          }),
        }),
        footer,
      },
    }),
  })
  self:show_buttons()
  return self
end

-------------------------------------------------------------------------------
-- the list of input sources
-------------------------------------------------------------------------------

-- Where the player left the dialog: the source whose editor was open, if any,
-- and where they were in the editor of each source.
local places = { editors = {} }

-- Forgets where the player left the dialog, so that it opens on the list of
-- sources again, as it does when a level starts.
function M.forget()
  places = { editors = {} }
end

---@class common.ui.controls.open.settings
---@field push fun(settings: trx.ui.layers.push.settings): trx.ui.StackLayer Opens a layer.
---@field finish fun(confirmed: boolean) Closes the dialog: `true` where the player confirmed out of the editor, `false` where they backed out.
---@field visible? trx.signal.Signal Whether the dialog shows. Always by default.
---@field centred? boolean Whether the dialog stands in the middle of the screen, rather than where it leaves room for the inventory ring.
---@field wrap? fun(control: fun(layer: trx.ui.StackLayer, keys: trx.ui.LayerKeys)): fun(layer: trx.ui.StackLayer, keys: trx.ui.LayerKeys) Wraps each input handler.

-- Opens the dialog: the list of input sources, and the editor of the source
-- picked over it. Where only one source is enabled, the editor opens alone,
-- and backing out of it closes the dialog. The dialog opens where the player
-- left it until `forget`. Returns the layer of the list, and
-- a function that closes whatever the dialog still has open.
---@param settings common.ui.controls.open.settings
---@return trx.ui.StackLayer
---@return fun()
function M.open(settings)
  local visible = settings.visible or trx.signal.new(true)
  local centred = { y = 0.5 }
  local chooser_place = settings.centred and centred or CHOOSER_PLACE
  local editor_place = settings.centred and centred or EDITOR_PLACE
  local wrap = settings.wrap or function(control)
    return control
  end
  local choosing = trx.signal.new(true)
  local available = available_backends()
  local req = requester.new({
    title = accent.text(trx.locale.get("general/settings/controls/customize")),
    place = chooser_place,
  })

  ---@type common.ui.controls.Editor?
  local editor = nil
  ---@type trx.ui.StackLayer?
  local editor_layer = nil

  local function show_backends()
    local rows = {}
    local selected = 1
    for i, entry in ipairs(available) do
      rows[i] = { text = entry.name() }
      if entry.backend == trx.input.backend then
        selected = i
      end
    end
    req:set_rows(rows)
    req.list:select(selected)
  end

  local function edit(entry)
    trx.config.set("input.backend", entry.setting)
    editor =
      M.editor({ backend = entry.backend, layout_setting = entry.layout })
    if places.editors[entry.backend] ~= nil then
      editor:restore(places.editors[entry.backend])
    end
    places.backend = entry.backend
    choosing:set(false)
    editor_layer = settings.push({
      root = trx.ui.widgets.Stack({
        visible = visible,
        children = { editor.widget },
      }),
      place = editor_place,
      on_input = wrap(function(layer, keys)
        local result = editor:control(keys)
        if result == "exit" then
          settings.finish(true)
        elseif result == "back" then
          if #available == 1 then
            settings.finish(false)
          else
            places.backend = nil
            layer:close()
          end
        end
      end),
      on_close = function()
        places.editors[entry.backend] = editor:remember()
        editor:close()
        editor = nil
        editor_layer = nil
        choosing:set(true)
      end,
    })
  end

  -- A source can be turned on or off while the dialog is open, from the
  -- console. The list follows, and a single source left opens its editor.
  local follow = trx.config.signals.changed:on(function()
    local now = available_backends()
    if #now == #available then
      return
    end
    available = now
    show_backends()
    if editor == nil and #available == 1 then
      edit(available[1])
    end
  end)

  show_backends()
  local layer = settings.push({
    root = trx.ui.widgets.Stack({
      visible = visible & choosing,
      children = { req.widget },
    }),
    place = req.place,
    on_input = wrap(function(_, keys)
      local picked, cancelled = req.list:control(keys)
      if cancelled then
        settings.finish(false)
      elseif picked ~= nil then
        edit(available[picked])
      end
    end),
  })
  -- The dialog opens again on the editor that the player left it on.
  if #available == 1 then
    edit(available[1])
  else
    for _, entry in ipairs(available) do
      if entry.backend == places.backend then
        edit(entry)
      end
    end
  end

  return layer,
    function()
      follow:detach()
      if editor_layer ~= nil then
        editor_layer:close()
      end
    end
end

return M
