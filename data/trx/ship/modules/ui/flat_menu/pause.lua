-- The pause screen of the flat menu system, as the original TR4 draws it: the
-- word that says the game is paused, and under it the statistics, the options
-- and the way back to the title screen. The options list the settings
-- dialogs. The statistics and each dialog show alone, in place of the list.

local fade = require("common.ui.fade")
local list = require("common.ui.flat_menu.list")
local options = require("common.ui.flat_menu.options")
local sound = require("common.ui.flat_menu.sound").play
local stats = require("common.ui.flat_menu.stats")

local M = {}

trx.locale.declare({
  ["general/flat_menu/statistics"] = "Statistics",
  ["general/flat_menu/exit_to_title"] = "Exit to Title",
})

local Role = trx.input.Role

-- How long the backdrop takes to come in and to go, and how long the screen
-- rests black before it leaves for the title screen.
local FADE_TICKS = 12
local EXIT_HOLD_TICKS = 3

-- Opens the pause screen.
---@param ctx trx.ui.PauseContext
---@return trx.ui.StackLayer
function M.open(ctx)
  local style = trx.config.get("ui.pause_background_style")
  local fades = fade.screen(ctx, {
    enabled = function()
      return trx.config.get("ui.pause_fade_effects") and style ~= "none"
    end,
  })
  local backdrop = fades:enter({
    layer = { backdrop = style },
    from = 0,
    to = 1,
    ticks = FADE_TICKS,
  })

  local state = {
    ---@type common.ui.flat_menu.List[]
    pages = {},
    -- Whether the statistics or a dialog shows in place of the list.
    is_away = false,
    is_leaving = false,
  }

  ---@return common.ui.flat_menu.List
  local function page()
    return state.pages[#state.pages]
  end

  -- Starts the screen leaving, and returns whether it was not leaving yet.
  local function start_leaving()
    if state.is_leaving then
      return false
    end
    state.is_leaving = true
    return true
  end

  local function resume()
    if not start_leaving() then
      return
    end
    fades:move(backdrop, 0, FADE_TICKS, 0, function()
      ctx:resume()
    end)
  end

  -- Fades to black over the backdrop, as a pattern or a tint does not end in
  -- black.
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

  -- Shows what `push` opens in place of the list, until it closes.
  local function go_away(push)
    local layer = push(ctx, function()
      state.is_away = false
    end)
    state.is_away = layer ~= nil
  end

  -- The statistics show only for a level that keeps them, and the way back
  -- to the title screen only where the screen can take it.
  local function main()
    local rows = {}
    local level = trx.game.current_level
    if level ~= nil and level.stats ~= nil then
      rows[#rows + 1] = {
        text = trx.locale.text("general/flat_menu/statistics"),
        choose = function()
          go_away(stats.push)
        end,
      }
    end
    rows[#rows + 1] = {
      text = options.title(),
      choose = function()
        state.pages[#state.pages + 1] = options.page(go_away)
      end,
    }
    if ctx.can_exit_to_title then
      rows[#rows + 1] = {
        text = trx.locale.text("general/flat_menu/exit_to_title"),
        choose = exit,
      }
    end
    return { heading = trx.locale.text("general/pause/paused"), rows = rows }
  end
  state.pages[1] = list.new(main)
  list.follow_settings(ctx, state.pages)

  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = function()
        if not state.is_away and not state.is_leaving then
          page():paint()
        end
      end,
    }),
    place = function()
      return 0, 0
    end,
    on_input = function(_, keys)
      if state.is_leaving or state.is_away then
        return
      end
      if keys:pressed(Role.PAUSE) then
        resume()
        return
      end
      local current = page()
      if keys:pressed(Role.MENU_BACK) or keys:pressed(Role.OPTION) then
        sound("select")
        if #state.pages > 1 then
          table.remove(state.pages)
        else
          resume()
        end
      elseif current:move(keys) then
        sound("move")
      elseif keys:pressed(Role.MENU_CONFIRM) then
        local row = current:picked()
        if row ~= nil and row.choose ~= nil then
          sound("select")
          row.choose()
        end
      end
    end,
  })
end

return M
