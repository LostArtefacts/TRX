-- The pause screen: the game held still behind its backdrop, the word that
-- says it is paused, and the question it asks when the player presses the
-- inventory key: whether to leave for the title screen, and then whether they
-- are sure. Each question is a layer, so backing out of the second one shows
-- the first one as the player left it.

local accent = require("common.ui.accent")
local fade = require("common.ui.fade")
local metrics = require("common.ui.metrics")
local requester = require("common.ui.widgets.requester")

local M = {}

-- Fallback text for the pause screen's own strings. The strings files and
-- their translations take precedence.
trx.locale.declare({
  ["general/pause/exit_to_title"] = "Exit to title?",
  ["general/pause/continue"] = "Continue",
  ["general/pause/quit"] = "Quit",
  ["general/pause/are_you_sure"] = "Are you sure?",
  ["general/pause/yes"] = "Yes",
  ["general/pause/no"] = "No",
})

-- The pause question sits at the foot of the screen, inside a margin of fifty
-- canvas units.
local MARGIN = 50.0

-- How long the backdrop takes to come in and to go, and how long the screen
-- rests black before it leaves for the title screen.
local FADE_TICKS = 12
local EXIT_HOLD_TICKS = 3

local function question(title, first, second)
  local req = requester.new({
    title = accent.text(trx.locale.text(title)),
    rows = {
      { text = trx.locale.text(first) },
      { text = trx.locale.text(second) },
    },
    row_pad = metrics.CHOICE_ROW_PAD,
    -- The engine spaces the rows of this question alike in every game, TR1
    -- included, so the spacing does not follow the dialog metrics.
    row_spacing = 3.0,
    arrows = false,
    place = { y = 1, margin = MARGIN },
  })
  return req
end

-- Asks whether to leave for the title screen, and then whether the player is
-- sure. Backing out of the first question runs `on_back`. Resuming closes both
-- questions first. Leaving for the title screen keeps the second question on
-- screen until the pause screen ends, as the screen fades out.
local function ask(ctx, on_back, resume, exit)
  local shown = trx.signal.new(true)
  ---@type trx.ui.StackLayer?
  local asking = nil
  local first = question(
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
    local leaving = false
    ctx:push({
      root = sure.widget,
      place = sure.place,
      on_input = function(layer, keys)
        if leaving then
          return
        end
        local picked, cancelled = sure.list:control(keys)
        if picked == 1 then
          leaving = true
          exit()
          return
        end
        if picked == nil and not cancelled then
          return
        end
        layer:close()
        if picked == 2 then
          asking:close()
          resume()
        end
      end,
      on_close = function()
        shown:set(true)
      end,
    })
  end

  asking = ctx:push({
    root = trx.ui.widgets.Stack({
      visible = shown,
      children = { first.widget },
    }),
    place = first.place,
    on_input = function(layer, keys)
      local picked, cancelled = first.list:control(keys)
      if cancelled then
        layer:close()
        on_back()
      elseif picked == 1 then
        layer:close()
        resume()
      elseif picked == 2 then
        confirm()
      end
    end,
  })
end

-- Opens the pause screen.
---@param ctx trx.ui.PauseContext
function M.open(ctx)
  local style = trx.config.get("ui.pause_background_style")
  local fades = fade.screen(ctx, {
    enabled = function()
      return trx.config.get("ui.pause_fade_effects") and style ~= "none"
    end,
  })
  local is_waiting = false
  local is_leaving = false

  local function wait()
    is_waiting = true
    trx.overlay.set_caption(
      accent.text(trx.locale.text("general/pause/paused"), accent.Role.NAME)
    )
  end

  local backdrop = fades:enter({
    layer = { backdrop = style },
    from = 0,
    to = 1,
    ticks = FADE_TICKS,
    on_done = function()
      if not is_leaving then
        wait()
      end
    end,
  })

  -- Starts the screen leaving, and returns whether it was not leaving yet.
  local function start_leaving()
    if is_leaving then
      return false
    end
    is_leaving = true
    is_waiting = false
    return true
  end

  local function resume()
    trx.overlay.set_caption()
    if not start_leaving() then
      return
    end
    fades:move(backdrop, 0, FADE_TICKS, 0, function()
      ctx:resume()
    end)
  end

  -- Fades to black over the backdrop, as a pattern or a tint does not end in
  -- black. The caption stays until the screen ends, because the question
  -- stands on the room above it and would move down as the caption went.
  local function exit()
    if not start_leaving() then
      return
    end
    fades:leave({
      ticks = FADE_TICKS,
      hold = EXIT_HOLD_TICKS,
      on_done = function()
        ctx:exit_to_title()
      end,
    })
  end

  ctx:on_end(function()
    trx.overlay.set_caption()
  end)

  return ctx:push({
    root = trx.ui.widgets.Spacer(),
    on_input = function(_, keys)
      if is_leaving then
        return
      end
      local pause = keys:pressed(trx.input.Role.PAUSE)
      local option = keys:pressed(trx.input.Role.OPTION)
      if not is_waiting then
        if pause then
          resume()
        end
      elseif not trx.game.is_playable then
        if pause or option then
          resume()
        end
      elseif pause then
        resume()
      elseif option then
        is_waiting = false
        ask(ctx, function()
          is_waiting = true
        end, resume, exit)
      end
    end,
  })
end

return M
