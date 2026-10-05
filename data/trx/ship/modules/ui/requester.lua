-- The engine's list dialog, built from widgets: a frame, a heading, a list of
-- rows, and an optional footer under the frame. The geometry follows the
-- engine's requester, so that a dialog built here sits where the player
-- expects it.
--
-- Usage:
--   local requester = require("common.ui.requester")
--   local req = requester.new({ title = "Load Game", rows = rows })
--   ctx:push({ root = req.root, on_input = ... })

local M = {}

local TEXT_HEIGHT = 15
local HINT_HEIGHT = 7
local TITLE_PAD_X = 10
local TITLE_SPACING = 3
local BODY_PAD_Y = 4
local FOOTER_SPACING = 3
local MESSAGE_PAD = 8

-- A list shorter than this is awkward to browse, so a page that cannot fit
-- this many rows is drawn smaller instead of losing further rows.
local MIN_VISIBLE_ROWS = 5

-- As many rows as the originals show, however much room a large screen leaves.
local MAX_VISIBLE_ROWS = 10

local function is_tr1()
  return trx.game.tr_version == 1
end

local function outer_pad()
  return is_tr1() and 2 or 3
end

local function body_pad_x()
  return is_tr1() and 8 or 4
end

local function title_pad_y()
  return is_tr1() and 2 or 1
end

function M.row_spacing()
  return is_tr1() and 2 or 3
end

-- The height of everything but the rows, in canvas units at the default text
-- size.
local function chrome_height(has_title, scrolls)
  ---@type number
  local height = 2 * (outer_pad() + BODY_PAD_Y)
  if has_title then
    height = height + TEXT_HEIGHT + 2 * title_pad_y() + TITLE_SPACING
  end
  if scrolls then
    height = height + 2 * HINT_HEIGHT
  end
  return height
end

local function available_height()
  return trx.ui.safe_area.height / trx.ui.text_scale
end

-- How many rows fit on the screen, within the bounds the originals keep. The
-- scroll arrows take room of their own, so the rows are counted again with
-- them where the list does not fit without them.
local function fit_rows(settings, count)
  local function fits(scrolls)
    local room = available_height()
      - (settings.footer_height or 0)
      - chrome_height(settings.title ~= nil, scrolls)
      + settings.row_spacing
    local rows = math.floor(room / (TEXT_HEIGHT + settings.row_spacing))
    return math.min(math.max(rows, MIN_VISIBLE_ROWS), MAX_VISIBLE_ROWS)
  end
  local rows = fits(false)
  if settings.arrows ~= false and count > rows then
    rows = fits(true)
  end
  return rows
end

-- Creates a dialog. The settings are:
--   title         - the heading, or nil for none
--   rows          - the rows, as trx.ui.widgets.List takes them
--   width         - the least width of the whole frame
--   reserve       - whether to keep room for every row the screen fits
--   row_pad       - the room on each side of a row's text
--   row_spacing   - the gap between two rows
--   arrows        - whether to show scroll arrows, true by default
--   footer        - a widget drawn under the frame
--   footer_height - the height the footer takes, for fitting the rows
function M.new(settings)
  settings.row_spacing = settings.row_spacing or M.row_spacing()
  ---@class common.ui.Requester
  ---@field rows trx.ui.ListRow[]
  ---@field list trx.ui.List
  ---@field title? trx.signal.Signal
  ---@field panel trx.ui.Widget
  ---@field root trx.ui.Widget
  ---@field place? fun(w: number, h: number): number, number
  local req = { rows = settings.rows or {} }

  -- How many rows fit, which follows the rows and the room the screen leaves.
  -- It is worked out again on every tick that the dialog reads input, and the
  -- list measures itself again only where the count changes.
  local visible = trx.signal.new(fit_rows(settings, #req.rows))
  local function refit()
    visible:set(fit_rows(settings, #req.rows))
  end

  req.list = trx.ui.widgets.List({
    rows = req.rows,
    visible = visible,
    reserve = settings.reserve,
    width = settings.width ~= nil
        and settings.width - 2 * (outer_pad() + body_pad_x())
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
        x = TITLE_PAD_X + body_pad_x(),
        y = title_pad_y(),
        child = trx.ui.widgets.Stack({
          align = trx.ui.HAlign.CENTER,
          children = { trx.ui.widgets.Label({ text = req.title }) },
        }),
      }),
    })
  end
  children[#children + 1] = trx.ui.widgets.Pad({
    x = body_pad_x(),
    y = BODY_PAD_Y,
    child = req.list,
  })

  local panel = trx.ui.widgets.Frame({
    style = trx.ui.FrameStyle.DIALOG,
    z = 170,
    child = trx.ui.widgets.Pad({
      x = outer_pad(),
      y = outer_pad(),
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
  req.root = trx.ui.widgets.Fit({ child = body })

  function req.set_rows(_, rows)
    req.rows = rows
    refit()
    req.list:set_rows(rows)
  end

  function req.selected_row()
    local index = req.list:selection()
    return index ~= nil and req.rows[index] or nil
  end

  -- Reads one tick of input. Returns the picked row's index, "cancel" where
  -- the player backed out, and nil where nothing was decided.
  function req.control(_, keys)
    refit()
    local picked = req.list:control(keys)
    if picked ~= nil then
      return picked
    end
    if keys:pressed(trx.input.Role.MENU_BACK) then
      return "cancel"
    end
    return nil
  end

  return req
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

-- Places a dialog across the safe area: centered from side to side, and the
-- given fraction of the way down the room that the margin leaves.
function M.anchored(anchor, margin)
  margin = margin or 0
  return function(w, h)
    local safe = trx.ui.safe_area
    return safe.x + (safe.width - w) / 2,
      safe.y + margin + (safe.height - 2 * margin - h) * anchor
  end
end

-- Places a dialog so that the middle of its frame rests on the line that a
-- standard page rests on: a reserved, titled list long enough to scroll, with
-- the given footer under it, the given fraction of the way down the safe area.
-- Every passport page rests on this line, whatever its own height.
function M.on_page_line(req, anchor, footer_height)
  return function(w)
    local spacing = M.row_spacing()
    local rows = fit_rows({
      title = "",
      row_spacing = spacing,
      footer_height = footer_height,
    }, MAX_VISIBLE_ROWS + 1)
    local height = chrome_height(true, true)
      + rows * TEXT_HEIGHT
      + (rows - 1) * spacing
    local line = (available_height() - height - 5) * anchor
      + (height - footer_height) / 2
    local _, panel_height = req.panel:measure()
    local safe = trx.ui.safe_area
    return safe.x + (safe.width - w) / 2,
      safe.y + line * trx.ui.text_scale - panel_height / 2
  end
end

return M
