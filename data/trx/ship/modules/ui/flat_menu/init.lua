-- The flat menu: the game's menu as the original TR4 draws it. What Lara
-- carries stands in a row across the screen, and the entry in the middle
-- names what can be done with it in a list.

local death = require("common.ui.flat_menu.death")
local fade = require("common.ui.fade")
local items = require("common.ui.flat_menu.items")
local ring = require("common.ui.ring")
local saves = require("common.ui.flat_menu.saves")
local sound = require("common.ui.flat_menu.sound").play
local strip = require("common.ui.flat_menu.strip")
local title = require("common.ui.flat_menu.title")
local timer = require("common.timer")

local M = {}

trx.locale.declare({
  ["general/flat_menu/use"] = "USE",
  ["general/flat_menu/combine"] = "COMBINE",
  ["general/flat_menu/equip"] = "EQUIP",
  ["general/flat_menu/combine_with"] = "COMBINE WITH",
  ["general/flat_menu/examine"] = "EXAMINE",
  ["general/flat_menu/load_game"] = "LOAD GAME",
  ["general/flat_menu/save_game"] = "SAVE GAME",
  ["general/flat_menu/count_fmt"] = "%d x %s",
})

local objects = trx.catalog.objects
local Role = trx.input.Role
local TEXT_SCALE = require("common.ui.flat_menu.text").SCALE

-- Where the original draws each part, as shares of the screen's height: the
-- row of what Lara carries, the row of what an entry combines with, and the
-- name over each.
local MAIN_ROW = 42 / 240
local COMBINE_ROW = 190 / 240
local MAIN_NAME = 1 / 2 - 3 / 16
local COMBINE_NAME = 1 / 2 + 3 / 16
-- The height of a line of the original's text.
local LINE = 3 / 40
-- The compass stands at the left, level with the middle of the screen.
local COMPASS_X = 60 / 512

-- The colours the original prints its text in.
local COLOR_PICKED = 1
local COLOR_OPTION = 5
local COLOR_NAME = 8

local FADE_TICKS = 12

-- How much larger an examined entry is drawn than in the row, and how long
-- it takes to fade in, as the original's examine mode does.
local EXAMINE_ZOOM = 2
local EXAMINE_FADE_TICKS = 16

-- The entry the menu was left on, which it opens on again where the player
-- asks the inventory to remember its position. A new game forgets it.
---@type integer?
local left_on = nil

local function is_remembered()
  local ok, mode = pcall(trx.config.get, "gameplay.ring_memory_mode")
  return ok and mode ~= nil and mode ~= "off"
end

-- The waterskins, each as the entries it is drawn as from empty to full. A
-- waterskin left in the menu opens it again however full the skin is now.
local WATERSKINS = {
  {
    "WATERSKIN_1_EMPTY",
    "WATERSKIN_1_1",
    "WATERSKIN_1_2",
    "WATERSKIN_1_3",
  },
  {
    "WATERSKIN_2_EMPTY",
    "WATERSKIN_2_1",
    "WATERSKIN_2_2",
    "WATERSKIN_2_3",
    "WATERSKIN_2_4",
    "WATERSKIN_2_5",
  },
}

local function colored(color, text)
  return ("\\{color %d}%s"):format(color, text)
end

-- The action an option of the list takes.
local Option = {
  LOAD = "load",
  SAVE = "save",
  EXAMINE = "examine",
  USE = "use",
  EQUIP = "equip",
  COMBINE = "combine",
}

local OPTION_TEXT = {
  [Option.LOAD] = function()
    return trx.locale.get("general/flat_menu/load_game")
  end,
  [Option.SAVE] = function()
    return trx.locale.get("general/flat_menu/save_game")
  end,
  [Option.EXAMINE] = function()
    return trx.locale.get("general/flat_menu/examine")
  end,
  [Option.USE] = function()
    return trx.locale.get("general/flat_menu/use")
  end,
  [Option.EQUIP] = function()
    return trx.locale.get("general/flat_menu/equip")
  end,
  [Option.COMBINE] = function()
    return trx.locale.get("general/flat_menu/combine")
  end,
}

-- Returns the weapon an entry draws, or nil for any other entry, its ammo
-- included.
local function weapon_of(object)
  for _, weapon in ipairs(trx.weapons.all) do
    if
      weapon.object ~= nil and trx.inventory.icon_of(weapon.object) == object
    then
      return weapon
    end
  end
  return nil
end

-- The examine items, which offer only to be examined, as in the original.
local EXAMINE_ONLY = { "EXAMINE_ITEM_1", "EXAMINE_ITEM_2", "EXAMINE_ITEM_3" }

local function is_examine_only(object)
  for _, key in ipairs(EXAMINE_ONLY) do
    local item = objects[key]
    if item ~= nil and (trx.inventory.icon_of(item) or item) == object then
      return true
    end
  end
  return false
end

-- The options of an entry, in the order the original lists them. Every entry
-- other than a weapon or an examine item offers to be used, as in the
-- original, even one the ring combines rather than uses, or one with nothing
-- to use it on yet, such as the crowbar.
local function options_of(object)
  if object == objects.LOAD_OPTION then
    return { Option.LOAD }
  elseif object == objects.SAVE_OPTION then
    return { Option.SAVE }
  end
  local actions = trx.inventory.actions(object) or {}
  local result = {}
  if actions.examine then
    result[#result + 1] = Option.EXAMINE
  end
  if weapon_of(object) ~= nil then
    result[#result + 1] = Option.EQUIP
  elseif not is_examine_only(object) then
    result[#result + 1] = Option.USE
  end
  if actions.combine then
    result[#result + 1] = Option.COMBINE
  end
  return result
end

-- The name of an entry, with how many there are, as the original names it.
-- A weapon shows no count, however many times Lara picked it up.
local function name_of(entry)
  local info = trx.objects[entry.object]
  local name = info ~= nil and info.name or ""
  if (entry.count or 0) > 1 and weapon_of(entry.object) == nil then
    local fmt = trx.locale.get("general/flat_menu/count_fmt")
    return fmt:format(entry.count, name)
  end
  return name
end

-- The load and save entries the original shows at the end of the row, where
-- the level has their models and the game lets the player save.
---@return table[]
local function save_entries()
  local result = {}
  local function add(object)
    local info = trx.objects[object]
    if info ~= nil and info.loaded then
      result[#result + 1] = { object = object, count = 1 }
    end
  end
  if trx.savegame.slot_count() > 0 then
    add(objects.LOAD_OPTION)
    if trx.savegame.manual_allowed then
      add(objects.SAVE_OPTION)
    end
  end
  return result
end

-- Whether the row shows an entry. The compass stands apart from the row, the
-- stopwatch leaves the statistics to the pause screen, and the binoculars
-- leave the row while the player turns them off.
local function is_listed(object)
  if
    object == objects.COMPASS_OPTION or object == objects.STOPWATCH_OPTION
  then
    return false
  elseif object == objects.BINOCULARS_OPTION then
    return trx.config.get("gameplay.enable_binoculars")
  end
  return true
end

-- What Lara carries, in the order the original shows it.
---@return table[]
local function contents()
  local result = {}
  for i = 1, trx.inventory.entry_count() do
    local entry = trx.inventory.entry_at(i)
    if entry ~= nil and is_listed(entry.object) then
      result[#result + 1] = {
        object = entry.object,
        count = entry.count,
        order = items.order(entry.object) or 1000 + i,
      }
    end
  end
  table.sort(result, function(a, b)
    return a.order < b.order
  end)
  return result
end

-- The entries that Lara carries that the picked one could combine with.
local function combine_contents(picked)
  local result = {}
  for _, entry in ipairs(contents()) do
    local actions = trx.inventory.actions(entry.object)
    if entry.object ~= picked and actions ~= nil and actions.combine then
      result[#result + 1] = entry
    end
  end
  return result
end

-------------------------------------------------------------------------------
-- Text
-------------------------------------------------------------------------------

-- Places a line of text, centred on a share of the screen's height.
local function push_line(ctx, text, row)
  return ctx:push({
    root = trx.ui.widgets.Label({ text = text, scale = TEXT_SCALE }),
    place = function(w)
      return (trx.ui.canvas.width - w) / 2, trx.ui.canvas.height * row
    end,
  })
end

-- Shows the name of the picked entry of a row.
local function name_line(state, row_strip, row)
  local text = trx.signal.new("")
  local function follow()
    local entry = row_strip:picked()
    text:set(entry ~= nil and colored(COLOR_NAME, name_of(entry)) or "")
  end
  follow()
  return push_line(state.ctx, text, row), follow
end

-- Shows the options of the picked entry, each centred on its own line, the
-- chosen one pulsing.
local function option_lines(state)
  local lines = trx.signal.new({})
  local function follow()
    local texts = {}
    for i, option in ipairs(state.options) do
      local color = i == state.option and COLOR_PICKED or COLOR_OPTION
      texts[#texts + 1] = colored(color, OPTION_TEXT[option]())
    end
    lines:set(texts)
  end
  follow()
  local primitive = trx.ui.primitive
  local function step()
    return trx.ui.canvas.height * LINE
  end
  local layer = state.ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        local texts = lines:get()
        local width = 0.0
        for _, text in ipairs(texts) do
          width = math.max(width, (primitive.measure_text(text, TEXT_SCALE)))
        end
        return width, step() * #texts
      end,
      paint = function(x, y, w)
        for i, text in ipairs(lines:get()) do
          local text_w = primitive.measure_text(text, TEXT_SCALE)
          primitive.text(
            text,
            x + (w - text_w) / 2,
            y + step() * (i - 1),
            TEXT_SCALE
          )
        end
      end,
    }),
    place = function(w)
      local line = step()
      local y = trx.ui.canvas.height / 2 - line
      if #state.options == 1 then
        y = y + line
      elseif #state.options == 2 then
        y = y + line / 2
      end
      return (trx.ui.canvas.width - w) / 2, y - line / 2
    end,
  })
  return layer, follow
end

-------------------------------------------------------------------------------
-- The menu's statuses
-------------------------------------------------------------------------------

local Status = {
  OPENING = "opening",
  OPEN = "open",
  COMBINING = "combining",
  -- The row of partners fades out, and where two entries combined, the row of
  -- what Lara carries fades out and back in on what they made, as the
  -- original does.
  UNCOMBINING = "uncombining",
  SWAPPING_OUT = "swapping_out",
  SWAPPING_IN = "swapping_in",
  FADING = "fading",
  SCREEN = "screen",
  DONE = "done",
}

local function finish(state, used)
  if state.status == Status.DONE then
    return
  end
  state.status = Status.DONE
  if used ~= nil and not trx.game.is_leaving then
    trx.inventory.use(used)
  end
  if state.ctx.is_held then
    state.ctx:confirm()
  end
end

local function hide_texts(state)
  for _, key in ipairs({ "name_layer", "options_layer" }) do
    if state[key] ~= nil then
      state[key]:close()
      state[key] = nil
    end
  end
  ring.show_health_bar(false)
end

local function show_texts(state)
  state.name_layer, state.follow_name = name_line(state, state.main, MAIN_NAME)
  state.options_layer, state.follow_options = option_lines(state)
end

-- Picks the entry the menu was left on, or another fill of the same
-- waterskin.
local function pick_left_on(row_strip)
  if not is_remembered() or left_on == nil or row_strip:pick(left_on) then
    return
  end
  for _, skin in ipairs(WATERSKINS) do
    local members = {}
    local is_member = false
    for _, key in ipairs(skin) do
      local object = objects[key]
      if object ~= nil then
        members[#members + 1] = object
        is_member = is_member or object == left_on
      end
    end
    if is_member then
      for _, object in ipairs(members) do
        if row_strip:pick(object) then
          return
        end
      end
    end
  end
end

-- Closes the menu, fading its entries out with its backdrop. Without a fade,
-- the backdrop and the entries stay until the menu has closed, so that the
-- game does not show for a frame before what the menu used.
local function close(state, used)
  local ticks = state.fades:ticks(FADE_TICKS)
  state.closing = used
  -- The key that chose the entry is the action key in the game too. Held on,
  -- it would act at once as the menu closes, and open the menu again at a
  -- lock that the entry does not fit.
  if used ~= nil then
    trx.input.hold_off(Role.ACTION)
  end
  local picked = state.main ~= nil and state.main:picked() or nil
  if picked ~= nil then
    left_on = picked.object
  end
  if ticks > 0 then
    state.backdrop:to(0, ticks)
    if state.main ~= nil then
      state.main:show(false, ticks)
    end
  end
  hide_texts(state)
  state.status = Status.FADING
end

local function refresh_options(state)
  local entry = state.main:picked()
  state.options = entry ~= nil and options_of(entry.object) or {}
  state.option = 1
  state.follow_options()
  state.follow_name()
end

-- Shows Lara's health bar while the row rests on a medipack with its options
-- showing, and hides it as soon as the row slides on and while the player
-- keeps turning it, as the ring does.
local function follow_health_bar(state)
  local entry = state.main ~= nil and state.main:picked() or nil
  ring.show_health_bar(
    state.options_layer ~= nil
      and not state.main:is_sliding()
      and not (state.turning_held and #state.main.list > 1)
      and entry ~= nil
      and (
        entry.object == objects.SMALL_MEDIPACK_OPTION
        or entry.object == objects.LARGE_MEDIPACK_OPTION
      )
  )
end

-- Builds the row again around what Lara carries now, resting on an entry.
local function rebuild(state, object)
  local list = contents()
  if state.mode == trx.ui.MenuMode.GAME then
    for _, entry in ipairs(save_entries()) do
      list[#list + 1] = entry
    end
  end
  local current = strip.new(list, MAIN_ROW)
  current.on_change = function()
    refresh_options(state)
  end
  if object ~= nil then
    current:pick(object)
  end
  state.main = current
  refresh_options(state)
end

-- Shows the list of save slots in place of the row, and brings the row back
-- once the list closes without leaving the game.
local function open_saves(state, kind)
  state.status = Status.SCREEN
  state.hidden = true
  hide_texts(state)
  saves.push(state.ctx, kind, function(done)
    if done and kind == saves.Kind.LOAD then
      close(state, nil)
      return
    end
    state.hidden = false
    state.status = Status.OPEN
    show_texts(state)
    refresh_options(state)
  end)
end

local function open_screen(state, screen, object, on_end)
  state.status = Status.SCREEN
  local ctx
  ctx = trx.ui.screens.open(screen, { object = object }, function(confirmed)
    if state.screen ~= ctx then
      return
    end
    state.screen = nil
    state.status = Status.OPEN
    on_end(confirmed)
  end)
  state.screen = ctx
  if ctx == nil then
    state.status = Status.OPEN
  end
end

-- Shows the examine screen with the entry alone in the middle of the screen,
-- as the original's examine mode does, and the row, its names and its options
-- out of the way. The screen reads input only while the ring says that its
-- entry rests open, which an examined entry always does, and turns the entry
-- through the ring.
local function examine(state, object)
  hide_texts(state)
  local examined = {
    object = object,
    yaw = 0.0,
    pitch = 0.0,
    fade = trx.ui.tween(0):to(1, EXAMINE_FADE_TICKS),
  }
  state.examined = examined
  ring.set_rest_frame(0)
  ring.set_turner(function(yaw, pitch)
    examined.yaw = examined.yaw + yaw
    examined.pitch = examined.pitch + pitch
  end)
  local function put_away()
    state.examined = nil
    ring.set_rest_frame(nil)
    ring.set_turner(nil)
    ring.set_hint({})
    show_texts(state)
    refresh_options(state)
  end
  open_screen(state, trx.ui.Screen.EXAMINE, object, put_away)
  if state.screen == nil then
    put_away()
  end
end

local function start_combine(state)
  local picked = state.main:picked()
  local list = combine_contents(picked.object)
  if #list == 0 then
    sound("no")
    return
  end
  state.combine = strip.new(list, COMBINE_ROW)
  state.combine.fade:set(0)
  state.combine:show(true)
  state.combine.on_change = function()
    state.follow_combine_name()
  end
  state.combine_name, state.follow_combine_name =
    name_line(state, state.combine, COMBINE_NAME)
  state.combine_heading = push_line(
    state.ctx,
    colored(COLOR_PICKED, trx.locale.get("general/flat_menu/combine_with")),
    0.5 - LINE / 2
  )
  state.options_layer:close()
  state.options_layer = nil
  state.status = Status.COMBINING
end

local function stop_combine(state, combined)
  state.combine_name:close()
  state.combine_heading:close()
  state.combine:show(false)
  state.combined = combined
  state.status = Status.UNCOMBINING
end

-- Runs the steps that follow a combine, one after the other as each fade
-- ends.
local function tick_combine(state)
  if state.status == Status.UNCOMBINING and not state.combine:is_fading() then
    state.combine = nil
    if state.combined == nil then
      state.options_layer, state.follow_options = option_lines(state)
      state.status = Status.OPEN
      return
    end
    hide_texts(state)
    state.main:show(false)
    state.status = Status.SWAPPING_OUT
  elseif
    state.status == Status.SWAPPING_OUT and not state.main:is_fading()
  then
    rebuild(state, state.combined)
    state.combined = nil
    state.main.fade:set(0)
    state.main:show(true)
    state.status = Status.SWAPPING_IN
  elseif state.status == Status.SWAPPING_IN and not state.main:is_fading() then
    state.status = Status.OPEN
    show_texts(state)
    refresh_options(state)
  end
end

local function choose(state)
  local option = state.options[state.option]
  local entry = state.main:picked()
  if option == nil or entry == nil then
    return
  end
  if option ~= Option.USE and option ~= Option.EQUIP then
    sound("choose")
  end
  if option == Option.USE or option == Option.EQUIP then
    close(state, entry.object)
  elseif option == Option.EXAMINE then
    examine(state, entry.object)
  elseif option == Option.COMBINE then
    start_combine(state)
  elseif option == Option.LOAD then
    open_saves(state, saves.Kind.LOAD)
  elseif option == Option.SAVE then
    open_saves(state, saves.Kind.SAVE)
  end
end

local function control_combine(state, keys)
  local combine = state.combine
  if combine:is_fading() then
    return
  end
  if keys:held(Role.MENU_LEFT) then
    if combine:turn(1) then
      sound("rotate")
    end
    return
  elseif keys:held(Role.MENU_RIGHT) then
    if combine:turn(-1) then
      sound("rotate")
    end
    return
  end
  if combine:is_sliding() then
    return
  end
  if keys:pressed(Role.MENU_CONFIRM) then
    local picked = state.main:picked().object
    local partner = combine:picked().object
    ---@type trx.catalog.objects?
    local combined = nil
    for _, other in ipairs(trx.inventory.combine_partners(picked) or {}) do
      if other == partner then
        combined = trx.inventory.combine(picked, partner)
      end
    end
    if combined ~= nil then
      sound("combine")
      stop_combine(state, trx.inventory.icon_of(combined) or combined)
    else
      sound("no")
      stop_combine(state, nil)
    end
  elseif keys:pressed(Role.MENU_BACK) then
    sound("select")
    stop_combine(state, nil)
  end
end

local function control(state, keys)
  state.turning_held = keys:held(Role.MENU_LEFT) or keys:held(Role.MENU_RIGHT)
  if state.status == Status.COMBINING then
    control_combine(state, keys)
    return
  end
  if state.status ~= Status.OPEN then
    return
  end
  if keys:pressed(Role.OPTION) or keys:pressed(Role.MENU_BACK) then
    sound("select")
    close(state, nil)
    return
  end
  local main = state.main
  if keys:held(Role.MENU_LEFT) then
    if main:turn(1) then
      sound("rotate")
    end
    return
  elseif keys:held(Role.MENU_RIGHT) then
    if main:turn(-1) then
      sound("rotate")
    end
    return
  end
  if main:is_sliding() then
    return
  end
  if keys:pressed(Role.MENU_UP) and state.option > 1 then
    state.option = state.option - 1
    sound("select")
    state.follow_options()
  elseif keys:pressed(Role.MENU_DOWN) and state.option < #state.options then
    state.option = state.option + 1
    sound("select")
    state.follow_options()
  elseif keys:pressed(Role.MENU_CONFIRM) then
    choose(state)
  end
end

local function tick(state)
  if state.main == nil then
    if state.status == Status.FADING and not state.backdrop:is_moving() then
      finish(state, state.closing)
    end
    return
  end
  state.main:tick()
  if state.combine ~= nil then
    state.combine:tick()
  end
  -- The names and the options show once the row has faded in, as they go
  -- before it fades out.
  if state.status == Status.OPENING and not state.main:is_fading() then
    state.status = Status.OPEN
    show_texts(state)
    refresh_options(state)
  end
  tick_combine(state)
  follow_health_bar(state)
  if state.status == Status.FADING and not state.backdrop:is_moving() then
    finish(state, state.closing)
  end
end

-- The compass points where Lara faces, settling after a swing as the menu
-- opens.
local function compass_bones(state)
  local lara = trx.lara.item
  local facing = lara ~= nil and lara.rot.y or 0
  local swing = state.compass_settle
    * math.sin(math.pi * 2 * (trx.game.real_time * 30 % 64) / 64)
  return { { bone = 0, y = trx.math.wrap_angle(facing + swing - 0x8000) } }
end

local function paint(state)
  trx.ui.primitive.backdrop(state.style, state.backdrop:get())
  local examined = state.examined
  if examined ~= nil then
    strip.draw(
      examined.object,
      trx.ui.canvas.width / 2,
      trx.ui.canvas.height / 2,
      {
        xrot = examined.pitch,
        yrot = examined.yaw,
        zoom = EXAMINE_ZOOM,
        opacity = examined.fade:get(),
      }
    )
    return
  end
  if state.hidden or state.main == nil then
    return
  end
  state.main:paint()
  if state.combine ~= nil then
    state.combine:paint()
  end
  if state.compass then
    local w, h = trx.ui.canvas.width, trx.ui.canvas.height
    local left = w / 2 - h * 4 / 3 / 2
    strip.draw(objects.COMPASS_OPTION, left + h * 4 / 3 * COMPASS_X, h / 2, {
      bones = compass_bones(state),
      opacity = state.backdrop:get(),
    })
  end
end

-------------------------------------------------------------------------------
-- Opening
-------------------------------------------------------------------------------

-- Forgets the entry the menu was left on as a game starts. Called once, as
-- the menu is set up.
function M.setup()
  trx.events.on_game_start(function()
    left_on = nil
  end)
end

-- Opens the quick save and load screens as the list of save slots. The
-- screen ends with a cancel, which the engine takes whether or not a game was
-- saved.
---@param ctx trx.ui.SaveLoadContext
---@return trx.ui.StackLayer
function M.open_save_load(ctx)
  local kind = ctx.mode == trx.ui.MenuMode.LOAD and saves.Kind.LOAD
    or saves.Kind.SAVE
  return saves.push(ctx, kind, function()
    ctx:cancel()
  end)
end

-- Opens the menu in the game, or to use an entry at a lock.
-- On the title screen, it opens the title menu.
-- Once Lara has died, it opens the menu that follows.
---@param ctx trx.ui.MenuContext
---@return trx.ui.StackLayer?
function M.open(ctx)
  local MenuMode = trx.ui.MenuMode
  if ctx.mode == MenuMode.TITLE then
    return title.open(ctx)
  end
  if ctx.mode == MenuMode.DEATH then
    return death.open(ctx)
  end
  local saving = ctx.mode == MenuMode.SAVE or ctx.mode == MenuMode.SAVE_CRYSTAL
  local loading = ctx.mode == MenuMode.LOAD
  if
    ctx.mode ~= MenuMode.GAME
    and ctx.mode ~= MenuMode.KEYS
    and not saving
    and not loading
  then
    return nil
  end
  ---@type table[]
  local list = {}
  if not saving and not loading then
    list = contents()
    if ctx.mode == MenuMode.GAME then
      for _, entry in ipairs(save_entries()) do
        list[#list + 1] = entry
      end
    end
    if #list == 0 then
      return nil
    end
  end

  ---@type table<string, any>
  local state = {
    ctx = ctx,
    mode = ctx.mode,
    status = Status.OPEN,
    options = {},
    option = 1,
    compass_settle = 4096,
  }
  local compass = trx.objects[objects.COMPASS_OPTION]
  state.compass = compass ~= nil
    and compass.loaded
    and trx.inventory.has(objects.COMPASS_OPTION)

  state.fades = fade.screen(ctx, {
    enabled = function()
      return fade.enabled("ui.inventory_fade_effects")
    end,
  })
  state.style = trx.config.get("ui.inventory_background_style")
  state.backdrop = trx.ui.tween(0):to(1, state.fades:ticks(FADE_TICKS))

  if saving or loading then
    state.compass = false
  else
    state.main = strip.new(list, MAIN_ROW)
    if ctx.object ~= nil then
      state.main:pick(trx.inventory.icon_of(ctx.object) or ctx.object)
    else
      pick_left_on(state.main)
    end
    state.main.on_change = function()
      refresh_options(state)
    end
    state.main.fade:set(0)
    state.main:show(true, state.fades:ticks(FADE_TICKS))
  end

  local ticking = timer.every(1, function()
    if state.compass_settle > 0 then
      state.compass_settle = math.max(state.compass_settle - 32, 0)
    end
    tick(state)
  end, { clock = timer.Clock.UI })

  ctx:on_end(function()
    ticking:cancel()
    ring.show_health_bar(false)
    ring.set_rest_frame(nil)
    ring.set_turner(nil)
    ring.set_hint({})
    if state.screen ~= nil then
      local screen = state.screen
      state.screen = nil
      screen:cancel()
    end
  end)

  local layer = ctx:push({
    root = trx.ui.widgets.Custom({
      measure = function()
        return trx.ui.canvas.width, trx.ui.canvas.height
      end,
      paint = function()
        paint(state)
      end,
    }),
    place = function()
      return 0, 0
    end,
    on_input = function(_, keys)
      control(state, keys)
    end,
  })
  if saving or loading then
    state.status = Status.SCREEN
    saves.push(ctx, saving and saves.Kind.SAVE or saves.Kind.LOAD, function()
      close(state, nil)
    end)
    return layer
  end
  state.status = Status.OPENING
  return layer
end

return M
