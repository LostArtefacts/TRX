-- The statistics screen: what the player found in a level when it ends, the
-- totals when the game ends, and the same from the compass or the stopwatch
-- in the inventory ring. In the gym, the stopwatch shows the best times on the
-- assault course instead, and a held key clears them.

local box = require("common.ui.screens.stats.box")
local counts = require("common.ui.screens.stats.counts")
local fade = require("common.ui.fade")
local listing = require("common.ui.screens.stats.rows")
local ring = require("common.ui.ring")
require("common.ui.scroll")

local M = {}

trx.config.declare({
  key = "ui.stats_fade_effects",
  kind = "boolean",
  default = true,
})

local Mode = listing.Mode

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/stats/basic_fmt"] = "%d",
  ["general/stats/detail_fmt"] = "%d of %d",
  ["general/stats/final_statistics"] = "Final Statistics",
  ["general/stats/bonus_statistics"] = "Bonus Statistics",
  ["general/stats/assault_title"] = "BEST TIMES",
  ["general/stats/assault_no_times_set"] = "No Times Set",
  ["general/stats/assault_finish"] = "Finish",
  ["general/stats/assault_best_time_fmt"] = "%s",
  ["general/stats/assault_other_times_fmt"] = "%s",
  ["general/stats/gym_assault_course"] = "Assault Course",
  ["general/stats/gym_racetrack_course"] = "Race Track Course",
  ["general/stats/assault_reset_times"] = "Reset",
  ["general/stats/level"] = "Level",
  ["general/stats/time_taken"] = "Time Taken",
  ["general/stats/ammo"] = "Ammo Hits/Used",
  ["general/stats/ammo_used"] = "Ammo Used",
  ["general/stats/ammo_fmt"] = "%d / %d",
  ["general/stats/ammo_hits"] = "Hits",
  ["general/stats/kills"] = "Kills",
  ["general/stats/crystals"] = "Crystals",
  ["general/stats/pickups"] = "Pickups",
  ["general/stats/deaths"] = "Deaths",
  ["general/stats/medipacks_used"] = "Health Packs Used",
  ["general/stats/distance_travelled"] = "Distance Traveled",
  ["general/stats/secrets"] = "Secrets Found",
  ["general/stats/none"] = "None",
  ["settings/ui.stats_fade_effects/title"] = "Stats fade effects",
  ["settings/ui.stats_fade_effects/description"] = "Fine-tunes the fade effects to be enabled or disabled in the end of "
    .. "the level statistics screen. Needs the Fade Effects option to be "
    .. "enabled to work.",
})

-- The settings that decide which rows the screen lists.
local ROW_SETTINGS = {
  "gameplay.save_crystal_mode",
  "ui.stats.show_ammo",
  "ui.stats.show_crystals",
  "ui.stats.show_deaths",
  "ui.stats.show_distance_travelled",
  "ui.stats.show_kills",
  "ui.stats.show_level_header",
  "ui.stats.show_medipacks_used",
  "ui.stats.show_pickups",
  "ui.stats.show_secrets",
  "ui.stats.show_time_taken",
  "ui.stats.show_totals",
}

-- Reads the counters. They hold still while the screen is up, except for the
-- level clock in the inventory ring, which runs on and which its row follows.
local function count(state, live)
  if state.mode == Mode.LEVEL then
    state.totals = counts.level_totals(state.level)
    state.floor_secrets = counts.has_floor_secrets(state.level)
  elseif state.mode == Mode.FINAL then
    state.totals =
      counts.final_totals(state.level.type == trx.game.LevelType.BONUS)
  else
    return
  end
  if live then
    local stats = state.level.stats
    state.clock = trx.signal.polled(function()
      return stats.timer // trx.game.LOGIC_FPS
    end)
    state.time_text = state.clock:map(function(seconds)
      return counts.format_time(state, seconds * trx.game.LOGIC_FPS)
    end)
  else
    state.time_text = counts.format_time(state, state.totals.timer)
  end
end

-- The box with what it lists now. The ring hides it until the entry is open.
local function box_of(state)
  state.rows = listing.all_rows(state)
  return box.build(state)
end

-- Builds the box again whenever what it lists or the room for it changes: the
-- settings, the room that the rest of the interface leaves, and for the best
-- times, how many rows the box shows and the times themselves. The scroll
-- moves the rows on show without building the box again. The screen ending
-- stops it.
local function follow(ctx, state)
  local sources = { trx.ui.signals.layout }
  for _, key in ipairs(ROW_SETTINGS) do
    sources[#sources + 1] = trx.signal.config(key)
  end
  if state.mode == Mode.ASSAULT then
    sources[#sources + 1] = state.visible
    sources[#sources + 1] = state.records
  end
  local function rebuild()
    state.layer:set_root(box_of(state), box.place(state))
  end
  local listeners = {}
  for _, source in ipairs(sources) do
    listeners[#listeners + 1] = source:on(rebuild)
  end
  ctx:on_end(function()
    for _, listener in ipairs(listeners) do
      listener:detach()
    end
    if state.clock ~= nil then
      state.time_text:stop()
      state.clock:stop()
    end
    if state.resting ~= nil then
      state.resting:stop()
    end
  end)
end

local function new_state(level, mode, bare, live)
  local state = {
    level = level,
    mode = mode,
    bare = bare,
    look = listing.look(),
  }
  if mode == Mode.ASSAULT then
    state.first = trx.signal.new(1)
    state.visible = trx.signal.new(listing.assault_visible_rows())
    state.records = trx.signal.new(0)
  end
  count(state, live)
  state.rows = listing.all_rows(state)
  return state
end

local function push(ctx, state, on_input)
  state.layer = ctx:push({
    root = box_of(state),
    place = box.place(state),
    on_input = on_input,
  })
  follow(ctx, state)
  return state.layer
end

-- How long the backdrop takes to come in: a picture from black, any other
-- style from the game behind it. A pattern shows at once.
local BACKDROP_TICKS = { image = 30, default = 15 }
local PATTERNS = { pattern_static = true, pattern_wave = true }

-- How long the screen takes to fade to black as it leaves, and then how long
-- it stays black, before the next screen starts.
local LEAVE_TICKS = 15
local LEAVE_HOLD_TICKS = 3

-- Whether the screen fades: always at the end of the game, and otherwise as
-- the player picked.
local function fades_run(ctx)
  return ctx.is_final or trx.config.get("ui.stats_fade_effects")
end

-- Brings in the backdrop the screen stands on: the picture that ends the
-- game, or the style the player picked.
local function enter_backdrop(ctx)
  local style = ctx.is_final and "image"
    or trx.config.get("ui.stats_background_style")
  local fades = fade.screen(ctx, {
    enabled = function()
      return fades_run(ctx)
    end,
  })
  fades:enter({
    layer = { backdrop = style, path = ctx.background },
    from = PATTERNS[style] and 1 or 0,
    to = 1,
    ticks = BACKDROP_TICKS[style] or BACKDROP_TICKS.default,
  })
end

-- The screen between levels and at the end of the game. A skip ends it, and
-- the next screen starts once the screen has faded to black.
---@param ctx trx.ui.StatsContext
local function open_level_end(ctx)
  local mode = ctx.is_final and Mode.FINAL or Mode.LEVEL
  local state = new_state(ctx.level, mode, ctx.is_bare, false)
  if mode == Mode.FINAL and #state.rows == 0 then
    return nil
  end
  enter_backdrop(ctx)
  return push(ctx, state, function(_, keys)
    if not keys:pressed(trx.input.Role.MENU_SKIP) then
      return
    end
    trx.input.hold_off_skip(trx.input.SkipContext.TO_SCREEN)
    if fades_run(ctx) then
      fade.hand_over({ ticks = LEAVE_TICKS, hold = LEAVE_HOLD_TICKS })
    end
    ctx:confirm()
  end)
end

local function scroll(state, step)
  ---@type integer
  local visible = state.visible:get()
  ---@type integer
  local first = state.first:get()
  first = first + step
  local last_first = math.max(#state.rows - visible + 1, 1)
  if first < 1 or first > last_first then
    if not trx.config.get("ui.enable_wraparound") then
      return
    end
    first = first < 1 and last_first or 1
  end
  state.first:set(first)
end

-- Reads the reset button. A completed hold clears the times.
local function control_reset(state, keys)
  if not state.reset:control(keys, counts.has_any_time()) then
    return
  end
  for _, track in ipairs(counts.tracks()) do
    trx.assault.stats.clear(track)
  end
  state.first:set(1)
  state.records:set(state.records:get() + 1)
end

local function in_gym()
  local level = trx.game.current_level
  return level ~= nil and level.type == trx.game.LevelType.GYM
end

local function close(ctx, object, confirmed)
  local anim = trx.inventory_ring.selection_anim()
  if anim ~= nil then
    trx.inventory_ring.animate_selection(anim.frame_count - 1, 1)
  end
  if object == trx.catalog.objects.STOPWATCH_OPTION then
    trx.sound.stop(trx.catalog.samples.MENU_STOPWATCH)
  end
  if confirmed then
    ctx:confirm()
  else
    ctx:cancel()
  end
end

-- The compass and the stopwatch in the inventory ring. The compass shows the
-- statistics only where the player asks it to, and is otherwise just put
-- away again.
---@param ctx trx.ui.EntryContext
local function open_ring_entry(ctx)
  local object = ctx.object
  local level = trx.game.current_level
  if level == nil then
    return nil
  end
  local shows_stats = object ~= trx.catalog.objects.COMPASS_OPTION
    or trx.config.get("gameplay.enable_compass_stats")

  local mode = Mode.LEVEL
  if in_gym() and trx.game.tr_version >= 2 then
    mode = Mode.ASSAULT
  end
  local state =
    new_state(level, mode, trx.config.get("ui.stats.style") == "bare", true)
  state.resting = ring.resting()
  state.shown = shows_stats and state.resting or false

  return push(
    ctx,
    state,
    ring.while_resting(function(_, keys)
      -- The rows the best times show are counted once the entry rests open,
      -- against the room the ring leaves then.
      if mode == Mode.ASSAULT and not state.counted_rows then
        state.counted_rows = true
        state.visible:set(listing.assault_visible_rows())
      end
      if mode == Mode.ASSAULT then
        control_reset(state, keys)
        if keys:pressed(trx.input.Role.MENU_DOWN) then
          scroll(state, 1)
        elseif keys:pressed(trx.input.Role.MENU_UP) then
          scroll(state, -1)
        end
      end
      if keys:pressed(trx.input.Role.MENU_CONFIRM) then
        close(ctx, object, true)
        return
      elseif keys:pressed(trx.input.Role.MENU_BACK) then
        close(ctx, object, false)
        return
      elseif object == trx.catalog.objects.STOPWATCH_OPTION then
        trx.sound.play(trx.catalog.samples.MENU_STOPWATCH)
      end
    end)
  )
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.STATS, open_level_end)
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open_ring_entry,
    { object = trx.catalog.objects.COMPASS_OPTION }
  )
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open_ring_entry,
    { object = trx.catalog.objects.STOPWATCH_OPTION }
  )
end

return M
