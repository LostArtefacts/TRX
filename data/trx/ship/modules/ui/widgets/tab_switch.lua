-- A row of tab names, one of them active, with the arrows that say which way
-- the player can switch. The bar can show the active tab alone, for a dialog
-- too narrow for every name.
--
-- Usage:
--   local tab_switch = require("common.ui.widgets.tab_switch")
--   local tabs = tab_switch.new({ titles = { "Volume", "Misc" } })
--   tabs:control(keys)

local metrics = require("common.ui.metrics")
local row_arrows = require("common.ui.widgets.row_arrows")

local M = {}

local TAB_SPACING = 10
local TAB_PAD_X = 2
local TAB_PAD_Y = 1

-- How the bar draws the active tab.
---@enum common.ui.TabSwitch.Mode
M.Mode = {
  -- Outlined, with no arrows: the player is elsewhere in the dialog.
  IDLE = 0,
  -- Outlined, with arrows: the tab keys switch tabs from elsewhere.
  ARROWS = 1,
  -- Framed as the option under the cursor: the player is on the bar.
  FOCUSED = 2,
}

local function wraps()
  return trx.config.get("ui.enable_wraparound")
end

---@class common.ui.TabSwitch
---@field widget trx.ui.Widget The bar.
---@field active trx.signal.Signal The active tab, counted from 1.
---@field mode trx.signal.Signal How the bar draws, one of `common.ui.widgets.tab_switch.Mode`.
---@field single trx.signal.Signal Whether the bar shows the active tab alone.
---@field count integer How many tabs there are.
---@field private wraps trx.signal.Signal
local TabSwitch = {}
TabSwitch.__index = TabSwitch

-- Makes the tab one step to either side active. Returns false where the
-- active tab is at that end and the setting does not wrap.
---@param dir integer `-1` for the left, `1` for the right.
---@return boolean
function TabSwitch:cycle(dir)
  local target = self.active:get() + dir
  if self.count == 0 then
    return false
  elseif target < 1 or target > self.count then
    if not wraps() then
      return false
    end
    target = target < 1 and self.count or 1
  end
  self.active:set(target)
  return true
end

-- Switches tabs for the keys pressed. With `arrows` false, the left and the
-- right keys are left for the content. Returns whether the tab changed.
---@param keys trx.ui.LayerKeys
---@param arrows? boolean Whether left and right switch tabs. `true` by default.
---@return boolean
function TabSwitch:control(keys, arrows)
  local Role = trx.input.Role
  if
    (arrows ~= false and keys:pressed(Role.MENU_LEFT))
    or keys:pressed(Role.MENU_TAB_LEFT)
    or keys:pressed(Role.STEP_LEFT)
  then
    return self:cycle(-1)
  elseif
    (arrows ~= false and keys:pressed(Role.MENU_RIGHT))
    or keys:pressed(Role.MENU_TAB_RIGHT)
    or keys:pressed(Role.STEP_RIGHT)
  then
    return self:cycle(1)
  end
  return false
end

local function tab(self, index, title)
  local style = trx.signal.combine(
    self.active,
    self.mode,
    function(active, mode)
      if active ~= index then
        return false
      end
      return mode == M.Mode.FOCUSED and trx.ui.FrameStyle.SELECTED
        or trx.ui.FrameStyle.OUTLINE
    end
  )
  local shown = trx.signal.combine(
    self.active,
    self.single,
    function(active, single)
      return not single or active == index
    end
  )
  return trx.ui.widgets.Stack({
    v_align = trx.ui.VAlign.CENTER,
    visible = shown,
    children = {
      trx.ui.widgets.Frame({
        style = style,
        child = trx.ui.widgets.Pad({
          x = TAB_PAD_X,
          y = TAB_PAD_Y,
          child = trx.ui.widgets.Label({ text = title }),
        }),
      }),
    },
  })
end

-- How wide a bar of these tabs is with every tab, and with the widest tab
-- alone, in canvas units at the default text size. Both count the arrows.
---@param titles string[]
---@return number full
---@return number single
function M.widths(titles)
  local arrows = metrics.text_width("\\{button left}")
    + metrics.text_width("\\{button right}")
    + 2 * row_arrows.Spacing.MEDIUM
  local full, single = 0.0, 0.0
  for i, title in ipairs(titles) do
    local w = metrics.text_width(title) + 2 * TAB_PAD_X
    full = full + w + (i > 1 and TAB_SPACING or 0)
    single = math.max(single, w)
  end
  return full + arrows, single + arrows
end

---@class common.ui.TabSwitch.settings
---@field titles any[] The tab names, or signals carrying them, in order.
---@field active? integer The tab active at first. `1` by default.
---@field mode? common.ui.TabSwitch.Mode How the bar draws at first. `IDLE` by default.
---@field wraps? trx.signal.Signal Whether the tabs wrap around, which decides the arrows. The `ui.enable_wraparound` setting by default.
---@field single? boolean Whether the bar shows the active tab alone at first.

-- Creates a tab bar.
---@param settings common.ui.TabSwitch.settings
---@return common.ui.TabSwitch
function M.new(settings)
  local self = setmetatable({
    active = trx.signal.new(settings.active or 1),
    mode = trx.signal.new(settings.mode or M.Mode.IDLE),
    single = trx.signal.new(settings.single == true),
    wraps = settings.wraps or trx.signal.config("ui.enable_wraparound"),
    count = #settings.titles,
  }, TabSwitch)

  local tabs = {}
  for i, title in ipairs(settings.titles) do
    tabs[i] = tab(self, i, title)
  end

  local function arrow(towards_end)
    return trx.signal.combine(
      self.active,
      self.mode,
      self.wraps,
      function(active, mode, wrap)
        if mode == M.Mode.IDLE or self.count == 0 then
          return false
        end
        if towards_end then
          return wrap or active < self.count
        end
        return wrap or active > 1
      end
    )
  end

  self.widget = trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    children = {
      row_arrows.new({
        left = arrow(false),
        right = arrow(true),
        child = trx.ui.widgets.Stack({
          orientation = trx.ui.Orientation.HORIZONTAL,
          align = trx.ui.HAlign.CENTER,
          v_align = trx.ui.VAlign.CENTER,
          spacing = TAB_SPACING,
          children = tabs,
        }),
      }),
    },
  })
  return self
end

return M
