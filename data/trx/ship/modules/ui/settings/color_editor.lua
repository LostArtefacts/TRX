-- The color editor that a color setting opens: the color with its code, and a
-- slider each for its hue, its chroma and its lightness in OKLCH. Every press
-- writes the color back to the setting, so the game shows it at once.
--
-- Usage:
--   local color_editor = require("common.ui.settings.color_editor")
--   color_editor.open(function(settings)
--     return ctx:push(settings)
--   end, "visuals.water_color")

local metrics = require("common.ui.metrics")
local scroll = require("common.ui.scroll")
local handlers = require("common.ui.settings.handlers")
local rows = require("common.ui.settings.rows")
local color = require("common.ui.widgets.color")
local row_arrows = require("common.ui.widgets.row_arrows")
local text_dialog = require("common.ui.widgets.text_dialog")

local M = {}

local SECTION_SPACING = 5
local ROW_SPACING = 4
local CODE_SPACING = 10
local SWATCH_WIDTH = 48
local SWATCH_HEIGHT = 12
local SLIDER_WIDTH = 100
local MAX_CHROMA = 0.4

-- How far a press moves a slider: degrees of hue, or hundredths of the range
-- of the chroma and the lightness.
local STEP_FAST = 5
local STEP_SLOW = 1

local Role = trx.input.Role

-- Each slider runs through these stops of its own component, and takes the
-- other two components from the color being edited.
local SLIDERS = {
  {
    label = function()
      return trx.locale.get("general/settings/common/hue")
    end,
    stops = {
      { h = 0 },
      { h = 60 },
      { h = 120 },
      { h = 180 },
      { h = 240 },
      { h = 300 },
      { h = 360 },
    },
    position = function(lch)
      return lch.h / 360
    end,
    step = function(lch, delta)
      lch.h = (lch.h + delta) % 360
    end,
  },
  {
    label = function()
      return trx.locale.get("general/settings/common/chroma")
    end,
    stops = { { c = 0 }, { c = MAX_CHROMA } },
    position = function(lch)
      return lch.c / MAX_CHROMA
    end,
    step = function(lch, delta)
      lch.c =
        math.max(0, math.min(lch.c + delta / 100 * MAX_CHROMA, MAX_CHROMA))
    end,
  },
  {
    label = function()
      return trx.locale.get("general/settings/common/lightness")
    end,
    stops = { { l = 0 }, { l = 0.5 }, { l = 1 } },
    position = function(lch)
      return lch.l
    end,
    step = function(lch, delta)
      lch.l = math.max(0, math.min(lch.l + delta / 100, 1))
    end,
  },
}

local function to_lch(rgb)
  local l, c, h = trx.math.to_oklch(rgb)
  return { l = l, c = math.max(0, math.min(c, MAX_CHROMA)), h = h }
end

local function to_rgb(lch)
  return trx.math.from_oklch(lch.l, lch.c, lch.h)
end

local function slider_row(index, slider, lch, selected)
  local is_selected = selected:map(function(at)
    return at == index
  end)
  local stops = lch:map(function(state)
    local result = {}
    for i, stop in ipairs(slider.stops) do
      result[i] = to_rgb({
        l = stop.l or state.l,
        c = stop.c or state.c,
        h = stop.h or state.h,
      })
    end
    return result
  end)
  local pad = metrics.frame_overhang()
  return trx.ui.widgets.Frame({
    style = is_selected:map(function(on)
      return on and trx.ui.FrameStyle.SELECTED or false
    end),
    child = trx.ui.widgets.Pad({
      x = pad,
      y = pad,
      child = trx.ui.widgets.Stack({
        orientation = trx.ui.Orientation.HORIZONTAL,
        align = trx.ui.HAlign.DISTRIBUTE,
        children = {
          trx.ui.widgets.Label({ text = slider.label() }),
          row_arrows.new({
            left = is_selected,
            right = is_selected,
            child = color.slider({
              stops = stops,
              value = lch:map(slider.position),
              w = SLIDER_WIDTH,
            }),
          }),
        },
      }),
    }),
  })
end

-- Opens the editor over the dialog for a color setting.
---@param push fun(settings: trx.ui.layers.push.settings): trx.ui.StackLayer Opens a layer.
---@param key string The setting.
---@return trx.ui.StackLayer
function M.open(push, key)
  local lch = trx.signal.new(to_lch(trx.config.get(key)))
  local selected = trx.signal.new(1)
  local rgb = lch:map(to_rgb)

  local sliders = {}
  for i, slider in ipairs(SLIDERS) do
    sliders[i] = slider_row(i, slider, lch, selected)
  end

  local root = text_dialog.frame({
    title = rows.title(key),
    heavy = true,
    spacing = SECTION_SPACING,
    body = {
      trx.ui.widgets.Stack({
        spacing = ROW_SPACING,
        align = trx.ui.HAlign.SPAN,
        children = {
          trx.ui.widgets.Stack({
            orientation = trx.ui.Orientation.HORIZONTAL,
            align = trx.ui.HAlign.DISTRIBUTE,
            spacing = CODE_SPACING,
            children = {
              trx.ui.widgets.Label({ text = rgb:map(rows.hex) }),
              row_arrows.new({
                child = color.swatch({
                  color = rgb,
                  w = SWATCH_WIDTH,
                  h = SWATCH_HEIGHT,
                }),
              }),
            },
          }),
          table.unpack(sliders),
        },
      }),
    },
  })

  local function control(layer, keys)
    if
      not rows.exists(key)
      or keys:pressed(Role.MENU_BACK)
      or keys:pressed(Role.MENU_SHOW_INFO)
    then
      layer:close()
      return
    end
    local step = keys:pressed(Role.MENU_UP) and -1
      or keys:pressed(Role.MENU_DOWN) and 1
      or nil
    if step ~= nil then
      selected:set(
        scroll.step(selected:get(), step, #SLIDERS) or selected:get()
      )
    elseif keys:pressed(Role.MENU_LEFT) or keys:pressed(Role.MENU_RIGHT) then
      local handler = handlers.get(key)
      local fine = keys:held(Role.MENU_FINE_ADJUST)
      local delta = fine and (handler.slow or STEP_SLOW)
        or (handler.fast or STEP_FAST)
      if keys:held(Role.MENU_LEFT) then
        delta = -delta
      end
      local state = lch:get()
      local next_state = { l = state.l, c = state.c, h = state.h }
      SLIDERS[selected:get()].step(next_state, delta)
      lch:set(next_state)
      trx.config.set(key, to_rgb(next_state))
    elseif keys:pressed(Role.UNBIND_KEY) then
      trx.config.reset(key)
      lch:set(to_lch(trx.config.get(key)))
    end
  end

  return push({ root = root, on_input = control })
end

return M
