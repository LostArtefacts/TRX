-- Examining an entry in the inventory ring: its description, a page at a time,
-- and turning it by hand once the description is out of the way.

local ring = require("common.ui.ring")
local text_dialog = require("common.ui.widgets.text_dialog")

local M = {}

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/actions/hide_dialog"] = "Hide dialog",
  ["general/actions/rotate"] = "Rotate",
})

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

-- The description, wrapped and cut into pages for the room the screen has.
local function prepare(state)
  state.dialog:set_text(
    state.title,
    state.description,
    trx.ui.canvas.width * 2.0 / 3.0,
    max_lines()
  )
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
    state.dialog:control(keys)
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
    title = object ~= nil and object.name or "",
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
  state.dialog = text_dialog.new({
    shown = state.shown,
    footer = trx.ui.widgets.Stack({
      align = trx.ui.HAlign.CENTER,
      children = {
        trx.ui.widgets.InputHint({
          input = trx.input.Role.MENU_SHOW_INFO,
          text = trx.locale.get("general/actions/hide_dialog"),
        }),
      },
    }),
  })
  prepare(state)
  -- The pages follow the room, which changes as the window does.
  listeners[#listeners + 1] = trx.ui.signals.layout:on(function()
    prepare(state)
  end)
  return ctx:push({ root = state.dialog.widget, on_input = on_input })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.EXAMINE, open)
end

return M
