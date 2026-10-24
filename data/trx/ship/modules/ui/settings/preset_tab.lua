-- The presets tab of the gameplay settings: a list of the presets, and the
-- question that a picked preset asks before it changes anything, with the
-- settings it would change.
--
-- Usage:
--   local preset_tab = require("common.ui.settings.preset_tab")
--   local tab = preset_tab.new({
--     title = "Presets",
--     editing = editing,
--     push = function(settings)
--       return ctx:push(settings)
--     end,
--   })

local accent = require("common.ui.accent")
local metrics = require("common.ui.metrics")
local scroll = require("common.ui.scroll")
local presets = require("common.ui.settings.presets")
local rows = require("common.ui.settings.rows")
local window = require("common.ui.widgets.window")

local M = {}

local LIST_ROW_SPACING = 5
local ROW_PAD = 4
local CONFIRM_PAD = 6
local CONFIRM_MIN_W = 72
local CONFIRM_ROW_SPACING = 2
local CONFIRM_MIN_ROWS = 4
local CONFIRM_MAX_ROWS = 10
local CHOICE_SPACING = 12
local VALUE_SPACING = 12
local CHOICE_PAD = 8

local Role = trx.input.Role

local width_of = metrics.text_width

local function wrap(text, width)
  return trx.ui.primitive.wrap_text(text, width * trx.ui.text_scale)
end

local function lines_of(text)
  local lines = {}
  for line in (text .. "\n"):gmatch("(.-)\n") do
    lines[#lines + 1] = line
  end
  return lines
end

local function label(text)
  return trx.ui.widgets.Label({ text = text })
end

local function centered(child)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    children = { child },
  })
end

-- Opens a dialog that says the preset was applied, or that it had nothing to
-- change. Confirm or back closes it.
local function show_message(push, title, message)
  local width = math.min(
    math.max(CONFIRM_MIN_W, width_of(title), width_of(message)),
    trx.ui.safe_area.width / trx.ui.text_scale
      - 4 * CONFIRM_PAD
      - window.chrome_width()
  )
  return push({
    root = trx.ui.widgets.Fit({
      child = trx.ui.widgets.Pad({
        x = CONFIRM_PAD,
        y = CONFIRM_PAD,
        child = window.new({
          title = accent.text(wrap(title, width)),
          heavy = true,
          content = trx.ui.widgets.Pad({
            x = CONFIRM_PAD,
            y = CONFIRM_PAD,
            child = label(wrap(message, width)),
          }),
        }),
      }),
    }),
    on_input = function(layer, keys)
      if keys:pressed(Role.MENU_CONFIRM) or keys:pressed(Role.MENU_BACK) then
        layer:close()
      end
    end,
  })
end

local function choice_widget(text, selected)
  return trx.ui.widgets.Frame({
    style = selected:map(function(on)
      return on and trx.ui.FrameStyle.SELECTED or false
    end),
    child = trx.ui.widgets.Pad({ x = CHOICE_PAD, child = label(text) }),
  })
end

-- Opens the question that a picked preset asks: the settings it would change,
-- each with the value it holds and the value it would take, and a choice to
-- apply the preset or go back.
local function confirm(push, preset, changes)
  local title = trx.locale.format(
    "general/config_presets/title_fmt",
    presets.title(preset)
  )
  local description =
    trx.locale.get("general/config_presets/confirm_description")
  local note = trx.locale.get("general/config_presets/confirm_restart_note")
  local apply_text = trx.locale.get("general/config_presets/confirm_apply")
  local back_text = trx.locale.get("general/config_presets/confirm_back")

  -- The dialog is as wide as its widest line, within what the screen allows.
  local natural = math.max(CONFIRM_MIN_W + 0.0, width_of(title))
  natural = math.max(
    natural,
    CHOICE_SPACING
      + width_of(apply_text)
      + width_of(back_text)
      + 4 * CHOICE_PAD
  )
  local budget = trx.ui.safe_area.width / trx.ui.text_scale
    - 2 * CONFIRM_PAD
    - window.chrome_width()

  -- Each setting shows its name with the change to its value on the right.
  -- Where any setting is too wide for that, every name stands over its value.
  local entries = {}
  local paired = 0.0
  for _, change in ipairs(changes) do
    local name = rows.title(change.key)
    local value = ("%s \\{button right} %s"):format(
      change.current,
      change.target
    )
    paired = math.max(paired, width_of(name) + VALUE_SPACING + width_of(value))
    entries[#entries + 1] = { name = name, value = value }
  end
  local side_by_side = paired + 1 <= budget
  if side_by_side then
    natural = math.max(natural, paired)
  else
    for _, entry in ipairs(entries) do
      entry.value = "  " .. entry.value
      natural = math.max(natural, width_of(entry.name), width_of(entry.value))
    end
  end
  local width = math.min(natural + 1, math.max(budget, CONFIRM_MIN_W))

  local wrapped_title = wrap(title, width)
  local wrapped_description = wrap(description, width)
  local wrapped_note = wrap(note, width)
  local line_height = metrics.line_height()
  local area = trx.ui.safe_area_for(
    (width + window.chrome_width() + 2 * CONFIRM_PAD) * trx.ui.text_scale
  )
  local room = area.height / trx.ui.text_scale
    - window.chrome_height({ title = true, hints = true })
    - 2 * CONFIRM_PAD
  local function rows_in(fixed_lines)
    return math.floor(
      (room - fixed_lines * line_height) / (line_height + CONFIRM_ROW_SPACING)
    )
  end

  -- Where the full dialog leaves too few rows, such as under the logo of the
  -- title screen, the description and the note scroll with the settings, and
  -- only the heading and the choice stay in place.
  local title_lines = #lines_of(wrapped_title)
  local visible = rows_in(
    title_lines
      + 1
      + #lines_of(wrapped_description)
      + 3
      + #lines_of(wrapped_note)
  )
  local compact = visible < CONFIRM_MIN_ROWS
  if compact then
    visible = rows_in(title_lines + 2)
  end
  visible = math.max(CONFIRM_MIN_ROWS, math.min(visible, CONFIRM_MAX_ROWS))

  -- The list scrolls a line at a time, so each wrapped line is a row, and so
  -- is a setting that shows its value beside it. An empty line is a gap of
  -- one line.
  local lines = {}
  local function add(text)
    for _, line in ipairs(lines_of(text)) do
      lines[#lines + 1] = line
    end
  end
  if compact then
    add(wrapped_description)
    add("")
  end
  for _, entry in ipairs(entries) do
    if side_by_side then
      lines[#lines + 1] = entry
    else
      add(wrap(entry.name, width))
      for _, line in ipairs(lines_of(wrap(entry.value, width))) do
        lines[#lines + 1] = accent.text(line)
      end
    end
  end
  if compact then
    add("")
    add(wrapped_note)
  end
  local list = scroll.new({ count = #lines, visible = visible })

  local function line_widget(line)
    if type(line) == "table" then
      return trx.ui.widgets.Stack({
        orientation = trx.ui.Orientation.HORIZONTAL,
        align = trx.ui.HAlign.DISTRIBUTE,
        spacing = VALUE_SPACING,
        children = { label(line.name), label(accent.text(line.value)) },
      })
    elseif line == "" then
      return trx.ui.widgets.Spacer({ h = line_height })
    end
    return label(line)
  end

  -- Every row is as wide as the dialog, so that the dialog keeps its width
  -- whichever rows show as the list scrolls.
  local line_rows = {}
  for i, line in ipairs(lines) do
    line_rows[i] = trx.ui.widgets.Resize({
      w = width,
      scaled = true,
      visible = list:shows(i),
      child = line_widget(line),
    })
  end

  local choice = trx.signal.new(1)
  local root = trx.ui.widgets.Fit({
    child = trx.ui.widgets.Pad({
      x = CONFIRM_PAD,
      y = CONFIRM_PAD,
      child = window.new({
        title = accent.text(wrapped_title),
        heavy = true,
        scroll = list,
        reserve_scroll = true,
        header = not compact and trx.ui.widgets.Stack({
          align = trx.ui.HAlign.SPAN,
          children = {
            label(wrapped_description),
            trx.ui.widgets.Spacer({ h = line_height }),
          },
        }) or nil,
        content = trx.ui.widgets.Stack({
          align = trx.ui.HAlign.SPAN,
          spacing = CONFIRM_ROW_SPACING,
          children = line_rows,
        }),
        footer = trx.ui.widgets.Stack({
          align = trx.ui.HAlign.SPAN,
          children = {
            trx.ui.widgets.Spacer({ h = line_height, visible = not compact }),
            trx.ui.widgets.Label({
              text = wrapped_note,
              visible = not compact,
            }),
            trx.ui.widgets.Spacer({ h = line_height }),
            centered(trx.ui.widgets.Stack({
              orientation = trx.ui.Orientation.HORIZONTAL,
              v_align = trx.ui.VAlign.CENTER,
              spacing = CHOICE_SPACING,
              children = {
                choice_widget(apply_text, choice:eq(1)),
                choice_widget(back_text, choice:eq(2)),
              },
            })),
          },
        }),
      }),
    }),
  })

  return push({
    root = root,
    on_input = function(layer, keys)
      if keys:pressed(Role.MENU_DOWN) then
        list:scroll_down(false)
      elseif keys:pressed(Role.MENU_UP) then
        list:scroll_up(false)
      end
      if keys:pressed(Role.MENU_LEFT) then
        choice:set(1)
      elseif keys:pressed(Role.MENU_RIGHT) then
        choice:set(2)
      end
      local confirmed = keys:pressed(Role.MENU_CONFIRM)
      if confirmed and choice:get() == 1 then
        layer:close()
        presets.apply(preset)
        show_message(
          push,
          title,
          trx.locale.get("general/config_presets/applied")
        )
      elseif confirmed or keys:pressed(Role.MENU_BACK) then
        layer:close()
      end
    end,
  })
end

---@class common.ui.settings.PresetTab: common.ui.settings.Tab
---@field title string|trx.signal.Signal The name on the tab bar, or a signal that holds it.
---@field presets common.ui.settings.Preset[] The presets, in the order shown.
---@field scroll common.ui.Scroll The presets and the one under the cursor.
---@field widget trx.ui.Widget The list.
---@field footer trx.ui.Widget What shows under the dialog, which is room alone.
---@field private push fun(settings: trx.ui.layers.push.settings): trx.ui.StackLayer
local PresetTab = {}
PresetTab.__index = PresetTab

-- The presets list takes the width that the other tabs give the dialog.
function PresetTab:widths()
  return 0, 0
end

-- Shows as many presets as fit in the height of `row_count` rows of settings,
-- and returns the height that they take.
---@param row_count integer
---@return number
function PresetTab:fit(row_count)
  local line = metrics.line_height()
  local room = row_count * line
  local shown =
    math.floor((room + LIST_ROW_SPACING) / (line + LIST_ROW_SPACING))
  shown = math.min(shown, #self.presets)
  self.scroll:set_visible(shown)
  if shown <= 0 then
    return 0
  end
  return shown * line + (shown - 1) * LIST_ROW_SPACING
end

-- Reads the keys for the list. Returns whether the tab took them, and whether
-- the cursor left the list for the tab bar. Back is left to the dialog, which
-- closes as it does from any other tab.
---@param keys trx.ui.LayerKeys
---@return boolean consumed
---@return boolean leave
function PresetTab:control(keys)
  local wraps = scroll.wraps()
  local count = #self.presets
  local at = self.scroll.selected:get() or 0
  if keys:pressed(Role.MENU_UP) then
    return true, not self.scroll:select_prev(false)
  elseif keys:pressed(Role.MENU_DOWN) then
    if count == 0 or (at >= count and wraps) then
      return true, true
    end
    self.scroll:select_next(false)
    return true, false
  elseif keys:pressed(Role.MENU_CONFIRM) and at >= 1 then
    local preset = self.presets[at] --[[@as common.ui.settings.Preset]]
    local changes = presets.changes(preset)
    if #changes > 0 then
      confirm(self.push, preset, changes)
    else
      show_message(
        self.push,
        trx.locale.format(
          "general/config_presets/title_fmt",
          presets.title(preset)
        ),
        trx.locale.get("general/config_presets/no_changes")
      )
    end
    return true, false
  end
  return false, false
end

function PresetTab:close() end

---@class common.ui.settings.PresetTab.settings
---@field title string|trx.signal.Signal The name on the tab bar, or a signal that holds it.
---@field editing trx.signal.Signal Whether the cursor is on the list rather than on the tab bar.
---@field changed trx.signal.Signal Says when any setting changes, for as long as the dialog is open. The names follow the player's language through it.
---@field push fun(settings: trx.ui.layers.push.settings): trx.ui.StackLayer Opens a layer over the dialog.

-- Creates the presets tab.
---@param settings common.ui.settings.PresetTab.settings
---@return common.ui.settings.PresetTab
function M.new(settings)
  local self = setmetatable({
    title = settings.title,
    presets = presets.load(),
    push = settings.push,
  }, PresetTab)
  self.scroll = scroll.new({ count = #self.presets })

  local tr1_pad = metrics.frame_overhang()
  local children = {}
  for i, preset in ipairs(self.presets) do
    local selected = trx.signal.combine(
      settings.editing,
      self.scroll.selected,
      function(editing, at)
        return editing and at == i
      end
    )
    children[i] = trx.ui.widgets.Pad({
      y = -tr1_pad,
      visible = self.scroll:shows(i),
      child = trx.ui.widgets.Frame({
        style = selected:map(function(on)
          return on and trx.ui.FrameStyle.SELECTED or false
        end),
        child = trx.ui.widgets.Pad({
          x = ROW_PAD,
          y = tr1_pad,
          child = centered(label(settings.changed:map(function()
            return presets.title(preset)
          end))),
        }),
      }),
    })
  end
  if #self.presets == 0 then
    children[1] = centered(label(settings.changed:map(function()
      return trx.locale.get("general/config_presets/empty")
    end)))
  end
  self.widget = trx.ui.widgets.Stack({
    align = trx.ui.HAlign.SPAN,
    spacing = LIST_ROW_SPACING,
    children = children,
  })
  self.footer = trx.ui.widgets.Spacer({ h = metrics.line_height() })
  return self
end

return M
