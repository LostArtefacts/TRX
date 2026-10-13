-- Examining an entry in the inventory ring: its description, a page at a time,
-- and turning it by hand once the description is out of the way.
--
-- Usage:
--   require("common.ui.examine").setup()

local metrics = require("common.ui.metrics")

local M = {}

local L = trx.locale.get

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/actions/hide_dialog"] = "Hide dialog",
  ["general/actions/rotate"] = "Rotate",
})

local DIALOG_PAD = 8.0
local TITLE_MARGIN = 5.0
local FOOTER_SPACING = 5.0

local function turn_speed()
  return trx.math.degrees(3)
end

-- A short screen holds fewer lines a page.
local function max_lines()
  local height = trx.ui.canvas.height / trx.ui.text_scale
  if height <= 240 then
    return 5
  elseif height <= 384 then
    return 7
  end
  return 12
end

local function spacer(w, h)
  return trx.ui.widgets.Custom({
    measure = function()
      local scale = trx.ui.text_scale
      return w * scale, h * scale
    end,
    paint = function() end,
  })
end

local function label(text)
  return trx.ui.widgets.Label({ text = text })
end

local function aligned(align, child)
  return trx.ui.widgets.Stack({ align = align, children = { child } })
end

-- One page of the description. Every page takes its own width, so the box
-- keeps one width whichever page shows, but only the page that shows takes
-- room or draws.
local function page_widget(state, index)
  return trx.ui.widgets.Custom({
    measure = function()
      local w, h = trx.ui.primitive.measure_text(state.pages[index], 1.0)
      if index ~= state.page then
        return w, 0
      elseif #state.pages > 1 then
        return w, metrics.TEXT_HEIGHT * state.lines * trx.ui.text_scale
      end
      return w, h
    end,
    paint = function(x, y, _, h)
      if h > 0 then
        trx.ui.primitive.text(state.pages[index], x, y, 1.0, 0)
      end
    end,
  })
end

local function page_indicator(state)
  local children = {}
  if state.page > 1 then
    children[#children + 1] = label("\\{button left} ")
  end
  children[#children + 1] = label(
    trx.locale.format("general/misc/pagination_nav", state.page, #state.pages)
  )
  if state.page < #state.pages then
    children[#children + 1] = label(" \\{button right}")
  end
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    children = children,
  })
end

local function dialog(state)
  local body = {
    aligned(trx.ui.HAlign.CENTER, label(state.title)),
    spacer(TITLE_MARGIN, TITLE_MARGIN),
  }
  for i = 1, #state.pages do
    body[#body + 1] = page_widget(state, i)
  end
  if #state.pages > 1 then
    body[#body + 1] = spacer(TITLE_MARGIN, TITLE_MARGIN * 3)
    body[#body + 1] = aligned(trx.ui.HAlign.RIGHT, page_indicator(state))
  end

  return trx.ui.widgets.Stack({
    spacing = FOOTER_SPACING,
    align = trx.ui.HAlign.SPAN,
    shown = state.shown,
    children = {
      trx.ui.widgets.Frame({
        style = trx.ui.FrameStyle.DIALOG,
        z = 160,
        child = trx.ui.widgets.Pad({
          x = DIALOG_PAD,
          y = DIALOG_PAD,
          child = trx.ui.widgets.Stack({
            align = trx.ui.HAlign.SPAN,
            children = body,
          }),
        }),
      }),
      aligned(
        trx.ui.HAlign.CENTER,
        trx.ui.widgets.InputHint({
          input = trx.input.Role.MENU_SHOW_INFO,
          text = L("general/actions/hide_dialog"),
        })
      ),
    },
  })
end

-- Whether the entry has come to rest, which is when the screen starts.
local function is_open()
  local anim = trx.inventory_ring.selection_anim()
  return anim ~= nil and anim.frame == anim.goal_frame
end

-- The description, wrapped and cut into pages the first time it shows.
local function prepare(state)
  local width = math.floor(trx.ui.canvas.width * 2.0 / 3.0)
  local wrapped = trx.ui.primitive.wrap_text(state.description, width)
  state.pages = trx.strings.paginate(wrapped, max_lines())
  state.lines = 0
  for _, page in ipairs(state.pages) do
    local _, breaks = page:gsub("\n", "")
    state.lines = math.max(state.lines, breaks + 1)
  end
  state.page = 1
end

-- Offers to turn the entry where the ring shows its hints, while the
-- description is out of the way.
local function offer_turning(state, offer)
  if offer == state.offers_turning then
    return
  end
  state.offers_turning = offer
  local Role = trx.input.Role
  trx.inventory_ring.set_input_hints(offer and {
    {
      input = { Role.FORWARD, Role.BACK, Role.LEFT, Role.RIGHT },
      text = L("general/actions/rotate"),
    },
  } or nil)
end

local function control(ctx, state, layer, keys)
  if not is_open() then
    return
  end
  if state.pages == nil and state.has_text then
    prepare(state)
    layer:set_root(dialog(state))
  end

  local show_text = state.has_text and not state.hidden
  state.shown:set(show_text)
  offer_turning(state, not show_text)

  if show_text then
    local step = keys:pressed(trx.input.Role.MENU_LEFT) and -1
      or keys:pressed(trx.input.Role.MENU_RIGHT) and 1
      or 0
    local page = state.page + step
    if step ~= 0 and page >= 1 and page <= #state.pages then
      state.page = page
      layer:set_root(dialog(state))
      trx.sound.play(trx.catalog.samples.MENU_PASSPORT)
    end
  end

  if keys:pressed(trx.input.Role.MENU_SHOW_INFO) then
    if show_text then
      state.hidden = true
      return
    end
    ctx:cancel()
    return
  end
  if
    keys:pressed(trx.input.Role.MENU_BACK)
    or keys:pressed(trx.input.Role.MENU_CONFIRM)
  then
    ctx:cancel()
    return
  end

  if not show_text then
    local function axis(minus, plus)
      return (keys:held(minus) and 1 or 0) - (keys:held(plus) and 1 or 0)
    end
    trx.inventory_ring.turn_selection(
      axis(trx.input.Role.MENU_LEFT, trx.input.Role.MENU_RIGHT) * turn_speed(),
      axis(trx.input.Role.MENU_DOWN, trx.input.Role.MENU_UP) * turn_speed()
    )
  end
end

local function open(ctx)
  local object = trx.objects[ctx.object]
  local description = object ~= nil and object.description or nil
  local state = {
    title = trx.strings.upper(object ~= nil and object.name or ""),
    description = description,
    has_text = description ~= nil and description ~= "",
    hidden = false,
    shown = trx.signal.new(false),
  }
  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return 0, 0
      end,
      paint = function() end,
    }),
    on_input = function(layer, keys)
      control(ctx, state, layer, keys)
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.EXAMINE, open)
end

return M
