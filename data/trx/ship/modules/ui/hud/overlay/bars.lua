-- The bars of the in-game overlay: Lara's health, air, sprint and warmth, and
-- the health of what she aims at.

local place = require("common.ui.hud.overlay.place")
local timer = require("common.timer")

local M = {}

trx.config.declare({ key = "ui.show_bars", kind = "boolean", default = true })
trx.config.declare({
  key = "ui.enable_bar_flashing",
  kind = "boolean",
  default = true,
})
trx.config.declare({
  key = "ui.healthbar_location",
  kind = "dynamic_enum",
  values = place.LOCATIONS,
  default = "top_left",
})
place.normalize("ui.healthbar_location")
trx.config.declare({
  key = "ui.airbar_location",
  kind = "dynamic_enum",
  values = place.LOCATIONS,
  default = "top_right",
})
place.normalize("ui.airbar_location")
trx.config.declare({
  key = "ui.sprintbar_location",
  kind = "dynamic_enum",
  values = place.LOCATIONS,
  default = "top_right",
})
place.normalize("ui.sprintbar_location")
trx.config.declare({
  key = "ui.exposurebar_location",
  kind = "dynamic_enum",
  values = place.LOCATIONS,
  default = "top_right",
})
place.normalize("ui.exposurebar_location")
trx.config.declare({
  key = "ui.enemy_healthbar_location",
  kind = "dynamic_enum",
  values = place.LOCATIONS,
  default = "bottom_left",
})
place.normalize("ui.enemy_healthbar_location")

trx.locale.declare({
  ["enums/bar_show_mode/never"] = "Never",
  ["enums/bar_show_mode/always"] = "Always",
  ["enums/bar_show_mode/boss_only"] = "Boss only",
  ["settings/ui.airbar_location/title"] = "Airbar location",
  ["settings/ui.airbar_location/description"] = "Location where the airbar is displayed.",
  ["settings/ui.enable_bar_flashing/title"] = "Flash bars",
  ["settings/ui.enable_bar_flashing/description"] = "Makes Lara's health and oxygen bars blink when she's running low on "
    .. "either resource.",
  ["settings/ui.enemy_healthbar_location/title"] = "Enemy bar location",
  ["settings/ui.enemy_healthbar_location/description"] = "Location where the enemy healthbar is displayed.",
  ["settings/ui.enemy_healthbar_show_mode/title"] = "Enemy bar mode",
  ["settings/ui.enemy_healthbar_show_mode/description"] = "Controls when a healthbar is shown for the enemy Lara has "
    .. "targeted.\n\n- Always: show it for any target.\n- Boss only: show it "
    .. "only when the target is a boss.\n- Never: never show it.",
  ["settings/ui.exposurebar_location/title"] = "Exposure bar location",
  ["settings/ui.exposurebar_location/description"] = "Location where the cold water exposure bar is displayed.",
  ["settings/ui.healthbar_location/title"] = "Healthbar location",
  ["settings/ui.healthbar_location/description"] = "Location where the healthbar is displayed.",
  ["settings/ui.show_bars/title"] = "Show bars",
  ["settings/ui.show_bars/description"] = "Shows the ingame bars. Turning them off obscures information on "
    .. "Lara's health and other resources (for challenge runs).",
  ["settings/ui.sprintbar_location/title"] = "Sprintbar location",
  ["settings/ui.sprintbar_location/description"] = "Location where the sprintbar is displayed.",
})
trx.config.declare({
  key = "ui.enemy_healthbar_show_mode",
  kind = "dynamic_enum",
  values = {
    { value = "never", label = "enums/bar_show_mode/never" },
    { value = "always", label = "enums/bar_show_mode/always" },
    { value = "boss_only", label = "enums/bar_show_mode/boss_only" },
  },
  default = "always",
})
place.normalize("ui.enemy_healthbar_show_mode")

-- Builds the bars and places them. `shared` holds what the rest of the
-- overlay reads too.
---@param shared common.ui.hud.overlay.Shared
function M.setup(shared)
  local ui = trx.ui
  local signal = trx.signal
  local lara = trx.lara

  local state = {
    ui_enabled = shared.ui_enabled,
    playing = shared.playing,
    armed = shared.armed,
    bars_enabled = signal.config("ui.show_bars"),
    flashing_enabled = signal.config("ui.enable_bar_flashing"),

    -- The inventory ring asks for the health bar while it shows a medipack.
    health_bar_forced = trx.overlay.signals.health_bar_forced,

    -- The blink a low bar flashes with, and whether Lara was hurt just now.
    -- Both are counted in the block below, so a widget reads an answer rather
    -- than a frame.
    blink = signal.new(false),
    recently_hit = signal.new(false),

    -- A level opens on Lara's health bar, which stands for a moment and goes.
    opening = signal.new(false),
  }

  -- Show Lara's bars only when she controls herself and the cutscene is not
  -- active. A cutscene keeps the screen through its fades.
  state.lara_bars = state.ui_enabled
    & lara.signals.is_controllable
    & ~trx.cutscenes.signals.is_active

  -- How low a bar has to be before it flashes, which TR1 puts lower than the
  -- rest. It is read as a bar asks, because which game is running is not settled
  -- when this is loaded.
  local function bar_low_threshold()
    return trx.game.tr_version == 1 and 0.2 or 0.25
  end

  -- Counts a signal that says when one thing starts and stops flashing, so the
  -- blink below runs only while something is flashing. Set in the block below.
  local flashes_with

  do
    local BLINK_PERIOD = 10
    local HIT_TIME = 40
    local OPENING_TIME = 100
    local UI_CLOCK = { clock = timer.Clock.UI }

    -- How many things are flashing at once. The blink runs while any of them
    -- is and stands still otherwise, so a screen of full bars is quiet.
    local flashing_count = 0
    ---@type common.timer.Timer?
    local blinking = nil

    function flashes_with(flashing)
      local function follow(on)
        flashing_count = flashing_count + (on and 1 or -1)
        if flashing_count > 0 and blinking == nil then
          blinking = timer.every(BLINK_PERIOD, function()
            state.blink:set(not state.blink:get())
          end, UI_CLOCK)
        elseif flashing_count <= 0 and blinking ~= nil then
          blinking:cancel()
          blinking = nil
          state.blink:set(false)
        end
      end
      if flashing:get() then
        follow(true)
      end
      flashing:on(follow)
    end

    -- Holds a signal true for some ticks, counted again from the start each
    -- time it is raised.
    local function raise_for(held, ticks)
      ---@type common.timer.Timer?
      local running = nil
      return function()
        if running ~= nil then
          running:cancel()
        end
        held:set(true)
        running = timer.after(ticks, function()
          running = nil
          held:set(false)
        end, UI_CLOCK)
      end
    end

    -- Being hurt is her health moving, which the signal already says.
    local hit = raise_for(state.recently_hit, HIT_TIME)
    lara.signals.hp:on(hit)

    -- A level starting is Lara entering the world, which is asked after rather
    -- than waited on so that every level counts, not only the first.
    local opening = raise_for(state.opening, OPENING_TIME)
    lara.signals.exists:on(function(exists)
      if exists then
        opening()
      end
    end)
  end

  -- How full a bar is, held between empty and full.
  local function fraction(value, full)
    if value == nil or full == nil or full <= 0 then
      return 0
    end
    return math.max(0, math.min(1, value / full))
  end

  -- Whether a bar is low enough to flash, which is the same answer for every
  -- bar that flashes at all.
  local function runs_low(fill)
    return state.flashing_enabled
      & fill:map(function(value)
        return value <= bar_low_threshold()
      end)
  end

  -- What a flashing bar shows: it goes blank on the blink rather than leaving
  -- the screen. Counting the blink is what a flashing bar asks for, so it is
  -- asked for here.
  local function blinking(fill, flashing)
    flashes_with(flashing)
    return signal.combine(fill, flashing & state.blink, function(value, blank)
      return blank and 0 or value
    end)
  end

  -------------------------------------------------------------------------------
  -- Lara's health, which the poisoned bar stands in for while she is poisoned
  -------------------------------------------------------------------------------

  do
    local poisoned = lara.signals.poison:map(function(poison)
      return type(poison) == "number" and poison > 0
    end)

    local health =
      signal.combine(lara.signals.hp, lara.signals.max_hp, fraction)

    -- It flashes while it is low, and while she is poisoned however full it is.
    local flashing = runs_low(health) | (state.flashing_enabled & poisoned)

    local value = blinking(health, flashing)

    local dying = health:map(function(fill)
      return fill <= 0
    end)

    -- It shows while she is armed, hurt, poisoned or dying, and whenever
    -- something else insists on it.
    local shown = state.lara_bars
      & (
        state.health_bar_forced
        | (
          state.playing
          & state.bars_enabled
          & (
            state.opening
            | state.recently_hit
            | dying
            | state.armed
            | poisoned
            | flashing
          )
        )
      )

    place.at_setting(
      "ui.healthbar_location",
      ui.widgets.Bar({
        type = ui.BarType.LARA_HP,
        value = value,
        visible = shown & ~poisoned,
      })
    )

    place.at_setting(
      "ui.healthbar_location",
      ui.widgets.Bar({
        type = ui.BarType.LARA_HP_POISON,
        value = value,
        visible = shown & poisoned,
      })
    )
  end

  -------------------------------------------------------------------------------
  -- the air she has left underwater
  -------------------------------------------------------------------------------

  do
    local air = lara.signals.air:map(function(left)
      return fraction(left, trx.lara.MAX_AIR)
    end)

    local value = blinking(air, runs_low(air))

    local water = lara.signals.water_status

    local in_swamp = lara.signals.room_num:map(function(room_num)
      local room = room_num ~= nil and trx.rooms[room_num] or nil
      return room ~= nil and room.swamp == true
    end)

    local in_upv = lara.signals.vehicle:map(function(item_num)
      local vehicle = item_num ~= nil and trx.items[item_num] or nil
      return vehicle ~= nil and vehicle.object_id == trx.catalog.objects.UPV
    end)

    local drawn_on = air:map(function(fill)
      return fill < 1
    end)

    place.at_setting(
      "ui.airbar_location",
      ui.widgets.Bar({
        type = ui.BarType.LARA_AIR,
        value = value,
        visible = state.lara_bars
          & state.playing
          & state.bars_enabled
          & (
            water:eq(trx.lara.WaterState.UNDERWATER)
            | water:eq(trx.lara.WaterState.SURFACE)
            | (in_swamp & drawn_on)
            | (water:eq(trx.lara.WaterState.ABOVE_WATER) & in_upv)
          ),
      })
    )
  end

  -------------------------------------------------------------------------------
  -- the sprint she has left, which never flashes
  -------------------------------------------------------------------------------

  do
    local sprint = lara.signals.sprint:map(function(left)
      return fraction(left, trx.lara.MAX_SPRINT)
    end)

    place.at_setting(
      "ui.sprintbar_location",
      ui.widgets.Bar({
        type = ui.BarType.LARA_STAMINA,
        value = sprint,
        visible = state.lara_bars
          & state.playing
          & state.bars_enabled
          & sprint:map(function(fill)
            return fill < 1
          end),
      })
    )
  end

  -------------------------------------------------------------------------------
  -- the warmth she has left in the cold
  -------------------------------------------------------------------------------

  do
    -- How much cold she can take is a rule, and a rule is the level's, so it is
    -- read rather than waited on: no level can start with it unanswered, and a
    -- level that changes it is followed. It is read only while the bar could
    -- show.
    local whole = signal.polled_while(
      state.lara_bars & state.playing & state.bars_enabled,
      function()
        return trx.rules.get("exposure.max")
      end,
      1
    )

    local exposure = signal.combine(lara.signals.exposure, whole, fraction)

    local value = blinking(exposure, runs_low(exposure))

    place.at_setting(
      "ui.exposurebar_location",
      ui.widgets.Bar({
        type = ui.BarType.LARA_EXPOSURE,
        value = value,
        visible = state.lara_bars
          & state.playing
          & state.bars_enabled
          & exposure:map(function(fill)
            return fill < 1
          end),
      })
    )
  end

  -------------------------------------------------------------------------------
  -- what she is aiming at, which is an ally or an enemy
  -------------------------------------------------------------------------------

  do
    -- Which objects the game counts as bosses, built as it is asked for and
    -- dropped when the level ends.
    local boss_ids = nil

    trx.events.on_level_unload(function()
      boss_ids = nil
    end)

    local function is_boss(object_id)
      if boss_ids == nil then
        boss_ids = {}
        for _, id in ipairs(trx.objects.query:boss():ids()) do
          boss_ids[id] = true
        end
      end
      return boss_ids[object_id] == true
    end

    local is_ally = lara.signals.target:map(function(item_num)
      local target = item_num ~= nil and trx.items[item_num] or nil
      return target ~= nil and target.is_ally
    end)

    -- Whether what she is aiming at is worth a bar, which only moves when she
    -- takes a new target or the player changes the setting.
    local worth_a_bar = signal.combine(
      lara.signals.target,
      signal.config("ui.enemy_healthbar_show_mode"),
      function(item_num, mode)
        local target = item_num ~= nil and trx.items[item_num] or nil
        if target == nil or mode == "never" then
          return false
        end
        return mode ~= "boss_only" or is_boss(target.object_id)
      end
    )

    local shown = state.ui_enabled
      & state.playing
      & state.bars_enabled
      & state.armed
      & worth_a_bar

    -- Its health is the one thing here the engine tells nobody about, so it is
    -- read once a tick where everything else waits to be told, and only while
    -- there is a bar on screen to read it for.
    local health = signal.polled_while(shown, function()
      local target = lara.target
      if target == nil or target.max_hit_points <= 0 then
        return 0
      end
      return fraction(target.hit_points, target.max_hit_points)
    end, 0)

    place.at_setting(
      "ui.enemy_healthbar_location",
      ui.widgets.Bar({
        type = ui.BarType.ENEMY_HP,
        value = health,
        visible = shown & ~is_ally,
      })
    )

    place.at_setting(
      "ui.enemy_healthbar_location",
      ui.widgets.Bar({
        type = ui.BarType.ALLY_HP,
        value = health,
        visible = shown & is_ally,
      })
    )
  end
end

return M
