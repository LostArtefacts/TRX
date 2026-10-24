-- The title menu of the flat menu system, as the original TR4 prints it under
-- the logo of the title screen: New Game, Load Game, Options and Exit. The
-- choices that TRX adds to a new game, the level select and the game modes
-- among them, open as lists of their own. The title plays a demo once nobody
-- has touched it for a while.

local fade = require("common.ui.fade")
local list = require("common.ui.flat_menu.list")
local options = require("common.ui.flat_menu.options")
local pages = require("common.ui.screens.passport.pages")
local rows = require("common.ui.screens.passport.rows")
local saves = require("common.ui.flat_menu.saves")
local sound = require("common.ui.flat_menu.sound").play
local title_page = require("common.ui.title_page")

local M = {}

trx.locale.declare({
  ["general/flat_menu/exit"] = "Exit",
  ["general/flat_menu/gym"] = "Lara's Home",
})

local Role = trx.input.Role

-- How long the title takes to come in from black, and to go to black as it
-- leaves.
local FADE_TICKS = 16
local BLACK_TICKS = 8
local BLACK_HOLD_TICKS = 1

-- How many ticks the title waits for the player before it plays a demo, or
-- 0 where it plays none.
local function demo_ticks()
  if not trx.config.get("gameplay.enable_demo") or #trx.game.demos == 0 then
    return 0
  end
  return math.floor(trx.config.get("flow.demo_delay") * trx.game.LOGIC_FPS)
end

-- Opens the title menu.
---@param ctx trx.ui.MenuContext
---@return trx.ui.StackLayer
function M.open(ctx)
  local state = {
    ---@type common.ui.flat_menu.List[]
    pages = {},
    -- Whether a list of saves or a dialog shows in place of the menu.
    is_away = false,
    is_leaving = false,
    is_hidden = true,
    idle_ticks = 0,
    demo_ticks = demo_ticks(),
  }

  -- The title comes in from black and goes to black whatever the inventory
  -- fade effects setting says, as the ring's title does.
  local fades = fade.screen(ctx)
  fades:enter({
    ticks = FADE_TICKS,
    on_done = function()
      state.is_hidden = false
    end,
  })
  trx.overlay.show_version(true)
  ctx:on_end(function()
    trx.overlay.show_version(false)
    title_page.shows_main:set(true)
  end)

  ---@return common.ui.flat_menu.List
  local function page()
    return state.pages[#state.pages]
  end

  local function follow_main()
    title_page.shows_main:set(#state.pages <= 1 and not state.is_away)
  end

  local function push(next_page)
    state.pages[#state.pages + 1] = next_page
    follow_main()
  end

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

  -- Shows what `open` pushes in place of the menu, until it closes.
  local function go_away(open)
    local layer = open(ctx, function()
      state.is_away = false
      follow_main()
    end)
    state.is_away = layer ~= nil
    follow_main()
  end

  -- Makes a list from `build`, which returns the heading and the rows.
  local function list_of(build)
    return list.new(function()
      local heading, entries = build()
      return { heading = heading, rows = entries }
    end)
  end

  -- Starts a level from the level select, asking for the game mode first
  -- where there is more than one.
  local function start_level(level)
    if not pages.game_modes_available() then
      trx.game.play_level(level.num, { select = true, ng_plus = false })
      leave()
      return
    end
    push(list_of(function()
      local entries = {}
      for _, row in ipairs(rows.mode_rows()) do
        entries[#entries + 1] = {
          text = row.text,
          choose = function()
            trx.game.play_level(level.num, {
              select = true,
              ng_plus = row.choice == "new_game_plus",
            })
            leave()
          end,
        }
      end
      return trx.locale.get("general/passport/select_mode"), entries
    end))
  end

  -- Lists the levels that `level_rows` returns, and runs `on_level` with the
  -- one the player picks.
  local function level_list(level_rows, on_level)
    return list_of(function()
      local entries = {}
      for _, row in ipairs(level_rows()) do
        entries[#entries + 1] = {
          text = row.text,
          choose = function()
            on_level(row.level)
          end,
        }
      end
      return trx.locale.get("general/passport/select_level"), entries
    end)
  end

  -- Lists the saves that pass `filter`, and runs `on_slot` with the one the
  -- player picks. The list stays open where `on_slot` returns false.
  local function slot_list(filter, on_slot)
    go_away(function(_, on_close)
      return saves.pick(ctx, {
        heading = trx.locale.get("general/passport/select_save"),
        filter = filter,
        on_pick = function(slot)
          if not on_slot(slot) then
            return false
          end
          on_close()
          return true
        end,
        on_back = on_close,
      })
    end)
  end

  local function play_previous()
    slot_list(function(info)
      return info ~= nil and info.can_select_level
    end, function(slot)
      local reached = trx.savegame.reached_levels(slot.slot_num, slot.pool)
      local function level_rows()
        return reached and rows.reached_level_rows(reached) or {}
      end
      if #level_rows() == 0 then
        return false
      end
      local levels = level_list(level_rows, function(level)
        trx.game.play_level(level.num, {
          select = true,
          from_save = { slot_num = slot.slot_num, pool = slot.pool },
        })
        leave()
      end)
      levels.on_back = play_previous
      push(levels)
      return true
    end)
  end

  local function story()
    slot_list(function(info)
      return info ~= nil and info.has_story
    end, function(slot)
      trx.savegame.play_story(slot.slot_num, slot.pool)
      leave()
      return true
    end)
  end

  local function switch_mod()
    local current = 1
    local mods = list_of(function()
      local entries = {}
      for _, mod in ipairs(pages.switchable_mods()) do
        local key = "dynamic/mods/" .. mod.name .. "/title"
        local title = trx.locale.get(key)
        if title == key then
          title = mod.title or mod.name
        end
        entries[#entries + 1] = {
          text = title,
          choose = function()
            trx.mod.switch(mod)
            leave()
          end,
        }
        if trx.mod.current ~= nil and mod.name == trx.mod.current.name then
          current = #entries
        end
      end
      return trx.locale.get("general/passport/select_mod"), entries
    end)
    mods.selected = current
    mods:follow()
    push(mods)
  end

  -- What each choice of a new game does.
  local NEW_GAME = {
    new_game = function()
      trx.game.start_new_game()
      leave()
    end,
    new_game_plus = function()
      trx.game.start_new_game({ ng_plus = true })
      leave()
    end,
    play_prev = play_previous,
    story = story,
    switch_mod = switch_mod,
  }

  local function new_game()
    if trx.config.get("flow.play_any_level") then
      push(level_list(rows.all_level_rows, start_level))
      return
    end
    local choices = rows.new_game_rows()
    if #choices == 1 then
      NEW_GAME[choices[1].choice]()
      return
    end
    push(list_of(function()
      local entries = {}
      for _, row in ipairs(rows.new_game_rows()) do
        entries[#entries + 1] = {
          text = row.text,
          choose = NEW_GAME[row.choice],
        }
      end
      return trx.locale.get("general/passport/select_mode"), entries
    end))
  end

  -- The list opens with no saves too, as the original shows its empty slots.
  local function load_game()
    go_away(function(_, on_close)
      return saves.push(ctx, saves.Kind.LOAD, function(done)
        on_close()
        if done then
          leave()
        end
      end)
    end)
  end

  push(list_of(function()
    local main = {
      {
        text = trx.locale.get("general/passport/new_game"),
        choose = new_game,
      },
    }
    if pages.saving_enabled() then
      main[#main + 1] = {
        text = trx.locale.get("general/passport/load_game"),
        choose = load_game,
      }
    end
    if trx.game.gym ~= nil then
      main[#main + 1] = {
        text = trx.locale.get("general/flat_menu/gym"),
        choose = function()
          trx.game.play_gym()
          leave()
        end,
      }
    end
    main[#main + 1] = {
      text = options.title(),
      choose = function()
        push(options.page(go_away))
      end,
    }
    main[#main + 1] = {
      text = trx.locale.get("general/flat_menu/exit"),
      -- The exit fade covers the menu, so the menu does not fade out itself.
      choose = function()
        trx.game.exit_game()
      end,
    }
    return nil, main
  end))
  list.follow_settings(ctx, state.pages)

  -- Plays a demo once nobody has touched the title for a while.
  local function wait_for_player()
    if trx.input.is_anything_held() or #state.pages > 1 then
      state.idle_ticks = 0
      return false
    end
    state.idle_ticks = state.idle_ticks + 1
    if state.demo_ticks > 0 and state.idle_ticks >= state.demo_ticks then
      trx.game.play_demo()
      leave()
      return true
    end
    return false
  end

  return ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = function()
        if ctx.background ~= nil and not ctx.is_live then
          trx.ui.primitive.background(ctx.background)
        end
        if
          not state.is_hidden
          and not state.is_away
          and not state.is_leaving
        then
          page():paint()
        end
      end,
    }),
    place = function()
      return 0, 0
    end,
    on_input = function(_, keys)
      if state.is_leaving or state.is_hidden or state.is_away then
        return
      end
      if wait_for_player() then
        return
      end
      local current = page()
      if keys:pressed(Role.MENU_BACK) then
        if #state.pages > 1 then
          sound("select")
          local left = table.remove(state.pages)
          follow_main()
          if left.on_back ~= nil then
            left.on_back()
          end
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
