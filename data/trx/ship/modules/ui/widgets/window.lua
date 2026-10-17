-- The engine's dialog window, built from widgets: a frame, an optional heading,
-- and a body that holds a header, the content and a footer. A window over a
-- scrolling list shows arrows above and below the content where the list runs
-- past its edges.
--
-- Usage:
--   local window = require("common.ui.widgets.window")
--   local root = window.new({ title = "Sound", content = rows, scroll = list })

local metrics = require("common.ui.metrics")

local M = {}

local TITLE_SPACING = 3
local HINT_SCALE = 0.7
local HINT_ANCHOR_UP = 1.5
local HINT_ANCHOR_DOWN = -1.0

-- A row the height of a scroll arrow, there while `visible` holds and holding
-- the arrow while `blank` does not.
local function hint_row(glyph, anchor, visible, blank)
  local arrow = trx.ui.widgets.Custom({
    measure = function()
      return 0, metrics.HINT_HEIGHT * trx.ui.text_scale
    end,
    paint = function(x, y, w, h)
      local gw, gh = trx.ui.primitive.measure_text(glyph, HINT_SCALE)
      trx.ui.primitive.text(
        glyph,
        x + (w - gw) / 2,
        y + (h - gh) * anchor,
        HINT_SCALE,
        0
      )
    end,
  })
  return trx.ui.widgets.Blank({
    visible = visible,
    when = blank,
    child = arrow,
  })
end

-- The heading strip.
---@param title any The text, or a signal carrying it.
---@param pad_x? number The room on each side of the text. `metrics.title_pad_x()` by default.
---@return trx.ui.Widget
function M.heading(title, pad_x)
  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.HEADING,
    z = 160,
    child = trx.ui.widgets.Pad({
      x = pad_x or metrics.title_pad_x(),
      y = metrics.title_pad_y(),
      child = trx.ui.widgets.Stack({
        align = trx.ui.HAlign.CENTER,
        children = { trx.ui.widgets.Label({ text = title }) },
      }),
    }),
  })
end

-- The width that a window's frame takes around its body, in canvas units at
-- the default text size.
---@return number
function M.chrome_width()
  return 2 * (metrics.outer_pad() + metrics.body_pad_x())
end

-- The height that a window's frame takes around its body, in canvas units at
-- the default text size.
---@param settings { title?: any, title_spacing?: number, hints?: boolean }
---@return number
function M.chrome_height(settings)
  return metrics.chrome_height(
    settings.title ~= nil and (settings.title_spacing or TITLE_SPACING) or nil,
    settings.hints == true
  )
end

-- What the scroll arrows read of a list: `common.ui.Scroll` answers it.
---@class common.ui.ScrollHints
---@field visible trx.signal.Signal
---@field count trx.signal.Signal
---@field more_above fun(self: common.ui.ScrollHints): trx.signal.Signal
---@field more_below fun(self: common.ui.ScrollHints): trx.signal.Signal

---@class common.ui.Window.settings
---@field content trx.ui.Widget What the window holds.
---@field title? any The heading, or a signal carrying it. None by default.
---@field heavy? boolean Whether to draw the frame solid.
---@field title_spacing? number The gap under the heading. `3` by default.
---@field title_pad_x? number The room on each side of the heading's text. `metrics.title_pad_x()` by default.
---@field header? trx.ui.Widget A widget above the content, inside the frame.
---@field footer? trx.ui.Widget A widget under the content, inside the frame.
---@field scroll? common.ui.Scroll|common.ui.ScrollHints The list the content scrolls through, for the arrows.
---@field reserve_scroll? boolean Whether to keep room for the arrows while the list fits.

-- Creates a window.
---@param settings common.ui.Window.settings
---@return trx.ui.Widget
function M.new(settings)
  local body = {}
  body[#body + 1] = settings.header

  local scroll = settings.scroll
  local up, down = nil, nil
  if scroll ~= nil then
    local hints = settings.reserve_scroll and trx.signal.new(true)
      or trx.signal.combine(
        scroll.visible,
        scroll.count,
        function(visible, count)
          return visible < count
        end
      )
    up = hint_row("\\{arrow up}", HINT_ANCHOR_UP, hints, ~scroll:more_above())
    down =
      hint_row("\\{arrow down}", HINT_ANCHOR_DOWN, hints, ~scroll:more_below())
  end

  body[#body + 1] = up
  body[#body + 1] = settings.content
  body[#body + 1] = down
  body[#body + 1] = settings.footer

  local children = {}
  if settings.title ~= nil then
    children[#children + 1] = M.heading(settings.title, settings.title_pad_x)
  end
  children[#children + 1] = trx.ui.widgets.Pad({
    x = metrics.body_pad_x(),
    y = metrics.body_pad_y(),
    child = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.SPAN,
      children = body,
    }),
  })

  return trx.ui.widgets.Frame({
    style = settings.heavy and trx.ui.FrameStyle.DIALOG_HEAVY
      or trx.ui.FrameStyle.DIALOG,
    z = 170,
    child = trx.ui.widgets.Pad({
      x = metrics.outer_pad(),
      y = metrics.outer_pad(),
      child = trx.ui.widgets.Stack({
        spacing = settings.title_spacing or TITLE_SPACING,
        align = trx.ui.HAlign.SPAN,
        children = children,
      }),
    }),
  })
end

return M
