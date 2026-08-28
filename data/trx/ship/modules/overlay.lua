-- The in-game overlay: the bars, the ammunition count, and the readouts that
-- say where Lara is and what she is doing.
--
-- The lines of text the rest of the engine asks for, the arrows beside them
-- and the version are not here. Those belong to whoever sets them and are
-- drawn from the engine, into the same regions these widgets sit in.
--
-- Every widget is made once, when this is loaded, and given signals rather
-- than values. A signal moving brings a widget up to date; a frame in which
-- nothing moves measures nothing and reads nothing.
--
-- Each thing on screen is built in a block of its own, which holds the signals
-- only it reads and ends by placing its widget. What several of them read is
-- declared once, at the top, and nothing else crosses between them.
--
-- Nothing here draws. The widgets own that, and the region a widget sits in
-- owns where it goes, so what this places is stacked beside what the
-- inventory ring and the dialogs place rather than over it.

local ui = trx.ui
local signal = trx.signal
local lara = trx.lara

trx.locale.declare({
  ["general/overlay/item_count_fmt_pc"] = "\\{small}%s",
  ["general/overlay/item_count_fmt_ps1"] = "\\{small}%s",
  ["general/overlay/debug_position"] = "Position: ",
  ["general/overlay/debug_rotation"] = "Rotation: ",
  ["general/overlay/debug_speed"] = "Speed: ",
  ["general/overlay/debug_interaction"] = "Interaction: ",
  ["general/overlay/debug_animation"] = "Animation: ",
  ["general/overlay/debug_animation_state"] = "State: ",
  ["general/overlay/debug_animation_arm_l"] = "Arm L: ",
  ["general/overlay/debug_animation_arm_r"] = "Arm R: ",
  ["general/overlay/debug_camera_pos"] = "Camera origin: ",
  ["general/overlay/debug_camera_target"] = "Camera target: ",
  ["general/overlay/debug_immune"] = "Invulnerability on",
})

-------------------------------------------------------------------------------
-- what more than one thing on screen reads
-------------------------------------------------------------------------------

-- The state every block below draws on, in one place. Everything here is a
-- signal; a widget is given one rather than a value, so a frame in which none
-- of them moved costs nothing.
--
-- A block that reads something no other block reads keeps it to itself, as a
-- plain local.
local state = {
  ui_enabled = signal.config("ui.enable_game_ui"),
  bars_enabled = signal.config("ui.show_bars"),
  flashing_enabled = signal.config("ui.enable_bar_flashing"),
  language = signal.config("language"),
  playing = trx.game.signals.is_playing,
  armed = lara.signals.gun_status:eq(trx.lara.GunState.READY),

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

-- Lara's own bars are hers to show, so they go while she is not in charge of
-- herself and while a cutscene has the screen.
state.lara_bars = state.ui_enabled
  & lara.signals.is_controllable
  & ~trx.cutscenes.signals.is_playing

-- How low a bar has to be before it flashes, which TR1 puts lower than the
-- rest. It is read as a bar asks, because which game is running is not settled
-- when this is loaded.
local function bar_low_threshold()
  return trx.game.tr_version == 1 and 0.2 or 0.25
end

-- Runs something once a tick, for only as long as it has something to count.
-- A counter with nothing to count costs no tick at all, which is what every
-- one of them is for most of a level.
local function ticker(step)
  local listener = nil
  return function(wanted)
    if wanted and listener == nil then
      listener = signal.tick:on(step)
    elseif not wanted and listener ~= nil then
      listener:detach()
      listener = nil
    end
  end
end

-- Counts a signal that says when one thing starts and stops flashing, so the
-- blink below runs only while something is flashing. Set in the block below.
local flashes_with

do
  local BLINK_PERIOD = 10
  local HIT_TIME = 40

  local blink_frame, hit_timer = 0, 0

  -- How many things are flashing at once. The blink is counted while any of
  -- them is and stands still otherwise, so a screen of full bars is quiet.
  local flashing_count = 0
  local run_blink = ticker(function()
    blink_frame = blink_frame + 1
    if blink_frame >= BLINK_PERIOD then
      blink_frame = 0
      state.blink:set(not state.blink:get())
    end
  end)

  function flashes_with(flashing)
    local function follow(on)
      flashing_count = flashing_count + (on and 1 or -1)
      if flashing_count > 0 then
        run_blink(true)
      else
        run_blink(false)
        blink_frame = 0
        state.blink:set(false)
      end
    end
    if flashing:get() then
      follow(true)
    end
    flashing:on(follow)
  end

  local run_hit
  run_hit = ticker(function()
    hit_timer = hit_timer - 1
    if hit_timer <= 0 then
      run_hit(false)
      state.recently_hit:set(false)
    end
  end)

  -- Being hurt is her health moving, which the signal already says.
  lara.signals.hp:on(function()
    hit_timer = HIT_TIME
    state.recently_hit:set(true)
    run_hit(true)
  end)

  local OPENING_TIME = 100

  local opening_timer = 0
  local run_opening
  run_opening = ticker(function()
    opening_timer = opening_timer - 1
    if opening_timer <= 0 then
      run_opening(false)
      state.opening:set(false)
    end
  end)

  -- A level starting is Lara entering the world, which is asked after rather
  -- than waited on so that every level counts, not only the first.
  lara.signals.exists:on(function(exists)
    if not exists then
      return
    end
    opening_timer = OPENING_TIME
    state.opening:set(true)
    run_opening(true)
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

-- A signal read once a tick, but only while something is on screen to show it.
-- Most of what is read here is off for most of a game, and a reader that is
-- off does not run at all: it holds the idle value until it is wanted again.
local function polled_while(shown, read, idle)
  local held = signal.new(idle)
  local run = ticker(function()
    held:set(read())
  end)
  local function follow(on)
    if on then
      held:set(read())
    else
      held:set(idle)
    end
    run(on)
  end
  if shown:get() then
    follow(true)
  end
  shown:on(follow)
  return held
end

local REGION_OF = {
  top_left = ui.Region.TOP_LEFT,
  top_center = ui.Region.TOP_CENTER,
  top_right = ui.Region.TOP_RIGHT,
  bottom_left = ui.Region.BOTTOM_LEFT,
  bottom_center = ui.Region.BOTTOM_CENTER,
  bottom_right = ui.Region.BOTTOM_RIGHT,
}

-- A bar follows the setting that says where it goes, so moving it in the
-- options moves it on screen.
local function place_at(key, widget)
  ui.regions.place(
    signal.config(key):map(function(location)
      return REGION_OF[location] or ui.Region.TOP_LEFT
    end),
    widget
  )
end

-------------------------------------------------------------------------------
-- Lara's health, which the poisoned bar stands in for while she is poisoned
-------------------------------------------------------------------------------

do
  local poisoned = lara.signals.poison:above(0)

  local health = signal.combine(lara.signals.hp, lara.signals.max_hp, fraction)

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

  place_at(
    "ui.healthbar_location",
    ui.widgets.Bar({
      type = ui.BarType.LARA_HP,
      value = value,
      shown = shown & ~poisoned,
    })
  )

  place_at(
    "ui.healthbar_location",
    ui.widgets.Bar({
      type = ui.BarType.LARA_HP_POISON,
      value = value,
      shown = shown & poisoned,
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

  place_at(
    "ui.airbar_location",
    ui.widgets.Bar({
      type = ui.BarType.LARA_AIR,
      value = value,
      shown = state.lara_bars
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

  place_at(
    "ui.sprintbar_location",
    ui.widgets.Bar({
      type = ui.BarType.LARA_STAMINA,
      value = sprint,
      shown = state.lara_bars
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
  -- level that changes it is followed.
  local whole = signal.polled(function()
    return trx.rules.get("exposure.max")
  end)

  local exposure = signal.combine(lara.signals.exposure, whole, fraction)

  local value = blinking(exposure, runs_low(exposure))

  place_at(
    "ui.exposurebar_location",
    ui.widgets.Bar({
      type = ui.BarType.LARA_EXPOSURE,
      value = value,
      shown = state.lara_bars
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
  local health = polled_while(shown, function()
    local target = lara.target
    if target == nil or target.max_hit_points <= 0 then
      return 0
    end
    local whole = target.max_hit_points * (trx.game.is_ngplus and 2 or 1)
    return fraction(target.hit_points, whole)
  end, 0)

  place_at(
    "ui.enemy_healthbar_location",
    ui.widgets.Bar({
      type = ui.BarType.ENEMY_HP,
      value = health,
      shown = shown & ~is_ally,
    })
  )

  place_at(
    "ui.enemy_healthbar_location",
    ui.widgets.Bar({
      type = ui.BarType.ALLY_HP,
      value = health,
      shown = shown & is_ally,
    })
  )
end

-------------------------------------------------------------------------------
-- how many shots she has left for what she is holding
-------------------------------------------------------------------------------

do
  -- The weapon the count answers for: the one the vehicle carries where she is
  -- riding an armed one, and otherwise the one in her hands.
  local function counted_gun()
    local vehicle_gun = lara.vehicle_gun
    if vehicle_gun ~= nil then
      return vehicle_gun, true
    end
    if not state.armed:get() then
      return nil
    end
    return lara.equipped_gun, false
  end

  local shown = state.ui_enabled & state.playing

  -- What she is carrying is not a signal, so the count is read once a tick,
  -- and only while the interface it is drawn on is up.
  local text = polled_while(shown, function()
    local gun, from_vehicle = counted_gun()
    if gun == nil then
      return nil
    end
    local weapon = trx.weapons[gun]
    if weapon == nil or weapon.has_infinite_ammo then
      return nil
    end
    if not from_vehicle and weapon.ammo_object == nil then
      return nil
    end

    local shots = trx.inventory:shots(gun)
    local icon = trx.game.tr_version == 1 and weapon.ammo_icon or nil
    local inner = icon ~= nil and string.format("%6d %s", shots, icon)
      or string.format("%6d", shots)
    -- Both keys are named where they are used, so what carries them into the
    -- shipped strings can see them.
    if trx.config.get("ui.menu_style") == "ps1" then
      return trx.locale.format("general/overlay/item_count_fmt_ps1", inner)
    end
    return trx.locale.format("general/overlay/item_count_fmt_pc", inner)
  end)

  place_at(
    "ui.ammo_counter_location",
    ui.widgets.Label({
      text = text:map(function(value)
        return value or ""
      end),
      scale = 1.5,
      shown = shown & text:map(function(value)
        return value ~= nil
      end),
    })
  )
end

-------------------------------------------------------------------------------
-- where Lara is and what she is doing
--
-- A readout changes on almost every tick it is shown on, so it is read on one
-- rather than waited on. Nothing is read while the readout is off.
-------------------------------------------------------------------------------

do
  -- A readout is drawn while the game is on screen and Lara is in it, which
  -- includes photo mode.
  local debuggable = state.ui_enabled
    & lara.signals.exists
    & (trx.game.signals.is_playing | trx.game.signals.is_photo_mode)

  local function tiles(p)
    return p.x // 1024, p.y // 1024, p.z // 1024
  end

  local function readout(shown, read)
    return ui.widgets.Label({
      text = polled_while(shown, read, ""),
      shown = shown,
    })
  end

  -- The key is named where it is read, so what carries it into the shipped
  -- strings can see it.
  local function title(shown, read)
    return ui.widgets.Label({ text = state.language:map(read), shown = shown })
  end

  local pos_shown = debuggable & signal.config("debug.enable_debug_pos")
  local anim_shown = debuggable & signal.config("debug.enable_debug_anim")
  local camera_shown = debuggable & signal.config("debug.enable_debug_camera")

  -- Every readout is a title and a value, and the two are drawn as columns so
  -- a value lines up with its title however wide either is.
  local titles = {
    title(pos_shown, function()
      return trx.locale.get("general/overlay/debug_position")
    end),
    title(pos_shown, function()
      return trx.locale.get("general/overlay/debug_rotation")
    end),
    title(pos_shown, function()
      return trx.locale.get("general/overlay/debug_speed")
    end),
    title(pos_shown, function()
      return trx.locale.get("general/overlay/debug_interaction")
    end),
    title(anim_shown, function()
      return trx.locale.get("general/overlay/debug_animation")
    end),
    title(anim_shown, function()
      return trx.locale.get("general/overlay/debug_animation_state")
    end),
    title(anim_shown, function()
      return trx.locale.get("general/overlay/debug_animation_arm_l")
    end),
    title(anim_shown, function()
      return trx.locale.get("general/overlay/debug_animation_arm_r")
    end),
    title(camera_shown, function()
      return trx.locale.get("general/overlay/debug_camera_pos")
    end),
    title(camera_shown, function()
      return trx.locale.get("general/overlay/debug_camera_target")
    end),
  }

  local values = {
    readout(pos_shown, function()
      local x, y, z = tiles(lara.item.pos)
      return string.format(
        "\\{small}%d, %d, %d / %d",
        x,
        y,
        z,
        lara.item.room_num
      )
    end),

    readout(pos_shown, function()
      local r = lara.item.rot
      return string.format(
        "\\{small}%d\u{00B0}, %d\u{00B0}, %d\u{00B0}",
        r.x * 360 // 65536,
        r.y * 360 // 65536,
        r.z * 360 // 65536
      )
    end),

    readout(pos_shown, function()
      local it = lara.vehicle or lara.item
      return string.format("\\{small}%d, %d", it.speed, it.fall_speed)
    end),

    readout(pos_shown, function()
      return string.format(
        "\\{small}%d / %d / %d",
        lara.interact_item_num,
        lara.is_interact_moving and 1 or 0,
        lara.interact_move_count
      )
    end),

    readout(anim_shown, function()
      return string.format(
        "\\{small}%d, %d",
        lara.item.anim_num,
        lara.item.frame_num
      )
    end),

    readout(anim_shown, function()
      return string.format(
        "\\{small}%d, %d (%d)",
        lara.item.anim_state,
        lara.item.goal_anim_state,
        trx.catalog.to_slot(trx.catalog.Context.OBJECTS, lara.animation_object)
      )
    end),

    readout(anim_shown, function()
      return string.format(
        "\\{small}%d, %d (%d)",
        lara.left_arm_anim_num,
        lara.left_arm_frame_num,
        lara.flare_control and 1 or 0
      )
    end),

    readout(anim_shown, function()
      return string.format(
        "\\{small}%d, %d",
        lara.right_arm_anim_num,
        lara.right_arm_frame_num
      )
    end),

    readout(camera_shown, function()
      local x, y, z = tiles(trx.camera.pos)
      return string.format(
        "\\{small}%d, %d, %d / %d",
        x,
        y,
        z,
        trx.camera.room_num
      )
    end),

    readout(camera_shown, function()
      local x, y, z = tiles(trx.camera.target_pos)
      return string.format(
        "\\{small}%d, %d, %d / %d",
        x,
        y,
        z,
        trx.camera.target_room_num
      )
    end),
  }

  -- The raw numbers behind the readouts, down the other side of the screen.
  local lines = {
    readout(pos_shown, function()
      local p = lara.item.pos
      return string.format("\\{small}%d, %d, %d", p.x, p.y, p.z)
    end),

    readout(pos_shown, function()
      local r = lara.item.rot
      return string.format("\\{small}%d, %d, %d", r.x, r.y, r.z)
    end),

    readout(camera_shown, function()
      local p = trx.camera.pos
      return string.format("\\{small}%d, %d, %d", p.x, p.y, p.z)
    end),

    readout(camera_shown, function()
      local p = trx.camera.target_pos
      return string.format("\\{small}%d, %d, %d", p.x, p.y, p.z)
    end),

    -- The mark that says nothing can hurt her.
    ui.widgets.Label({
      text = state.language:map(function()
        return trx.locale.get("general/overlay/debug_immune")
      end),
      scale = 0.8,
      shown = debuggable
        & signal.config("debug.enable_debug_status")
        & signal.config("debug.enable_invulnerability"),
    }),
  }

  ui.regions.place(
    ui.Region.TOP_LEFT,
    ui.widgets.Stack({
      orientation = ui.Orientation.HORIZONTAL,
      spacing = 8,
      children = {
        ui.widgets.Stack({ children = titles }),
        ui.widgets.Stack({ children = values }),
      },
    })
  )

  -- The raw numbers line up against the edge they sit at, the way the engine
  -- lines them up.
  ui.regions.place(
    ui.Region.TOP_RIGHT,
    ui.widgets.Stack({ children = lines, align = ui.HAlign.RIGHT })
  )
end

-------------------------------------------------------------------------------
-- the frames drawn in the last second
-------------------------------------------------------------------------------

do
  local shown = state.ui_enabled & signal.config("ui.enable_fps_counter")

  -- The count is taken once a second, so the line is built when it moves
  -- rather than on every tick that reads it.
  local fps = polled_while(shown, function()
    return trx.game.measured_fps
  end, 0)

  ui.regions.place(
    ui.Region.TOP_LEFT,
    ui.widgets.Label({
      text = fps:map(function(value)
        return string.format("%d FPS", value)
      end),
      shown = shown,
    })
  )
end
