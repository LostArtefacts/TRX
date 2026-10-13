-- The engine's list dialog, built from widgets: a frame, a heading, a list of
-- rows, and an optional footer under the frame. The geometry follows the
-- engine's requester, so that a dialog built here sits where the player
-- expects it.
--
-- Usage:
--   local requester = require("common.ui.widgets.requester")
--   local req = requester.new({ title = "Load Game", rows = rows })
--   ctx:push({ root = req, place = req.place, on_input = ... })

local metrics = require("common.ui.metrics")

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
local function fit_rows(settings, count, width)
  local function fits(scrolls)
    local room = available_height(width)
      - (settings.footer_height or 0)
      - chrome_height(settings.title ~= nil, scrolls)
      + settings.row_spacing
    local rows =
      math.floor(room / (metrics.TEXT_HEIGHT + settings.row_spacing))
    return math.min(math.max(rows, MIN_VISIBLE_ROWS), MAX_VISIBLE_ROWS)
  end
  local rows = fits(false)
  if settings.arrows ~= false and count > rows then
    rows = fits(true)
  end
  return rows
end

-- The dialog's own state, which the widget it hands out reads through.
---@class common.ui.RequesterState
---@field rows trx.ui.ListRow[]
---@field list trx.ui.List
---@field title? trx.signal.Signal
---@field panel trx.ui.Widget
---@field refit fun()
local Requester = {}
Requester.__index = Requester

-- A dialog, which is a widget that also reads the player's input.
---@class common.ui.Requester: trx.ui.Widget
---@field rows trx.ui.ListRow[] The rows, as `set_rows` last gave them.
---@field list trx.ui.List The list of the rows.
---@field title? trx.signal.Signal The heading, or nil for none.
---@field panel trx.ui.Widget The frame, without the footer.
---@field place? fun(w: number, h: number): number, number Where the dialog goes, as trx.ui.layers.push takes it.
---@field set_rows fun(self: common.ui.Requester, rows: trx.ui.ListRow[])
---@field selected_row fun(self: common.ui.Requester): trx.ui.ListRow?
---@field control fun(self: common.ui.Requester, keys: trx.ui.LayerKeys): integer|"cancel"|nil Reads one tick of input. Returns the picked row's index, "cancel" where the player backed out, and nil where nothing was decided.

-- Replaces the rows.
function Requester:set_rows(rows)
  self.rows = rows
  self.refit()
  self.list:set_rows(rows)
end

-- The row that the cursor is on, or nil for none.
function Requester:selected_row()
  local index = self.list:selection()
  return index ~= nil and self.rows[index] or nil
end

-- Reads one tick of input. Returns the picked row's index, "cancel" where
-- the player backed out, and nil where nothing was decided.
function Requester:control(keys)
  local picked = self.list:control(keys)
  if picked ~= nil then
    return picked
  end
  if keys:pressed(trx.input.Role.MENU_BACK) then
    return "cancel"
  end
  return nil
end

-- Creates a dialog, which is a widget to push or to place in another widget.
-- The settings are:
--   title         - the heading, or nil for none
--   rows          - the rows, as trx.ui.widgets.List takes them
--   width         - the least width of the whole frame
--   reserve       - whether to keep room for every row the screen fits
--   row_pad       - the room on each side of a row's text
--   row_spacing   - the gap between two rows
--   arrows        - whether to show scroll arrows, true by default
--   footer        - a widget drawn under the frame
--   footer_height - the height the footer takes, for fitting the rows
--   place         - where the dialog goes, as trx.ui.layers.push takes it
function M.new(settings)
  settings.row_spacing = settings.row_spacing or metrics.row_spacing()
  ---@type common.ui.RequesterState
  local req = setmetatable({ rows = settings.rows or {} }, Requester)

  -- How wide the frame is decides which corners it has to keep clear of, and
  -- so how much room its rows have.
  local function frame_width()
    if req.panel ~= nil then
      return (req.panel:measure())
    end
    return settings.width
  end

  -- How many rows fit, which follows the rows and the room the screen leaves.
  -- The list measures itself again only where the count changes.
  local visible = trx.signal.new(fit_rows(settings, #req.rows, frame_width()))
  function req.refit()
    visible:set(fit_rows(settings, #req.rows, frame_width()))
  end

  req.list = trx.ui.widgets.List({
    rows = req.rows,
    visible = visible,
    reserve = settings.reserve,
    width = settings.width ~= nil
        and settings.width - 2 * (metrics.outer_pad() + metrics.body_pad_x())
      or nil,
    row_pad = settings.row_pad,
    row_spacing = settings.row_spacing,
    scroll_hints = settings.arrows ~= false,
  })

  local children = {}
  if settings.title ~= nil then
    req.title = trx.signal.new(settings.title)
    children[#children + 1] = trx.ui.widgets.Frame({
      style = trx.ui.FrameStyle.HEADING,
      z = 160,
      child = trx.ui.widgets.Pad({
        x = metrics.TITLE_PAD_X + metrics.body_pad_x(),
        y = metrics.title_pad_y(),
        child = trx.ui.widgets.Stack({
          align = trx.ui.HAlign.CENTER,
          children = { trx.ui.widgets.Label({ text = req.title }) },
        }),
      }),
    })
  end
  children[#children + 1] = trx.ui.widgets.Pad({
    x = metrics.body_pad_x(),
    y = metrics.BODY_PAD_Y,
    child = req.list,
  })

  local panel = trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.DIALOG,
    z = 170,
    child = trx.ui.widgets.Pad({
      x = metrics.outer_pad(),
      y = metrics.outer_pad(),
      child = trx.ui.widgets.Stack({
        spacing = TITLE_SPACING,
        align = trx.ui.HAlign.SPAN,
        children = children,
      }),
    }),
  })

  req.panel = panel

  local body = panel
  if settings.footer ~= nil then
    body = trx.ui.widgets.Stack({
      spacing = FOOTER_SPACING,
      align = trx.ui.HAlign.SPAN,
      children = { panel, settings.footer },
    })
  end
  ---@type common.ui.Requester
  local widget = trx.ui.widgets.Fit({ child = body })
  widget.rows = req.rows
  widget.list = req.list
  widget.title = req.title
  widget.panel = req.panel
  widget.place = settings.place
  function widget.set_rows(_, rows)
    req:set_rows(rows)
    widget.rows = rows
  end
  function widget.selected_row()
    return req:selected_row()
  end
  function widget.control(_, keys)
    return req:control(keys)
  end
  widget:release_with(trx.ui.signals.layout:on(req.refit))
  return widget
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

-- Remembers where a dialog goes, and works it out again only when the layout,
-- the dialog's size or `depends` changes.
local function remembered(place, depends)
  local stamp, last_w, last_h, last_depends, x, y
  return function(w, h)
    local now = trx.ui.signals.layout:get()
    local d = depends ~= nil and depends() or nil
    if now ~= stamp or w ~= last_w or h ~= last_h or d ~= last_depends then
      stamp, last_w, last_h, last_depends = now, w, h, d
      x, y = place(w, h)
    end
    return x, y
  end
end

-- Places a dialog across the safe area: centered from side to side, and the
-- given fraction of the way down the room that the margin leaves.
function M.anchored(anchor, margin)
  margin = margin or 0
  return remembered(function(w, h)
    local safe = trx.ui.safe_area_for(w)
    return safe.x + (safe.width - w) / 2,
      safe.y + margin + (safe.height - 2 * margin - h) * anchor
  end)
end

-- Places a dialog so that the middle of its frame rests on the line that a
-- standard page rests on: a reserved, titled list long enough to scroll, with
-- the given footer under it, the given fraction of the way down the safe area.
-- Every passport page rests on this line, whatever its own height.
function M.on_page_line(req, anchor, footer_height)
  local function panel_height()
    local _, height = req.panel:measure()
    return height
  end
  return remembered(function(w)
    local spacing = metrics.row_spacing()
    local rows = fit_rows({
      title = "",
      row_spacing = spacing,
      footer_height = footer_height,
    }, MAX_VISIBLE_ROWS + 1, w)
    local height = chrome_height(true, true)
      + rows * metrics.TEXT_HEIGHT
      + (rows - 1) * spacing
    local line = (available_height(w) - height - 5) * anchor
      + (height - footer_height) / 2
    local safe = trx.ui.safe_area_for(w)
    return safe.x + (safe.width - w) / 2,
      safe.y + line * trx.ui.text_scale - panel_height() / 2
  end, panel_height)
end

return M
