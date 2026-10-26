-- The menu of the flat menu system once Lara has died, as the original TR4
-- prints it: GAME OVER, and under it Load Game and Exit to Title. TRX adds
-- Restart Level where the level allows it. Where the game offers none of
-- these, the screen shows GAME OVER alone and leaves for the title screen on
-- its own.

local accent = require("common.ui.accent")
local fade = require("common.ui.fade")
local list = require("common.ui.flat_menu.list")
local pages = require("common.ui.screens.passport.pages")
local saves = require("common.ui.flat_menu.saves")
local sound = require("common.ui.flat_menu.sound").play

local M = {}

local Role = trx.input.Role

-- GAME OVER stands two lines above the first row.
local GAME_OVER_GAP = 2

-- How long the backdrop takes to come in, and how long the screen takes to go
-- to black as it leaves.
local FADE_TICKS = 12
local BLACK_TICKS = 8
local BLACK_HOLD_TICKS = 1

-- How long GAME OVER shows alone before the screen leaves, and how long it
-- shows before a key makes it leave sooner, as the original counts them.
local ALONE_TICKS = 300
local ALONE_SKIP_TICKS = 150

-- Opens the menu.
---@param ctx trx.ui.MenuContext
---@return trx.ui.StackLayer
function M.open(ctx)
  local state = {
    -- Whether the list of saves shows in place of the menu.
    is_away = false,
    is_leaving = false,
    ticks = 0,
  }

  -- The inventory fade effects setting governs only the backdrop; the fade to
  -- black follows the fade effects setting alone, as for the ring.
  local fades = fade.screen(ctx)
  local style = trx.config.get("ui.inventory_background_style")
  local backdrop = trx.ui.tween(0):to(
    1,
    fade.enabled("ui.inventory_fade_effects") and FADE_TICKS or 0
  )

  -- Goes to black and ends the menu, once the game knows where it goes next.
  local function leave()
    state.is_leaving = true
    fades:leave({
      ticks = BLACK_TICKS,
      hold = BLACK_HOLD_TICKS,
      on_done = function()
        if ctx.is_held then
          ctx:confirm()
        end
      end,
    })
  end

  local function load_game()
    if not pages.has_saves() then
      sound("no")
      return
    end
    state.is_away = true
    saves.push(ctx, saves.Kind.LOAD, function(done)
      if done then
        leave()
      else
        state.is_away = false
      end
    end, true)
  end

  local function build()
    local rows = {}
    if pages.saving_enabled() then
      rows[#rows + 1] = {
        text = trx.locale.text("general/passport/load_game"),
        choose = load_game,
      }
    end
    if trx.game.can_restart_level() then
      rows[#rows + 1] = {
        text = trx.locale.text("general/passport/restart_level"),
        choose = function()
          trx.game.restart_level()
          leave()
        end,
      }
    end
    -- The original offers the way back to the title screen only next to the
    -- saves; alone, GAME OVER takes the player there by itself.
    if #rows > 0 then
      rows[#rows + 1] = {
        text = trx.locale.text("general/passport/exit_to_title"),
        choose = function()
          trx.game.exit_to_title()
          leave()
        end,
      }
    end
    return {
      heading = trx.locale.text("general/inventory_ring/heading_game_over"),
      heading_role = accent.Role.MENU_WARNING,
      heading_gap = GAME_OVER_GAP,
      rows = rows,
    }
  end
  local menu = list.new(build)
  local alone = #menu.rows == 0
  list.follow_settings(ctx, { menu })

  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = function()
        trx.ui.primitive.backdrop(style, backdrop:get())
        if not state.is_away then
          menu:paint()
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
      if alone then
        state.ticks = state.ticks + 1
        if
          state.ticks > ALONE_TICKS
          or (state.ticks > ALONE_SKIP_TICKS and trx.input.is_anything_held())
        then
          trx.game.exit_to_title()
          leave()
        end
        return
      end
      if menu:move(keys) then
        sound("select")
      elseif keys:pressed(Role.MENU_CONFIRM) then
        local row = menu:picked()
        if row ~= nil and row.choose ~= nil then
          sound("choose")
          row.choose()
        end
      end
    end,
  })
end

return M
