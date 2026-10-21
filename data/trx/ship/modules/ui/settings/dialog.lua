-- A settings dialog: a window with a bar of tabs, the rows of the active tab
-- under it, and the hints of that tab under the window. The cursor is either
-- on the tab bar, where left and right switch tabs, or on the rows, where they
-- change the setting under it.
--
-- Usage:
--   local dialog = require("common.ui.settings.dialog")
--   local settings_dialog = dialog.new({
--     title = "Sound",
--     tabs = function(self) return { editor.new({ ... }) } end,
--   })
--   ctx:push({ root = settings_dialog.widget, place = dialog.PLACE })

local metrics = require("common.ui.metrics")
local scroll = require("common.ui.scroll")
local editor = require("common.ui.settings.editor")
local value = require("common.ui.value")
local tab_switch = require("common.ui.widgets.tab_switch")
local window = require("common.ui.widgets.window")

local M = {}

-- Room kept for text that measures a little wider than it draws.
local MEASURE_SLACK = 1.08

local TAB_BAR_SPACING = 8
local FOOTER_SPACING = 5
local MIN_VISIBLE_ROWS = 5

local Role = trx.input.Role

-- The height of everything but the rows, in canvas units at the default text
-- size.
local function chrome_height(line)
  return window.chrome_height({ title = true, hints = true })
    - metrics.line_height()
    + line
    + line
    + TAB_BAR_SPACING
    + FOOTER_SPACING
    + line
end

-- How many rows show in a dialog of the given width, in canvas units at the
-- default text size. The rows fit the room that the dialog is placed in, such
-- as the room under the logo of the title screen, and leave a line of text
-- free under the dialog, so that it stays clear of the caption at the foot of
-- the screen.
local function visible_rows(width)
  local line = metrics.line_height()
  local area =
    trx.ui.safe_area_for((width + window.chrome_width()) * trx.ui.text_scale)
  local room = area.height / trx.ui.text_scale - line / (1 - M.PLACE.y)
  local rows = math.floor((room - chrome_height(line)) / line)
  return math.max(MIN_VISIBLE_ROWS, math.min(rows, metrics.MAX_DIALOG_ROWS))
end

-- The size of the dialog: how wide its rows are, how many it shows, and
-- whether the tab bar shows the active tab alone.
local function lay_out(tabs, titles)
  local max_w, min_w = 0.0, 0.0
  for _, tab in ipairs(tabs) do
    local content, least = tab:widths()
    max_w = math.max(max_w, content)
    min_w = math.max(min_w, least)
  end
  local available = (
    trx.ui.safe_area.width / trx.ui.text_scale - window.chrome_width()
  ) / MEASURE_SLACK
  local names = {}
  for i, title in ipairs(titles) do
    names[i] = value.read(title)
  end
  local full, single = tab_switch.widths(names)
  local collapsed = full > math.max(min_w, available)
  local bar = collapsed and single or full
  max_w = math.max(max_w, bar)
  min_w = math.max(min_w, bar)
  local width = math.max(min_w, math.min(max_w, available))
  local rows = visible_rows(width)
  local height = 0
  for _, tab in ipairs(tabs) do
    height = math.max(height, tab:fit(rows))
  end
  return {
    width = width,
    height = height,
    collapsed = collapsed,
  }
end

-- What the window reads of the active tab's list for its scroll arrows.
local function active_scroll(active, tabs)
  local function each(read)
    local signals = {}
    for i, tab in ipairs(tabs) do
      signals[i] = read(tab.scroll)
    end
    return value.pick(active, signals)
  end
  return { --[[@as common.ui.ScrollHints]]
    visible = each(function(s)
      return s.visible
    end),
    count = each(function(s)
      return s.count
    end),
    more_above = function()
      return each(function(s)
        return s:more_above()
      end)
    end,
    more_below = function()
      return each(function(s)
        return s:more_below()
      end)
    end,
  }
end

---@class common.ui.settings.Tab
---@field title string|trx.signal.Signal The name on the tab bar, or a signal that holds it.
---@field scroll common.ui.Scroll The tab's rows.
---@field widget? trx.ui.Widget What the tab shows. An editor tab has none of its own and shows the rows the dialog shares.
---@field footer trx.ui.Widget The hints under the dialog.
---@field widths fun(self: common.ui.settings.Tab): number, number
---@field control fun(self: common.ui.settings.Tab, keys: trx.ui.LayerKeys): boolean, boolean
---@field close fun(self: common.ui.settings.Tab)
---@field fit fun(self: common.ui.settings.Tab, rows: integer): number Shows what fits in that many rows of settings, and returns the height it takes.

---@class common.ui.settings.Dialog
---@field active trx.signal.Signal The active tab, counted from 1.
---@field editing trx.signal.Signal Whether the cursor is on the rows.
---@field width trx.signal.Signal How wide the rows are, in canvas units at the default text size.
---@field dialog_width trx.signal.Signal How wide the dialog is, in the same units.
---@field changed trx.signal.Signal Says when any setting changes, for as long as the dialog is open.
---@field layout trx.signal.Signal Says when the room the interface lays out in changes, for as long as the dialog is open.
---@field tabs common.ui.settings.Tab[]
---@field widget trx.ui.Widget The dialog.
---@field private bar common.ui.TabSwitch
---@field private listeners trx.signal.Listener[]
local Dialog = {}
Dialog.__index = Dialog

function Dialog:enter(last)
  local tab = self.tabs[self.active:get()] --[[@as common.ui.settings.Tab]]
  self.editing:set(true)
  tab.scroll:select(last and tab.scroll.count:get() or 1)
end

-- Reads the keys. Returns true once the player backs out of the dialog.
---@param keys trx.ui.LayerKeys
---@return boolean
function Dialog:control(keys)
  local tab = self.tabs[self.active:get()] --[[@as common.ui.settings.Tab]]
  if not self.editing:get() then
    if self.bar:control(keys) then
      return false
    end
    if
      keys:pressed(Role.MENU_DOWN) and not keys:pressed(Role.MENU_CONFIRM)
    then
      self:enter(false)
    elseif keys:pressed(Role.MENU_UP) and scroll.wraps() then
      self:enter(true)
    elseif keys:pressed(Role.MENU_BACK) then
      return true
    end
    return false
  end

  local consumed, leave = tab:control(keys)
  if leave then
    self.editing:set(false)
    tab.scroll:select(nil)
    return false
  elseif consumed then
    return false
  end
  if
    not keys:held(Role.MENU_LEFT)
    and not keys:held(Role.MENU_RIGHT)
    and self.bar:control(keys, false)
  then
    local now = self.tabs[self.active:get()] --[[@as common.ui.settings.Tab]]
    if now ~= tab then
      tab.scroll:select(nil)
      now.scroll:select(1)
    end
    return false
  end
  return keys:pressed(Role.MENU_BACK)
end

-- Where the player was in the dialog: the active tab, whether the cursor is
-- on the rows, and the row under it and the first row in view on each tab.
---@class common.ui.settings.Dialog.Place
---@field active integer
---@field editing boolean
---@field tabs { first: integer, selected: integer? }[]

---@return common.ui.settings.Dialog.Place
function Dialog:remember()
  local tabs = {}
  for i, tab in ipairs(self.tabs) do
    tabs[i] = {
      first = tab.scroll.first:get(),
      selected = tab.scroll.selected:get(),
    }
  end
  return {
    active = self.active:get(),
    editing = self.editing:get(),
    tabs = tabs,
  }
end

-- Puts the player back where `Dialog:remember` found them.
---@param place common.ui.settings.Dialog.Place
function Dialog:restore(place)
  for i, tab in ipairs(self.tabs) do
    local at = place.tabs[i]
    if at ~= nil then
      tab.scroll.first:set(at.first)
      tab.scroll:select(at.selected)
    end
  end
  self.active:set(math.min(place.active, #self.tabs))
  self.editing:set(place.editing)
end

-- Stops following the settings, once the dialog has closed.
function Dialog:close()
  for _, listener in ipairs(self.listeners) do
    listener:detach()
  end
  for _, tab in ipairs(self.tabs) do
    tab:close()
  end
end

---@class common.ui.settings.Dialog.settings
---@field title string|trx.signal.Signal|fun(): string The heading, a signal that holds it, or a function that reads it again whenever a setting changes, so that it follows the player's language.
---@field tabs fun(dialog: common.ui.settings.Dialog): common.ui.settings.Tab[] Makes the tabs, which read the dialog's signals.

-- Creates a dialog.
---@param settings common.ui.settings.Dialog.settings
---@return common.ui.settings.Dialog
function M.new(settings)
  -- The dialog follows the settings and the room it lays out in through
  -- signals of its own, and stops following the engine's as it closes, so
  -- that nothing it built goes on listening once it has gone.
  local changed, follow_changed = value.mirror(trx.config.signals.changed)
  local layout, follow_layout = value.mirror(trx.ui.signals.layout)
  local self = setmetatable({
    editing = trx.signal.new(false),
    width = trx.signal.new(0),
    dialog_width = trx.signal.new(0),
    changed = changed,
    layout = layout,
    listeners = { follow_changed, follow_layout },
  }, Dialog)
  self.tabs = settings.tabs(self)

  local titles = {}
  for i, tab in ipairs(self.tabs) do
    titles[i] = tab.title
  end
  self.bar = tab_switch.new({
    titles = titles,
    mode = tab_switch.Mode.FOCUSED,
    wraps = self.changed:map(function()
      return scroll.wraps()
    end),
  })
  self.active = self.bar.active
  self.listeners[#self.listeners + 1] = self.editing:on(function(editing)
    self.bar.mode:set(
      editing and tab_switch.Mode.ARROWS or tab_switch.Mode.FOCUSED
    )
  end)

  local size = trx.signal.combine(self.layout, self.changed, function()
    return lay_out(self.tabs, titles)
  end)
  local function apply(layout)
    self.width:set(layout.width)
    self.dialog_width:set(layout.width + window.chrome_width())
    self.bar.single:set(layout.collapsed)
  end
  apply(size:get())
  self.listeners[#self.listeners + 1] = size:on(apply)

  -- The editor tabs share one set of rows. A tab of another kind brings its
  -- own widget.
  local contents = {
    trx.ui.widgets.Stack({
      align = trx.ui.HAlign.SPAN,
      visible = self.active:map(function(active)
        return self.tabs[active].widget == nil
      end),
      children = {
        editor.rows({
          tabs = self.tabs,
          active = self.active,
          editing = self.editing,
          width = self.width,
          changed = self.changed,
        }),
      },
    }),
  }
  local footers = {}
  for i, tab in ipairs(self.tabs) do
    local is_active = self.active:map(function(active)
      return active == i
    end)
    if tab.widget ~= nil then
      contents[#contents + 1] = trx.ui.widgets.Stack({
        align = trx.ui.HAlign.SPAN,
        visible = is_active,
        children = { tab.widget },
      })
    end
    footers[i] = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.SPAN,
      visible = is_active,
      children = { tab.footer },
    })
  end

  -- The content keeps one size whichever tab shows, as wide and as tall as
  -- the largest tab needs.
  local content = trx.ui.widgets.Resize({
    scaled = true,
    w = self.width,
    h = size:map(function(layout)
      return layout.height
    end),
    child = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.SPAN,
      children = contents,
    }),
  })

  self.widget = trx.ui.widgets.Fit({
    child = trx.ui.widgets.Stack({
      spacing = FOOTER_SPACING,
      align = trx.ui.HAlign.SPAN,
      children = {
        window.new({
          title = type(settings.title) == "function" and self.changed:map(
            settings.title
          ) or settings.title,
          scroll = active_scroll(self.active, self.tabs),
          reserve_scroll = true,
          header = trx.ui.widgets.Stack({
            align = trx.ui.HAlign.SPAN,
            children = {
              self.bar.widget,
              trx.ui.widgets.Spacer({ h = TAB_BAR_SPACING }),
            },
          }),
          content = content,
        }),
        trx.ui.widgets.Resize({
          scaled = true,
          w = self.dialog_width,
          h = metrics.line_height(),
          child = trx.ui.widgets.Stack({
            align = trx.ui.HAlign.SPAN,
            children = footers,
          }),
        }),
      },
    }),
  })
  return self
end

-- Where the dialog goes: centered across the screen, and a little below the
-- middle.
M.PLACE = { y = 0.6 }

return M
