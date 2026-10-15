-- One tab of a settings dialog: a row for each setting the tab shows, with its
-- title on the left and its value on the right, and the hints under the dialog
-- that say what the keys do to the row under the cursor.
--
-- A row is built once for every setting the tab lists. The rows follow the
-- settings through the dialog's signal of their changes, so a row that a
-- setting hides or greys out does so as that setting moves.
--
-- Usage:
--   local editor = require("common.ui.settings.editor")
--   local tab = editor.new({ title = "Volume", keys = keys, editing = editing,
--     width = width, dialog_width = dialog_width, push = push })

local metrics = require("common.ui.metrics")
local rows = require("common.ui.settings.rows")
local scroll = require("common.ui.scroll")
local color = require("common.ui.widgets.color")
local row_arrows = require("common.ui.widgets.row_arrows")
local text_dialog = require("common.ui.widgets.text_dialog")

local M = {}

local COLUMN_SPACING = 20
local FOOTER_SPACING = 20
local BAR_WIDTH = 60
local BAR_HEIGHT = 12
local SWATCH_SPACING = 4
local SWATCH_SIZE = metrics.TEXT_HEIGHT - 2

local Role = trx.input.Role

local width_of = metrics.text_width

-- How wide the arrows around a value are, with the gaps they keep from it.
local function arrows_width()
  return width_of("\\{button left}")
    + width_of("\\{button right}")
    + 2 * row_arrows.Spacing.MEDIUM
end

-- A box as wide as `width` holds, with the child placed in it by `align`.
local function lane(width, align, child)
  return trx.ui.widgets.Stack({
    align = align,
    children = { trx.ui.widgets.Spacer({ w = width }), child },
  })
end

---@class common.ui.settings.Editor: common.ui.settings.Tab
---@field keys string[] Every setting the tab lists that the game has.
---@field shown trx.signal.Signal The settings the tab shows, in order.
---@field private positions trx.signal.Signal Where each shown setting sits, counted from 1.
---@field scroll common.ui.Scroll The rows and the one under the cursor.
---@field widget trx.ui.Widget The rows.
---@field footer trx.ui.Widget The hints under the dialog.
---@field value_w trx.signal.Signal How wide the widest value is, with its arrows.
---@field private watch trx.signal.Listener
---@field private values table<string, { widget: trx.ui.Widget }>
---@field private changed trx.signal.Signal
---@field private layout trx.signal.Signal
---@field private widths_at? string
---@field private value_widths table<string, number>
---@field private push fun(settings: trx.ui.layers.push.settings): trx.ui.StackLayer
---@field private edit_color? fun(key: string)
local Editor = {}
Editor.__index = Editor

-- The setting under the cursor, or nil.
---@return string?
function Editor:selected()
  local index = self.scroll.selected:get()
  return index ~= nil and self.shown:get()[index] or nil
end

-- The widths the tab needs, in canvas units at the default text size: the
-- whole of its widest title and widest value, and the widest single row.
---@return number content
---@return number least
function Editor:widths()
  local arrows = arrows_width()
  local label_w, value_w, row_w = 0.0, 0.0, 0.0
  for _, key in ipairs(self.keys) do
    local title_w = width_of(rows.title(key))
    local own = self:value_width(key) + arrows
    label_w = math.max(label_w, title_w)
    value_w = math.max(value_w, own)
    row_w = math.max(row_w, title_w + COLUMN_SPACING + own)
  end
  return label_w + COLUMN_SPACING + value_w, row_w
end

-- How wide the value of a row can get, measured afresh.
local function measure_value(self, key)
  -- A bar and a swatch have sizes of their own, so the widget that shows them
  -- is measured rather than its text.
  if rows.bar_type(key) ~= nil then
    return self.values[key].widget:measure() / trx.ui.text_scale
  end
  local result = 0.0
  for _, text in ipairs(rows.value_texts(key)) do
    result = math.max(result, width_of(text))
  end
  if rows.is_color(key) then
    result = math.max(result, width_of("#FFFFFF"))
      + SWATCH_SPACING
      + SWATCH_SIZE
  end
  return result
end

-- How wide the value of a row can get, without its arrows.
---@param key string
---@return number
function Editor:value_width(key)
  -- The widths hold until a setting or the room changes, and the rows, the
  -- tab and the dialog each ask for every one of them.
  local revision = ("%s:%s:%s"):format(
    self.changed:get(),
    self.layout:get(),
    trx.ui.text_scale
  )
  if self.widths_at ~= revision then
    self.widths_at = revision
    self.value_widths = {}
  end
  local cached = self.value_widths[key]
  if cached == nil then
    cached = measure_value(self, key)
    self.value_widths[key] = cached
  end
  return cached
end

-- The texts of the hints under the dialog.
local function edit_hint()
  return "\\{input menu_confirm} "
    .. trx.locale.get("general/settings/common/edit_value")
end

local function help_hint()
  return "\\{input menu_show_info} "
    .. trx.locale.get("general/settings/common/toggle_help")
end

local function restore_hint()
  return "\\{input unbind_key} "
    .. trx.locale.get("general/settings/common/restore_default")
end

-- How wide the first hint is at most, and both hints, without the gap.
local function hint_widths()
  local first = math.max(width_of(edit_hint()), width_of(help_hint()))
  return first, first + width_of(restore_hint())
end

-- Shows up to `rows` rows, and returns the height that they take.
---@param rows integer
---@return number
function Editor:fit(rows)
  local shown = math.min(rows, self.scroll.count:get())
  self.scroll:set_visible(shown)
  return shown * metrics.line_height()
end

-- Stops following the settings, once the dialog has closed.
function Editor:close()
  self.watch:detach()
end

-- Opens the description of the setting under the cursor over the dialog.
function Editor:describe(key)
  local dialog = text_dialog.new({ heavy = true })
  local title = rows.title(key)
  local text = rows.description(key)
  if rows.held(key) then
    title = title .. "*"
    text = "* "
      .. trx.locale.get("general/settings/common/frozen_option_disclaimer")
      .. "\n\n"
      .. text
  end
  dialog:set_text(
    title,
    text,
    trx.ui.canvas.width * 2 / 3,
    math.max(self.scroll.visible:get(), 1)
  )
  self.push({
    root = dialog.widget,
    on_input = function(layer, keys)
      dialog:control(keys)
      if keys:pressed(Role.MENU_BACK) or keys:pressed(Role.MENU_SHOW_INFO) then
        layer:close()
      end
    end,
  })
end

-- Reads the keys for the rows. Returns whether the tab took them, and whether
-- the cursor left the rows for the tab bar.
---@param keys trx.ui.LayerKeys
---@return boolean consumed
---@return boolean leave
function Editor:control(keys)
  local key = self:selected()
  if key ~= nil then
    local fine = keys:held(Role.MENU_FINE_ADJUST)
    if keys:pressed(Role.MENU_LEFT) then
      rows.change(key, -1, fine)
      return true, false
    elseif keys:pressed(Role.MENU_RIGHT) then
      rows.change(key, 1, fine)
      return true, false
    end
  end
  if keys:pressed(Role.MENU_UP) then
    return true, not self.scroll:select_prev(false)
  elseif keys:pressed(Role.MENU_DOWN) then
    local wraps = trx.config.get("ui.enable_wraparound")
    return true, not self.scroll:select_next(false) and wraps
  end
  if key == nil then
    return false, false
  end
  if
    keys:pressed(Role.MENU_CONFIRM)
    and rows.is_color(key)
    and not rows.held(key)
    and self.edit_color ~= nil
  then
    self.edit_color(key)
    return true, false
  elseif keys:pressed(Role.MENU_SHOW_INFO) then
    self:describe(key)
    return true, false
  elseif keys:pressed(Role.UNBIND_KEY) and rows.restore(key) then
    return true, false
  end
  return false, false
end

-- The value side of a row: a bar for a bar's colors, a swatch beside the code
-- for a color, and the value as text for the rest.
local function value_widget(key)
  local bar = rows.bar_type(key)
  if bar ~= nil then
    return trx.ui.widgets.Bar({
      type = bar,
      value = 1,
      w = BAR_WIDTH,
      h = BAR_HEIGHT,
      preview = true,
    })
  end
  local text = trx.ui.widgets.Label({
    text = rows.state(key):map(function(state)
      return state.value
    end),
  })
  if not rows.is_color(key) then
    return text
  end
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    v_align = trx.ui.VAlign.CENTER,
    spacing = SWATCH_SPACING,
    children = {
      text,
      color.swatch({
        color = rows.state(key):map(function(state)
          return state.color
        end),
        w = SWATCH_SIZE,
        h = SWATCH_SIZE,
      }),
    },
  })
end

local function row_widget(self, key, settings)
  local changed = settings.changed
  -- Whether the cursor is on the row, the frame that follows from it, and
  -- which way the setting can step, worked out together.
  local look = trx.signal.combine(
    settings.editing,
    self.scroll.selected,
    self.positions,
    changed,
    function(editing, selected, positions)
      local focused = editing
        and selected ~= nil
        and positions[key] == selected
      return {
        style = focused and trx.ui.FrameStyle.SELECTED or false,
        left = focused and rows.can_change(key, -1),
        right = focused and rows.can_change(key, 1),
      }
    end
  )
  local shown = trx.signal.combine(
    self.positions,
    self.scroll.first,
    self.scroll.visible,
    function(positions, first, visible)
      local at = positions[key]
      return at ~= nil and at >= first and at < first + visible
    end
  )

  -- The value takes the room its widest value needs, and more where the title
  -- leaves it; the title takes the rest.
  local own_value = self:value_width(key) + arrows_width()
  local function lanes(row_w, max_value_w)
    local title_w = width_of(rows.title(key))
    local value_lane = math.max(
      math.min(max_value_w, row_w - COLUMN_SPACING - title_w),
      own_value
    )
    return math.max(title_w, row_w - COLUMN_SPACING - value_lane), value_lane
  end
  local label_lane = trx.signal.combine(
    settings.width,
    self.value_w,
    changed,
    function(row_w, max_value_w)
      return (lanes(row_w, max_value_w))
    end
  )
  local value_lane = trx.signal.combine(
    settings.width,
    self.value_w,
    changed,
    function(row_w, max_value_w)
      return select(2, lanes(row_w, max_value_w))
    end
  )
  local content = trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    align = trx.ui.HAlign.DISTRIBUTE,
    spacing = COLUMN_SPACING,
    children = {
      trx.ui.widgets.Resize({
        scaled = true,
        w = label_lane,
        child = trx.ui.widgets.Label({
          text = rows.state(key):map(function(state)
            return state.title
          end),
        }),
      }),
      lane(
        value_lane,
        trx.ui.HAlign.RIGHT,
        row_arrows.new({
          left = look:map(function(l)
            return l.left
          end),
          right = look:map(function(l)
            return l.right
          end),
          child = self.values[key].widget,
        })
      ),
    },
  })
  local style = look:map(function(l)
    return l.style
  end)
  -- In TR1 the frame of the row under the cursor reaches past the row, and
  -- the row is padded back in. The other games need neither pad.
  local overhang = metrics.frame_overhang()
  if overhang == 0 then
    return trx.ui.widgets.Frame({
      shown = shown,
      style = style,
      child = content,
    })
  end
  return trx.ui.widgets.Pad({
    x = -overhang,
    y = -overhang,
    shown = shown,
    child = trx.ui.widgets.Frame({
      style = style,
      child = trx.ui.widgets.Pad({
        x = overhang,
        y = overhang,
        child = content,
      }),
    }),
  })
end

local function footer_widget(self, settings)
  local changed = settings.changed
  local hints = trx.signal.combine(
    settings.editing,
    self.scroll.selected,
    changed,
    function(editing)
      local key = editing and self:selected() or nil
      if key == nil then
        return { help = false, edit = false, restore = false }
      end
      local free = not rows.held(key)
      return {
        help = true,
        edit = rows.is_color(key) and free,
        restore = free and not trx.config.is_default(key),
      }
    end
  )
  -- The hints shrink to the width of the dialog, and the first one keeps the
  -- room of the wider of its two texts, so that the footer keeps one width
  -- whichever it shows.
  local fit = trx.signal.combine(
    settings.dialog_width,
    settings.layout,
    function(dialog_w)
      local first, texts = hint_widths()
      local scale = texts > 0
          and math.min(1.0, (dialog_w - FOOTER_SPACING) / texts)
        or 1.0
      return { scale = scale, first = first * scale }
    end
  )
  local scale = fit:map(function(f)
    return f.scale
  end)
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    align = trx.ui.HAlign.DISTRIBUTE,
    spacing = FOOTER_SPACING,
    children = {
      lane(
        fit:map(function(f)
          return f.first
        end),
        trx.ui.HAlign.LEFT,
        trx.ui.widgets.Label({
          scale = scale,
          hidden = hints:map(function(h)
            return not h.help
          end),
          text = hints:map(function(h)
            return h.edit and edit_hint() or help_hint()
          end),
        })
      ),
      trx.ui.widgets.Label({
        scale = scale,
        hidden = hints:map(function(h)
          return not h.restore
        end),
        text = restore_hint(),
      }),
    },
  })
end

---@class common.ui.settings.Editor.settings
---@field title string The name on the tab bar.
---@field keys string[] The settings the tab lists, in order.
---@field editing trx.signal.Signal Whether the cursor is on the rows rather than on the tab bar.
---@field width trx.signal.Signal How wide a row is, in canvas units at the default text size.
---@field dialog_width trx.signal.Signal How wide the dialog is, in the same units.
---@field changed trx.signal.Signal Says when any setting changes, as `trx.config.signals.changed` does, for as long as the dialog is open.
---@field layout trx.signal.Signal Says when the room the interface lays out in changes, as `trx.ui.signals.layout` does, for as long as the dialog is open.
---@field push fun(settings: trx.ui.layers.push.settings): trx.ui.StackLayer Opens a layer over the dialog.
---@field edit_color? fun(key: string) Opens the color editor for a setting.

-- Creates a tab of rows.
---@param settings common.ui.settings.Editor.settings
---@return common.ui.settings.Editor
function M.new(settings)
  local keys = {}
  for _, key in ipairs(settings.keys) do
    if rows.exists(key) then
      keys[#keys + 1] = key
    end
  end
  local self = setmetatable({
    keys = keys,
    title = settings.title,
    changed = settings.changed,
    layout = settings.layout,
    value_widths = {},
    push = settings.push,
    edit_color = settings.edit_color,
    scroll = scroll.new(),
  }, Editor)

  local changed = settings.changed
  self.shown = changed:map(function()
    local shown = {}
    for _, key in ipairs(keys) do
      if not rows.hidden(key) then
        shown[#shown + 1] = key
      end
    end
    return shown
  end)
  self.positions = self.shown:map(function(shown)
    local positions = {}
    for i, key in ipairs(shown) do
      positions[key] = i
    end
    return positions
  end)
  self.scroll:set_count(#self.shown:get())
  self.watch = self.shown:on(function(shown)
    self.scroll:set_count(#shown)
  end)
  self.values = {}
  for _, key in ipairs(keys) do
    self.values[key] = { widget = value_widget(key) }
  end
  self.value_w = trx.signal.combine(changed, settings.layout, function()
    local arrows = arrows_width()
    local result = 0.0
    for _, key in ipairs(keys) do
      result = math.max(result, self:value_width(key) + arrows)
    end
    return result
  end)

  local children = {}
  for i, key in ipairs(keys) do
    children[i] = row_widget(self, key, settings)
  end
  self.widget = trx.ui.widgets.Stack({
    align = trx.ui.HAlign.SPAN,
    children = children,
  })
  self.footer = footer_widget(self, settings)
  return self
end

return M
