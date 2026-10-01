-- The question the pause screen asks when the player presses the inventory
-- key: whether to leave for the title screen, and then whether they are sure.
-- Each question is a layer, so backing out of the second one shows the first
-- one as the player left it.
--
-- Usage:
--   require("common.pause").setup()

local requester = require("common.ui.requester")

local M = {}

-- The engine's pause dialog sits at the foot of the screen, inside a margin
-- of fifty canvas units.
local MARGIN = 50.0

local function question(title, first, second)
  local req = requester.new({
    title = trx.locale.get(title),
    rows = {
      { text = trx.locale.get(first) },
      { text = trx.locale.get(second) },
    },
    row_pad = 20.0,
    row_spacing = 3.0,
    arrows = false,
  })
  req.place = requester.anchored(1.0, MARGIN)
  return req
end

local function open(ctx)
  local shown = trx.signal.new(true)
  local ask = question(
    "general/pause/exit_to_title",
    "general/pause/continue",
    "general/pause/quit"
  )

  local function confirm()
    local sure = question(
      "general/pause/are_you_sure",
      "general/pause/yes",
      "general/pause/no"
    )
    shown:set(false)
    ctx:push({
      root = sure.root,
      place = sure.place,
      on_input = function(layer, keys)
        local picked = sure:control(keys)
        if picked == "cancel" then
          layer:close()
        elseif picked == 1 then
          ctx:exit_to_title()
        elseif picked == 2 then
          ctx:resume()
        end
      end,
      on_close = function()
        shown:set(true)
      end,
    })
  end

  return ctx:push({
    root = trx.ui.widgets.Stack({ shown = shown, children = { ask.root } }),
    place = ask.place,
    on_input = function(_, keys)
      local picked = ask:control(keys)
      if picked == "cancel" then
        ctx:cancel()
      elseif picked == 1 then
        ctx:resume()
      elseif picked == 2 then
        confirm()
      end
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.PAUSE, open)
end

return M
