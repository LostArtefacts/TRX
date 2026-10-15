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
local value = require("common.ui.value")
local tab_switch = require("common.ui.widgets.tab_switch")
local window = require("common.ui.widgets.window")

local M = {}

-- Room kept for text that measures a little wider than it draws.
local MEASURE_SLACK = 1.08

local TAB_BAR_SPACING = 8
local FOOTER_SPACING = 5
local MIN_VISIBLE_ROWS = 5
local MAX_VISIBLE_ROWS = 16

local Role = trx.input.Role

-- The height of everything but the rows, in canvas units at the default text
-- size.
local function chrome_height(line)
  return window.chrome_height({ title = true, hints = true })
    - metrics.TEXT_HEIGHT
    + line
    + line
    + TAB_BAR_SPACING
    + FOOTER_SPACING
    + line
end

local function visible_rows()
  local line = metrics.line_height()
  local room = trx.ui.safe_area.height / trx.ui.text_scale
  local rows = math.floor((room - chrome_height(line)) / line)
  return math.max(MIN_VISIBLE_ROWS, math.min(rows, MAX_VISIBLE_ROWS))
end

-- The size of the dialog: how wide its rows are, how many it shows, and
-- whether the tab bar shows the active tab alone.
local function lay_out(tabs, titles)
  local rows = visible_rows()
  local max_w, min_w, height = 0.0, 0.0, 0
  for _, tab in ipairs(tabs) do
    local content, least = tab:widths()
    max_w = math.max(max_w, content)
    min_w = math.max(min_w, least)
    height = math.max(height, tab:fit(rows))
  end
  local available = (
    trx.ui.safe_area.width / trx.ui.text_scale - window.chrome_width()
  ) / MEASURE_SLACK
  local full, single = tab_switch.widths(titles)
  local collapsed = full > math.max(min_w, available)
  local bar = collapsed and single or full
  max_w = math.max(max_w, bar)
  min_w = math.max(min_w, bar)
  return {
    width = math.max(min_w, math.min(max_w, available)),
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
---@field title string The name on the tab bar.
---@field scroll common.ui.Scroll The tab's rows.
---@field widget trx.ui.Widget The rows.
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
    elseif
      keys:pressed(Role.MENU_UP) and trx.config.get("ui.enable_wraparound")
    then
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
    return false
  end
  return keys:pressed(Role.MENU_BACK)
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
---@field title string The heading.
---@field tabs fun(dialog: common.ui.settings.Dialog): common.ui.settings.Tab[] Makes the tabs, which read the dialog's signals.

-- Creates a dialog.
---@param settings common.ui.settings.Dialog.settings
---@return common.ui.settings.Dialog
function M.new(settings)
  local self = setmetatable({
    editing = trx.signal.new(false),
    width = trx.signal.new(0),
    dialog_width = trx.signal.new(0),
    changed = trx.signal.new(trx.config.signals.changed:get()),
    layout = trx.signal.new(trx.ui.signals.layout:get()),
  }, Dialog)
  -- The dialog follows the settings and the room it lays out in through
  -- signals of its own, and stops following the engine's as it closes, so
  -- that nothing it built goes on listening once it has gone.
  self.listeners = {
    trx.config.signals.changed:on(function(count)
      self.changed:set(count)
    end),
    trx.ui.signals.layout:on(function(count)
      self.layout:set(count)
    end),
  }
  self.tabs = settings.tabs(self)

  local titles = {}
  for i, tab in ipairs(self.tabs) do
    titles[i] = tab.title
  end
  self.bar = tab_switch.new({
    titles = titles,
    mode = tab_switch.Mode.FOCUSED,
    wraps = self.changed:map(function()
      return trx.config.get("ui.enable_wraparound")
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

  local contents, footers = {}, {}
  for i, tab in ipairs(self.tabs) do
    local is_active = self.active:map(function(active)
      return active == i
    end)
    contents[i] = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.SPAN,
      shown = is_active,
      children = { tab.widget },
    })
    footers[i] = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.SPAN,
      shown = is_active,
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
          title = settings.title,
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
          h = metrics.TEXT_HEIGHT,
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
