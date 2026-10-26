-- The engine's list dialog, built from widgets: a frame, a heading, a list of
-- rows, and an optional footer under the frame. The geometry follows the
-- engine's requester, so that a dialog built here sits where the player
-- expects it.
--
-- Usage:
--   local requester = require("common.ui.widgets.requester")
--   local req = requester.new({ title = "Load Game", rows = rows })
--   ctx:push({ root = req.widget, place = req.place, on_input = ... })

local metrics = require("common.ui.metrics")
local window = require("common.ui.widgets.window")

local M = {}

local TITLE_SPACING = 3
local FOOTER_SPACING = 3
local MESSAGE_PAD = 8

-- A list shorter than this is awkward to browse, so a page that cannot fit
-- this many rows is drawn smaller instead of losing further rows.
local MIN_VISIBLE_ROWS = 5

-- As many rows as the originals show, however much room a large screen leaves.
local MAX_VISIBLE_ROWS = 10

-- The height of everything but the rows, in canvas units at the default text
-- size.
local function chrome_height(has_title, scrolls)
  return metrics.chrome_height(has_title and TITLE_SPACING or nil, scrolls)
end

-- The room a dialog of the given width has, which is the whole safe area
-- unless it is narrow enough to clear what sits in the corners.
local function available_height(width)
  local area = width ~= nil and trx.ui.safe_area_for(width) or trx.ui.safe_area
  return area.height / trx.ui.text_scale
end

-- How many rows fit on the screen, within the bounds the originals keep. The
-- scroll arrows take room of their own, so the rows are counted again with
-- them where the list does not fit without them.
local function fit_rows(fitting, count, width)
  local function fits(scrolls)
    local room = available_height(width)
      - (fitting.footer_height or 0)
      - chrome_height(fitting.title ~= nil, scrolls)
      + fitting.row_spacing
    local rows =
      math.floor(room / (metrics.line_height() + fitting.row_spacing))
    return math.min(math.max(rows, MIN_VISIBLE_ROWS), MAX_VISIBLE_ROWS)
  end
  local rows = fits(false)
  if fitting.arrows ~= false and count > rows then
    rows = fits(true)
  end
  return rows
end

-- A dialog: its widget, and the list that reads its input.
---@class common.ui.Requester
---@field widget trx.ui.Widget The dialog, to push or to place in another widget.
---@field rows trx.ui.ListRow[] The rows, as `set_rows` last gave them.
---@field list trx.ui.List The list of the rows.
---@field title? trx.signal.Signal The heading, or nil for none.
---@field panel trx.ui.Widget The frame, without the footer.
---@field place? trx.ui.Placement|fun(w: number, h: number): number, number Where the dialog goes, as trx.ui.layers.push takes it.
---@field private count trx.signal.Signal
local Requester = {}
Requester.__index = Requester

-- Replaces the rows.
---@param new_rows trx.ui.ListRow[]
function Requester:set_rows(new_rows)
  self.rows = new_rows
  self.count:set(#new_rows)
  self.list:set_rows(new_rows)
end

-- Returns the row that the cursor is on, or nil for none.
---@return trx.ui.ListRow?
function Requester:selected_row()
  local index = self.list:selection()
  return index ~= nil and self.rows[index] or nil
end

---@class common.ui.Requester.settings
---@field title? string|trx.locale.Text The heading, or nil for none.
---@field rows? trx.ui.ListRow[] The rows, as trx.ui.widgets.List takes them.
---@field min_w? number The least width of the whole frame.
---@field reserve? boolean Whether to keep room for every row the screen fits.
---@field row_pad? number The room on each side of a row's text.
---@field row_spacing? number The gap between two rows.
---@field arrows? boolean Whether to show scroll arrows. `true` by default.
---@field footer? trx.ui.Widget A widget drawn under the frame.
---@field footer_height? number The height the footer takes, for fitting the rows.
---@field place? trx.ui.Placement|fun(w: number, h: number): number, number Where the dialog goes, as trx.ui.layers.push takes it.

-- Creates a dialog.
---@param settings common.ui.Requester.settings
---@return common.ui.Requester
function M.new(settings)
  local fitting = {
    title = settings.title,
    row_spacing = settings.row_spacing or metrics.row_spacing(),
    footer_height = settings.footer_height,
    arrows = settings.arrows,
  }
  local rows = settings.rows or {}

  ---@type trx.ui.Widget?
  local panel = nil

  -- How wide the frame is decides which corners it has to keep clear of, and
  -- so how much room its rows have. The frame measures its rows, and they can
  -- ask for its width again before it has one, so that call takes the last
  -- width rather than measuring the frame without end.
  local last_width = settings.min_w
  local is_measuring = false
  local function frame_width()
    if panel == nil or is_measuring then
      return last_width
    end
    is_measuring = true
    last_width = panel:measure()
    is_measuring = false
    return last_width
  end

  -- How many rows fit, which follows the rows and the room the screen leaves.
  -- The list measures itself again only where the count changes.
  local count = trx.signal.new(#rows)
  local page_rows = trx.signal.combine(
    trx.ui.signals.layout,
    count,
    function(_, rows_count)
      return fit_rows(fitting, rows_count, frame_width())
    end
  )

  local list = trx.ui.widgets.List({
    rows = rows,
    page_rows = page_rows,
    reserve = settings.reserve,
    min_w = settings.min_w ~= nil
        and settings.min_w - 2 * (metrics.outer_pad() + metrics.body_pad_x())
      or nil,
    row_pad = settings.row_pad,
    row_spacing = fitting.row_spacing,
    row_height = metrics.line_height(),
    hint_height = metrics.HINT_HEIGHT,
    rule_overhang = metrics.rule_overhang(),
    frame_overhang = metrics.frame_overhang(),
    scroll_hints = settings.arrows ~= false,
  })

  ---@type trx.signal.Signal?
  local title = settings.title ~= nil and trx.signal.new(settings.title) or nil
  local frame = window.new({
    title = title,
    title_pad_x = metrics.title_pad_x() + metrics.body_pad_x(),
    content = list,
  })

  panel = frame

  local body = frame
  if settings.footer ~= nil then
    body = trx.ui.widgets.Stack({
      spacing = FOOTER_SPACING,
      align = trx.ui.HAlign.SPAN,
      children = { frame, settings.footer },
    })
  end

  return setmetatable({
    widget = trx.ui.widgets.Fit({
      child = body --[[@as trx.ui.Widget]],
    }),
    rows = rows,
    list = list,
    title = title,
    panel = frame,
    place = settings.place,
    count = count,
  }, Requester)
end

-- A message in a frame of its own, such as a save that the chosen feature
-- cannot use.
function M.message(text)
  return trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.DIALOG,
    z = 170,
    child = trx.ui.widgets.Pad({
      x = MESSAGE_PAD,
      y = MESSAGE_PAD,
      child = trx.ui.widgets.Label({ text = text }),
    }),
  })
end

-- Places a dialog so that the middle of its frame rests on the line that a
-- standard page rests on: a reserved, titled list long enough to scroll, with
-- the given footer under it, the given fraction of the way down the safe area.
-- Every passport page rests on this line, whatever its own height.
function M.on_page_line(req, anchor, footer_height)
  return function(w)
    local spacing = metrics.row_spacing()
    local rows = fit_rows({
      title = "",
      row_spacing = spacing,
      footer_height = footer_height,
    }, MAX_VISIBLE_ROWS + 1, w)
    local height = chrome_height(true, true)
      + rows * metrics.line_height()
      + (rows - 1) * spacing
    local line = (available_height(w) - height - 5) * anchor
      + (height - footer_height) / 2
    local safe = trx.ui.safe_area_for(w)
    local _, panel_height = req.panel:measure()
    return safe.x + (safe.width - w) / 2,
      safe.y + line * trx.ui.text_scale - panel_height / 2
  end
end

return M
