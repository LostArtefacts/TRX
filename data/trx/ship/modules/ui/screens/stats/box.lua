-- The box that the statistics screen draws its rows in, laid out where the
-- engine lays out its own, and shrunk to fit a screen that leaves too little
-- room.

local counts = require("common.ui.screens.stats.counts")
local hold_button = require("common.ui.widgets.hold_button")
local listing = require("common.ui.screens.stats.rows")
local metrics = require("common.ui.metrics")

local M = {}

local Mode = listing.Mode

local BARE_ROW_SPACING = 11.0
local MIN_MARGIN_ROWS = 0.5
local MIN_ROW_SPACING_ROWS = 1.0 / 6.0

local ASSAULT_MIN_WIDTH = 290.0
local ASSAULT_ROW_SPACING = 3.0

local HINT_SCALE = 0.7

local function scrolls(state)
  return state.mode == Mode.ASSAULT and state.visible:get() < #state.rows
end

-- The room the frame takes around the rows, at the default text size.
local function chrome_height(state)
  return metrics.chrome_height(state.look.title_spacing, scrolls(state))
end

local function count_layout_rows(state)
  if state.mode == Mode.ASSAULT then
    return state.visible:get() + 1
  end
  return #state.rows
end

local function row_height()
  local _, height = trx.ui.primitive.measure_text("0", 1.0)
  return height / trx.ui.text_scale
end

-- Where the screen leaves too little room, the gaps between the rows shrink
-- first, then the margin, and then the whole box.
local function layout(state)
  local rows = count_layout_rows(state)
  local rh = row_height()
  local bare = listing.is_bare(state)
  local natural_spacing = bare and BARE_ROW_SPACING or 0.0
  local natural_margin = state.look.window_margin
  local gaps = bare and rows + 1 or 0

  local result = {
    margin = natural_margin,
    spacing = natural_spacing,
    scale = 1.0,
    squeezed = false,
  }

  local function text_height()
    return rows * rh + (bare and rh or chrome_height(state))
  end

  local available = trx.ui.safe_area.height / trx.ui.text_scale
  local natural = 2.0 * natural_margin + gaps * natural_spacing
  result.squeezed = text_height() + natural > available
  if not result.squeezed then
    return result
  end

  local min_margin = math.min(rh * MIN_MARGIN_ROWS, natural_margin)
  local min_spacing = math.min(rh * MIN_ROW_SPACING_ROWS, natural_spacing)

  local excess = text_height() + natural - available
  local spare_spacing = gaps * (natural_spacing - min_spacing)
  result.spacing = gaps > 0
      and natural_spacing - math.min(excess, spare_spacing) / gaps
    or 0.0

  excess = excess - spare_spacing
  if excess > 0.0 then
    local spare_margin = 2.0 * (natural_margin - min_margin)
    result.margin = natural_margin - math.min(excess, spare_margin) / 2.0
  end

  local content = text_height() + 2.0 * result.margin
  if bare then
    content = content + (rows + 1) * result.spacing
  end
  if content > 0.0 and content > available then
    result.scale = available / content
  end
  return result
end

-- Where the box is shrunk to fit, the engine shrinks the text and the margins
-- it measures as the box is built, but not the gaps it measures later, so a
-- squeezed box is drawn the way the engine draws it only if each part takes
-- the factor the same way. Text, margins and spacers take it; gaps do not.
local function fitted(state, value)
  return value * state.fit
end

local function label(state, text)
  return trx.ui.widgets.Label({ text = text, scale = state.fit })
end

local function centered_widget(state, text, shown)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.CENTER,
    visible = shown,
    children = { label(state, text) },
  })
end

-- The bare look draws its words in capitals.
local function bare_text(state, text)
  if listing.is_bare(state) then
    return trx.strings.upper(text)
  end
  return text
end

-- A row that names a counter and gives its value, either of which can be a
-- signal.
local function pair_widget(state, key, value, shown)
  if listing.is_bare(state) then
    return trx.ui.widgets.Stack({
      orientation = trx.ui.Orientation.HORIZONTAL,
      visible = shown,
      children = { label(state, key), label(state, " "), label(state, value) },
    })
  end
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    visible = shown,
    spacing = state.look.row_spacing,
    align = trx.ui.HAlign.DISTRIBUTE,
    children = { label(state, key), label(state, value) },
  })
end

local function row_widget(state, r)
  if r.text ~= nil then
    return centered_widget(state, bare_text(state, r.text))
  end
  return pair_widget(state, bare_text(state, r.key), r.value)
end

-- One of the best times on show, `offset` rows past the first one in view.
-- It follows the scroll rather than being built again.
local function scrolled_row(state, offset)
  local row = state.first:map(function(first)
    return state.rows[first + offset]
  end)
  local function part(read)
    return row:map(function(r)
      return r ~= nil and read(r) or ""
    end)
  end
  local text = part(function(r)
    return r.text ~= nil and bare_text(state, r.text) or ""
  end)
  local key = part(function(r)
    return r.key ~= nil and bare_text(state, r.key) or ""
  end)
  local value = part(function(r)
    return r.value or ""
  end)
  local is_text = row:map(function(r)
    return r == nil or r.text ~= nil
  end)
  local is_pair = row:map(function(r)
    return r ~= nil and r.text == nil
  end)

  local centered = centered_widget(state, text, is_text)
  local pair = pair_widget(state, key, value, is_pair)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.SPAN,
    children = { centered, pair },
  })
end

-- An arrow that says the rows go on past the top or the foot of the box. It
-- keeps its room while there is nothing to point at, which `lit` says.
local function hint(state, glyph, anchor, lit)
  local scale = fitted(state, HINT_SCALE)
  return trx.ui.widgets.Custom({
    measure = function()
      local gw = lit:get() and trx.ui.primitive.measure_text(glyph, scale) or 0
      return gw, fitted(state, metrics.HINT_HEIGHT) * trx.ui.text_scale
    end,
    paint = function(x, y, w)
      if not lit:get() then
        return
      end
      local gw, gh = trx.ui.primitive.measure_text(glyph, scale)
      local slack = fitted(state, metrics.HINT_HEIGHT) * trx.ui.text_scale - gh
      trx.ui.primitive.text(
        glyph,
        x + (w - gw) / 2,
        y + slack * anchor,
        scale,
        0
      )
    end,
  })
end

local function body_rows(state)
  local children = {}
  local function add(w)
    children[#children + 1] = w
  end
  add(trx.ui.widgets.Spacer({ w = fitted(state, state.look.min_width) }))
  if state.mode ~= Mode.ASSAULT then
    for _, r in ipairs(state.rows) do
      add(row_widget(state, r))
    end
    return children
  end

  add(trx.ui.widgets.Spacer({ w = fitted(state, ASSAULT_MIN_WIDTH) }))
  local shown = {}
  for offset = 0, math.min(state.visible:get(), #state.rows) - 1 do
    shown[#shown + 1] = scrolled_row(state, offset)
  end
  add(trx.ui.widgets.Stack({
    spacing = ASSAULT_ROW_SPACING,
    align = trx.ui.HAlign.SPAN,
    children = shown,
  }))
  state.reset = hold_button.new({
    words = trx.locale.get("general/stats/assault_reset_times"),
    scale = state.fit,
    from = state.reset,
  })
  state.reset:set_enabled(counts.has_any_time())
  add(state.reset.widget)
  return children
end

local function window(state)
  local body = body_rows(state)
  if scrolls(state) then
    local visible = state.visible:get()
    local up = state.first:map(function(first)
      return first > 1
    end)
    local down = state.first:map(function(first)
      return first + visible - 1 < #state.rows
    end)
    table.insert(body, 1, hint(state, "\\{arrow up}", 1.5, up))
    body[#body + 1] = hint(state, "\\{arrow down}", -1.0, down)
  end

  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.DIALOG,
    z = 170,
    child = trx.ui.widgets.Pad({
      x = fitted(state, metrics.outer_pad()),
      y = fitted(state, metrics.outer_pad()),
      child = trx.ui.widgets.Stack({
        spacing = state.look.title_spacing,
        align = trx.ui.HAlign.SPAN,
        children = {
          trx.ui.widgets.Frame({
            style = trx.ui.FrameStyle.HEADING,
            z = 160,
            child = trx.ui.widgets.Pad({
              x = fitted(state, metrics.TITLE_PAD_X),
              y = fitted(state, metrics.title_pad_y()),
              child = centered_widget(state, listing.title_of(state)),
            }),
          }),
          trx.ui.widgets.Pad({
            x = fitted(state, metrics.body_pad_x()),
            y = fitted(state, metrics.BODY_PAD_Y),
            child = trx.ui.widgets.Stack({
              align = trx.ui.HAlign.SPAN,
              children = body,
            }),
          }),
        },
      }),
    }),
  })
end

local function bare_box(state, spacing)
  local children = { label(state, listing.title_of(state)) }
  for _, w in ipairs(body_rows(state)) do
    children[#children + 1] = w
  end
  return trx.ui.widgets.Stack({
    spacing = spacing,
    align = trx.ui.HAlign.CENTER,
    children = children,
  })
end

function M.build(state)
  local box = layout(state)
  state.anchor_y = box.squeezed and 0.5 or state.look.window_y
  state.fit = box.scale
  local dialog = listing.is_bare(state) and bare_box(state, box.spacing)
    or window(state)
  return trx.ui.widgets.Pad({
    x = fitted(state, box.margin),
    y = fitted(state, box.margin),
    visible = state.shown,
    child = dialog,
  })
end

-- The box stands in the safe area at the look's height.
function M.place(state)
  return { y = state.anchor_y }
end

return M
