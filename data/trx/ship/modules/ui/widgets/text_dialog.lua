-- A framed dialog that shows a title over a text, a page at a time, with the
-- page it is on in the corner where the text runs to several pages.
--
-- Usage:
--   local text_dialog = require("common.ui.widgets.text_dialog")
--   local dialog = text_dialog.new({ heavy = true })
--   dialog:set_text("Title", "A long text...", 200, 8)
--   ctx:push({ root = dialog.widget, on_input = function(_, keys)
--     dialog:control(keys)
--   end })

local metrics = require("common.ui.metrics")

local M = {}

local DIALOG_PAD = 8
local TITLE_MARGIN = 5
local FOOTER_SPACING = 5

---@class common.ui.TextDialog
---@field widget trx.ui.Widget The dialog.
---@field title trx.signal.Signal The title, in capitals.
---@field pages trx.signal.Signal The pages of the text.
---@field page trx.signal.Signal The page that shows, counted from 1.
---@field private lines integer The most lines a page holds.
---@field private text? string The text the pages were made from.
local TextDialog = {}
TextDialog.__index = TextDialog

-- Wraps a text to a width and cuts it into pages. The dialog stays on the page
-- that it showed where the text is the one it already holds.
---@param title string
---@param text string
---@param width number The width to wrap to, in canvas units.
---@param max_lines integer The most lines a page holds.
function TextDialog:set_text(title, text, width, max_lines)
  self.title:set(trx.strings.upper(title))
  local wrapped = trx.ui.primitive.wrap_text(text, math.floor(width))
  local pages = trx.strings.paginate(wrapped, max_lines)
  local lines = 0
  for _, page in ipairs(pages) do
    local _, breaks = page:gsub("\n", "")
    lines = math.max(lines, breaks + 1)
  end
  self.lines = lines
  local page = text == self.text and math.min(self.page:get(), #pages) or 1
  self.text = text
  self.pages:set(pages)
  self.page:set(page)
end

-- Turns the page for the left and right keys, with the sound of a page.
---@param keys trx.ui.LayerKeys
function TextDialog:control(keys)
  local step = keys:pressed(trx.input.Role.MENU_LEFT) and -1
    or keys:pressed(trx.input.Role.MENU_RIGHT) and 1
    or 0
  local page = self.page:get() + step
  if step ~= 0 and page >= 1 and page <= #self.pages:get() then
    self.page:set(page)
    trx.sound.play(trx.catalog.samples.MENU_PASSPORT)
  end
end

-- The text, as wide as its widest page and as tall as its longest one, so
-- that the dialog keeps one size whichever page shows.
local function text_widget(self)
  return trx.ui.widgets.Custom({
    measure = function()
      local pages = self.pages:get()
      local current = pages[self.page:get()] or ""
      local w = 0.0
      for _, page in ipairs(pages) do
        w = math.max(w, (trx.ui.primitive.measure_text(page, 1.0)))
      end
      if #pages > 1 then
        return w, metrics.TEXT_HEIGHT * self.lines * trx.ui.text_scale
      end
      local _, h = trx.ui.primitive.measure_text(current, 1.0)
      return w, h
    end,
    paint = function(x, y)
      local current = self.pages:get()[self.page:get()]
      if current ~= nil then
        trx.ui.primitive.text(current, x, y, 1.0, 0)
      end
    end,
  })
end

local function page_indicator(self)
  local count = self.pages:map(function(pages)
    return #pages
  end)
  local has_left = self.page:map(function(page)
    return page > 1
  end)
  local has_right = trx.signal.combine(self.page, count, function(page, n)
    return page < n
  end)
  local text = trx.signal.combine(self.page, count, function(page, n)
    return trx.locale.format("general/misc/pagination_nav", page, n)
  end)
  return trx.ui.widgets.Stack({
    align = trx.ui.HAlign.RIGHT,
    visible = count:map(function(n)
      return n > 1
    end),
    children = {
      trx.ui.widgets.Spacer({ w = TITLE_MARGIN, h = TITLE_MARGIN * 3 }),
      trx.ui.widgets.Stack({
        orientation = trx.ui.Orientation.HORIZONTAL,
        children = {
          trx.ui.widgets.Label({
            text = "\\{button left} ",
            visible = has_left,
          }),
          trx.ui.widgets.Label({ text = text }),
          trx.ui.widgets.Label({
            text = " \\{button right}",
            visible = has_right,
          }),
        },
      }),
    },
  })
end

---@class common.ui.TextDialog.frame.settings
---@field title any The title, or a signal carrying it.
---@field body trx.ui.Widget[] What goes under the title, in order.
---@field heavy? boolean Whether to draw the frame solid.
---@field spacing? number The gap between the parts. `0` by default.

-- A frame with a title centered over what it holds, as a text dialog and the
-- color editor draw.
---@param settings common.ui.TextDialog.frame.settings
---@return trx.ui.Widget
function M.frame(settings)
  local children = {
    trx.ui.widgets.Stack({
      align = trx.ui.HAlign.CENTER,
      children = { trx.ui.widgets.Label({ text = settings.title }) },
    }),
    trx.ui.widgets.Spacer({ w = TITLE_MARGIN, h = TITLE_MARGIN }),
  }
  for _, widget in ipairs(settings.body) do
    children[#children + 1] = widget
  end
  return trx.ui.widgets.Frame({
    style = settings.heavy and trx.ui.FrameStyle.DIALOG_HEAVY
      or trx.ui.FrameStyle.DIALOG,
    z = 160,
    child = trx.ui.widgets.Pad({
      x = DIALOG_PAD,
      y = DIALOG_PAD,
      child = trx.ui.widgets.Stack({
        spacing = settings.spacing,
        align = trx.ui.HAlign.SPAN,
        children = children,
      }),
    }),
  })
end

---@class common.ui.TextDialog.settings
---@field heavy? boolean Whether to draw the frame solid.
---@field footer? trx.ui.Widget A widget under the frame.
---@field visible? any Whether the dialog is visible, or a signal that holds it.

-- Creates a text dialog with no text. `set_text` gives it one.
---@param settings? common.ui.TextDialog.settings
---@return common.ui.TextDialog
function M.new(settings)
  settings = settings or {}
  local self = setmetatable({
    title = trx.signal.new(""),
    pages = trx.signal.new({}),
    page = trx.signal.new(1),
    lines = 0,
  }, TextDialog)

  local frame = M.frame({
    title = self.title,
    heavy = settings.heavy,
    body = { text_widget(self), page_indicator(self) },
  })

  self.widget = trx.ui.widgets.Stack({
    spacing = FOOTER_SPACING,
    align = trx.ui.HAlign.SPAN,
    visible = settings.visible,
    children = { frame, settings.footer },
  })
  return self
end

return M
