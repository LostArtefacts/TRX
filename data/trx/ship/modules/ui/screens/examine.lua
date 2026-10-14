-- Examining an entry in the inventory ring: its description, a page at a time,
-- and turning it by hand once the description is out of the way.

local metrics = require("common.ui.metrics")
local ring = require("common.ui.ring")

local M = {}

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

local function label(text, shown)
  return trx.ui.widgets.Label({ text = text, shown = shown })
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
      if index ~= state.page:get() then
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
  local count = #state.pages
  local has_left = state.page:map(function(page)
    return page > 1
  end)
  local has_right = state.page:map(function(page)
    return page < count
  end)
  local text = state.page:map(function(page)
    return trx.locale.format("general/misc/pagination_nav", page, count)
  end)
  return trx.ui.widgets.Stack({
    orientation = trx.ui.Orientation.HORIZONTAL,
    children = {
      label("\\{button left} ", has_left),
      label(text),
      label(" \\{button right}", has_right),
    },
  })
end

local function dialog(state)
  local body = {
    aligned(trx.ui.HAlign.CENTER, label(state.title)),
    trx.ui.widgets.Spacer({ w = TITLE_MARGIN, h = TITLE_MARGIN }),
  }
  for i = 1, #state.pages do
    body[#body + 1] = page_widget(state, i)
  end
  if #state.pages > 1 then
    body[#body + 1] =
      trx.ui.widgets.Spacer({ w = TITLE_MARGIN, h = TITLE_MARGIN * 3 })
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
          text = trx.locale.get("general/actions/hide_dialog"),
        })
      ),
    },
  })
end

-- The description, wrapped and cut into pages for the room the screen has.
-- The player stays on the page that they read, or the last one.
local function prepare(state)
  local width = math.floor(trx.ui.canvas.width * 2.0 / 3.0)
  local wrapped = trx.ui.primitive.wrap_text(state.description, width)
  state.pages = trx.strings.paginate(wrapped, max_lines())
  state.lines = 0
  for _, page in ipairs(state.pages) do
    local _, breaks = page:gsub("\n", "")
    state.lines = math.max(state.lines, breaks + 1)
  end
  if state.page == nil then
    state.page = trx.signal.new(1)
  else
    state.page:set(math.min(state.page:get(), #state.pages))
  end
end

-- Offers to turn the entry where the ring shows its hints, while the
-- description is out of the way.
local function offer_turning(offer)
  trx.inventory_ring.set_input_hints(offer and {
    {
      input = {
        trx.input.Role.FORWARD,
        trx.input.Role.BACK,
        trx.input.Role.LEFT,
        trx.input.Role.RIGHT,
      },
      text = trx.locale.get("general/actions/rotate"),
    },
  } or nil)
end

local function control(ctx, state, keys)
  local show_text = state.shown:get()

  if show_text then
    local step = keys:pressed(trx.input.Role.MENU_LEFT) and -1
      or keys:pressed(trx.input.Role.MENU_RIGHT) and 1
      or 0
    local page = state.page:get() + step
    if step ~= 0 and page >= 1 and page <= #state.pages then
      state.page:set(page)
      trx.sound.play(trx.catalog.samples.MENU_PASSPORT)
    end
  end

  if keys:pressed(trx.input.Role.MENU_SHOW_INFO) then
    if show_text then
      state.hidden:set(true)
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

---@param ctx trx.ui.EntryContext
local function open(ctx)
  local object = trx.objects[ctx.object]
  local description = object ~= nil and object.description or nil
  local has_text = description ~= nil and description ~= ""
  local resting = ring.resting()
  local hidden = trx.signal.new(false)
  local state = {
    title = trx.strings.upper(object ~= nil and object.name or ""),
    description = description,
    hidden = hidden,
    shown = has_text and resting & ~hidden or trx.signal.new(false),
  }
  -- The hints offer turning the entry once it rests with the description out
  -- of the way, and take the offer back while the description shows.
  local offers = trx.signal.combine(
    resting,
    state.shown,
    function(rests, shown)
      if not rests then
        return nil
      end
      return not shown
    end
  )
  if offers:get() ~= nil then
    offer_turning(offers:get())
  end
  local listeners = { offers:on(offer_turning) }
  ctx:on_end(function()
    for _, listener in ipairs(listeners) do
      listener:detach()
    end
    offers:stop()
    state.shown:stop()
    resting:stop()
  end)

  local on_input = ring.while_resting(function(_, keys)
    control(ctx, state, keys)
  end)
  if not has_text then
    return ctx:push({ root = trx.ui.widgets.Spacer(), on_input = on_input })
  end
  prepare(state)
  local layer = ctx:push({ root = dialog(state), on_input = on_input })
  -- The pages follow the room, which changes as the window does.
  listeners[#listeners + 1] = trx.ui.signals.layout:on(function()
    prepare(state)
    layer:set_root(dialog(state))
  end)
  return layer
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.EXAMINE, open)
end

return M
