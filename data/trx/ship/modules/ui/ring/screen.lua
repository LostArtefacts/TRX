-- The inventory ring, which the game opens over the play, at a lock as the
-- keys ring, to save or load, when Lara dies, and as the title screen's menu.
--
-- The ring holds an entry for each thing Lara carries, or the entries of the
-- menu. Left and right turn it, up and down swap it for the keys ring above or
-- the menu below, and choosing an entry opens what the entry leads to: the
-- screen a script defines for it, or examining or combining it. The ring
-- closes once the player backs out or uses what they chose.
--
-- Usage:
--   require("common.ui.ring.screen").setup()

local entries = require("common.ui.ring.entries")
local fade = require("common.ui.fade")
local motion = require("common.ui.ring.motion")
local ring = require("common.ui.ring")
local timer = require("common.timer")

local M = {}

local Ring = entries.Ring
local Motion = motion.Motion
local Spin = motion.Spin
local Role = trx.input.Role
local Arrow = trx.overlay.Arrow
local objects = trx.catalog.objects
local samples = trx.catalog.samples
local DEG_1 = trx.math.DEG_1
local wrap = trx.math.wrap_angle

-- How long the backdrop takes to come in and to go, and how long the screen
-- takes to go black when the game leaves, in ticks.
local FADE_TICKS = 16
local BLACK_TICKS = 8
local BLACK_HOLD_TICKS = 1

-- How dark the ring goes behind the screen of the picked entry, where that
-- screen asks for it.
local DARKEN = 0.75

-- How far the title's camera tilts down onto its ring.
local TITLE_PITCH = 1024

local Status = {
  OPENING = "opening",
  OPEN = "open",
  LEAVING = "leaving",
  SELECTING = "selecting",
  SELECTED = "selected",
  CLOSING_ITEM = "closing_item",
  DESELECTING = "deselecting",
  EXITING = "exiting",
  CLOSING = "closing",
  FADING_OUT = "fading_out",
  DONE = "done",
}

-- The statuses that the ring names what it rests on in, and shows its
-- heading.
local SHOWN = {
  [Status.OPEN] = true,
  [Status.SELECTING] = true,
  [Status.SELECTED] = true,
  [Status.CLOSING_ITEM] = true,
  [Status.DESELECTING] = true,
}

-- The statuses the entry the ring rests on is held forward in.
local HELD = {
  [Status.SELECTING] = true,
  [Status.SELECTED] = true,
  [Status.CLOSING_ITEM] = true,
  [Status.DESELECTING] = true,
}

local Action = {
  USE = "use",
  EXAMINE = "examine",
  COMBINE = "combine",
}

-- The screen a script is offered for what the player does with an entry.
local SCREEN_OF = {
  [Action.USE] = trx.ui.Screen.RING_ENTRY,
  [Action.EXAMINE] = trx.ui.Screen.EXAMINE,
  [Action.COMBINE] = trx.ui.Screen.COMBINE,
}

-------------------------------------------------------------------------------
-- Where the rings rest between openings
-------------------------------------------------------------------------------

-- The entry each ring rests on, kept from one opening to the next, by its
-- place and by what it holds.
---@type table<string, { index: integer, object: integer? }>
local places = {}

-- The entry each ring was left on, for the player who asked the rings to open
-- where they were left.
---@type table<string, integer>
local left_on = {}

local function forget()
  places = {}
  left_on = {}
end

local function is_remembered(ring_type)
  local ok, mode = pcall(trx.config.get, "gameplay.ring_memory_mode")
  if not ok then
    return false
  end
  if mode == "main_ring" then
    return ring_type == Ring.MAIN
  elseif mode == "all_rings" then
    return ring_type == Ring.MAIN or ring_type == Ring.KEYS
  end
  return false
end

local function fixes_duplication()
  local ok, fixed =
    pcall(trx.config.get, "gameplay.fix_item_duplication_glitch")
  return ok and fixed
end

-- Returns the place of an object on a list, or nil.
local function index_of(list, object)
  for i, content in ipairs(list) do
    if content.object == object then
      return i
    end
  end
  return nil
end

-- Returns where a ring rests as it opens: where it was left, where the player
-- asked for that, or else the place it last rested at. Once an entry before
-- that place goes, the place moves to the start, rather than resting on what
-- moved into it, where the game fixes the item duplication glitch.
local function resting_place(ring_type, list, fallback)
  if is_remembered(ring_type) and left_on[ring_type] ~= nil then
    local index = index_of(list, left_on[ring_type])
    if index ~= nil then
      return index
    end
  end
  local place = places[ring_type]
  if place == nil then
    return fallback or 1
  end
  if
    fixes_duplication()
    and list[place.index] ~= nil
    and list[place.index].object ~= place.object
  then
    return 1
  end
  return math.max(1, math.min(place.index, #list))
end

local function keep_place(state)
  if state.stale ~= nil then
    places[state.ring_type] = state.stale.place
    return
  end
  local view = state.view
  local content = state.list[view.current]
  places[state.ring_type] = {
    index = view.current,
    object = content ~= nil and content.object or nil,
  }
end

-- Returns the place the keys ring rests on when the main ring moves up to
-- it, where the duplication glitch is left unfixed and an entry used up at
-- the end of the list left the place past the end. The ring rests on the
-- entry the place wraps round to, as the original game does, and picking
-- that entry uses the used up one, until the ring turns. Returns nil
-- otherwise.
local function stale_place(ring_type, place, mode)
  if
    ring_type ~= Ring.KEYS
    or fixes_duplication()
    or place == nil
    or place.object == nil
  then
    return nil
  end
  local count = #entries.contents(ring_type, mode)
  if count == 0 or place.index <= count then
    return nil
  end
  return {
    index = (place.index - 1) % count + 1,
    object = place.object,
    place = place,
  }
end

-------------------------------------------------------------------------------
-- Drawing
-------------------------------------------------------------------------------

-- The meshes of the passport, one bit each, and which show as its pages turn.
local PASS_MESH = {
  SPINE = 1,
  FRONT = 2,
  IN_FRONT = 4,
  PAGE_2 = 8,
  BACK = 16,
  IN_BACK = 32,
  PAGE_1 = 64,
}
local PASS_COMMON = PASS_MESH.SPINE
  | PASS_MESH.BACK
  | PASS_MESH.IN_BACK
  | PASS_MESH.FRONT
local PASS_PAGES = {
  { 14, PASS_MESH.IN_FRONT | PASS_MESH.PAGE_1 },
  { 18, PASS_MESH.IN_FRONT | PASS_MESH.PAGE_1 | PASS_MESH.PAGE_2 },
  { 19, PASS_MESH.PAGE_1 | PASS_MESH.PAGE_2 },
  { 23, PASS_MESH.PAGE_1 | PASS_MESH.PAGE_2 | PASS_MESH.IN_BACK },
  { 28, PASS_MESH.PAGE_2 | PASS_MESH.IN_BACK },
  { 29, 0 },
}

-- The mesh of the save crystal that shows, which the save crystal mode
-- decides, so that it cannot be declared with the entry.
local function crystal_meshes()
  local ok, mode = pcall(trx.config.get, "gameplay.save_crystal_mode")
  local heal = ok and mode == "heal"
  local index
  if trx.game.version == 3 then
    index = heal and 0 or 1
  elseif heal then
    index = 2
  else
    local ps1_ok, ps1 = pcall(trx.config.get, "visuals.enable_ps1_crystals")
    index = (ps1_ok and ps1) and 1 or 0
  end
  local object = trx.objects[objects.SAVE_CRYSTAL_OPTION]
  if object ~= nil and index >= object.mesh_count then
    index = 0
  end
  return 1 << index
end

local function meshes_of(object, spec, frame)
  if object == objects.PASSPORT_OPTION then
    for _, page in ipairs(PASS_PAGES) do
      if frame <= page[1] then
        return PASS_COMMON | page[2]
      end
    end
    return PASS_COMMON
  elseif
    object == objects.COMPASS_OPTION or object == objects.STOPWATCH_OPTION
  then
    if frame == 0 or frame >= 18 then
      return spec.meshes_sel
    end
    return -1
  elseif object == objects.SAVE_CRYSTAL_OPTION then
    return crystal_meshes()
  end
  return spec.meshes_sel
end

local function grey(value)
  return { r = value, g = value, b = value }
end

-- The light an entry is drawn in, each game the way it lit its own ring, and
-- the entry the ring rests on brighter than the rest. `shade` runs from `0`
-- for that entry to `1` for any other.
local function light_of(shade)
  local version = trx.game.version
  if version == 4 then
    return { ambient = grey((160 + (32 - 160) * shade) / 128 * 255) }
  elseif version == 3 then
    local unit = 255 / 4096
    return {
      ambient = grey(32),
      lights = {
        {
          dir = { x = 0x4000, y = -0x4000, z = 0x3000 },
          color = { r = 3312 * unit, g = 1664 * unit, b = 0 },
        },
        {
          dir = { x = -0x4000, y = -0x4000, z = 0x3000 },
          color = grey(3312 * unit),
        },
        {
          dir = { x = 0, y = 0x2000, z = 0x3000 },
          color = { r = 0, g = 0, b = 3072 * unit },
        },
      },
    }
  end
  -- TR1 and TR2 shade a model darker by its ambient light and lighten it
  -- towards the ring's one light.
  return {
    ambient = grey(128 - 32 * shade),
    lights = {
      {
        dir = { x = -1536, y = 256, z = 1024 },
        color = grey(170.67),
      },
    },
  }
end

-- The needle of the compass swings towards north as Lara faces, and settles.
local function swing_needle(state, entry)
  local lara = trx.lara ~= nil and trx.lara.item or nil
  if lara == nil then
    return
  end
  local delta = wrap(-entry.y_rot:get() - lara.rot.y - state.needle)
  state.needle_speed = trx.math.idiv(state.needle_speed * 19, 20)
    + trx.math.idiv(delta, 50)
  state.needle = wrap(state.needle + state.needle_speed)
end

-- The turns the hands of the stopwatch take, from the time the level has
-- been played.
local function stopwatch_bones()
  local level = trx.game.current_level
  local frames = level ~= nil and level.stats.timer or 0
  local seconds = frames // trx.game.LOGIC_FPS
  local function hand(period)
    return wrap((seconds % period) * DEG_1 * -360 // period)
  end
  return {
    { bone = 3, z = hand(43200) },
    { bone = 4, z = hand(3600) },
    { bone = 5, z = hand(60) },
  }
end

-- The object an entry draws now: the passport opens out of its closed cover
-- once the player picks it.
local function drawn_object(state, index)
  local content = state.list[index]
  if
    content.object == objects.PASSPORT_CLOSED
    and index == state.view.current
    and state.passport_open
  then
    return objects.PASSPORT_OPTION
  end
  return content.object
end

local function paint_entry(state, index)
  local view = state.view
  local content = state.list[index]
  ---@type common.ui.ring.Anim
  local anim = state.anims[index]
  local object = drawn_object(state, index)
  local opts = {
    camera = view:camera(),
    pose = view:pose(index),
    frame = anim.frame,
    at_pivot = content.spec.draws_at_pivot,
    mesh_mask = meshes_of(object, content.spec, anim.frame),
    light = light_of(view:shade(index)),
  }
  -- The entry moves from the frame the tick started on to the one it reached,
  -- and holds still while its animation waits.
  if anim.last ~= anim.frame then
    opts.frame = anim.last
    opts.next_frame = anim.frame
    opts.blend = trx.ui.interpolation_rate()
  end
  if object == objects.COMPASS_OPTION then
    opts.bones = { { bone = 0, y = state.needle } }
  elseif object == objects.STOPWATCH_OPTION then
    opts.bones = stopwatch_bones()
  end
  trx.ui.primitive.world_model(object, opts)
end

-- Returns the entries from the back of the ring to the front. Each model
-- hides only its own far side, so a nearer entry has to be painted later to
-- stand over the ones behind it.
local function back_to_front(state)
  local view = state.view
  local order = {}
  local depth = {}
  for index in ipairs(state.list) do
    local angle = view.rot:get() + (index - 1) * view:spacing()
    order[index] = index
    depth[index] = math.sin(angle * math.pi / (2 * trx.math.DEG_90))
  end
  table.sort(order, function(a, b)
    if depth[a] ~= depth[b] then
      return depth[a] > depth[b]
    end
    return a < b
  end)
  return order
end

local function paint(state)
  local ctx = state.ctx
  if state.mode ~= trx.ui.MenuMode.TITLE then
    trx.ui.primitive.backdrop(state.style, state.backdrop:get())
  elseif ctx.background ~= nil and not ctx.is_live then
    trx.ui.primitive.background(ctx.background)
  end
  if state.hidden then
    return
  end
  for _, index in ipairs(back_to_front(state)) do
    paint_entry(state, index)
  end
  if state.screen ~= nil and ring.signals.darkened:get() then
    trx.ui.primitive.darken(DARKEN)
  end
end

-------------------------------------------------------------------------------
-- The ring's contents
-------------------------------------------------------------------------------

-- What the motions read of an entry: its declared poses and the box its
-- model fills.
local function motion_spec(content)
  local spec = content.spec
  local info = trx.objects[content.object]
  local min_x, min_y, min_z, max_x, max_y, max_z
  if info ~= nil and info.loaded then
    min_x, min_y, min_z, max_x, max_y, max_z =
      trx.ui.primitive.mesh_bounds(content.object)
  end
  local bounds = nil
  if min_x ~= nil then
    bounds = {
      min = { x = min_x, y = min_y, z = min_z },
      max = { x = max_x, y = max_y, z = max_z },
    }
  end
  return {
    x_rot_pt_sel = spec.x_rot_pt_sel,
    x_rot_sel = spec.x_rot_sel,
    x_rot_nosel = spec.x_rot_nosel,
    y_rot_sel = spec.y_rot_sel,
    y_trans_sel = spec.y_trans_sel,
    z_trans_sel = spec.z_trans_sel,
    scale = spec.scale,
    y_offset = spec.y_offset,
    base_rot = {
      x = spec.base_rot_x,
      y = spec.base_rot_y,
      z = spec.base_rot_z,
    },
    bounds = bounds,
  }
end

-- Where an entry is in its animation: the frame on show, the frame it heads
-- for and which way, and the steps left on the frame it is on.
---@class common.ui.ring.Anim
---@field frame integer
---@field last integer The frame the tick started on, which the paint moves on from.
---@field goal integer
---@field direction integer
---@field count integer

-- Puts a ring of a type on show, resting on an entry, with the camera where
-- the ring before it left it. Each entry keeps its animation frame and its
-- turn about its own axis until the ring closes, as it moves between rings.
local function fill(state, ring_type, current)
  local before = state.view
  if before ~= nil then
    for i, content in ipairs(state.list) do
      state.kept[content.object] = {
        anim = state.anims[i],
        y_rot = before.entries[i].y_rot:latest(),
      }
    end
  end
  local list = entries.contents(ring_type, state.mode)
  local specs = {}
  ---@type common.ui.ring.Anim[]
  local anims = {}
  for i, content in ipairs(list) do
    specs[i] = motion_spec(content)
    local kept = state.kept[content.object]
    anims[i] = kept ~= nil and kept.anim
      or { frame = 0, last = 0, goal = 0, direction = 1, count = 0 }
  end
  local view = motion.new({
    entries = specs,
    current = math.max(1, math.min(current, #list)),
    pitch = state.mode == trx.ui.MenuMode.TITLE and TITLE_PITCH or 0,
  })
  for i, content in ipairs(list) do
    local kept = state.kept[content.object]
    if kept ~= nil then
      view.entries[i].y_rot:set(kept.y_rot)
    end
  end
  if before ~= nil then
    view.camera_y:set(before.camera_y:get())
    view.pitch:set(before.pitch:get())
    view.left_by = before.left_by
  end
  state.ring_type = ring_type
  state.list = list
  state.anims = anims
  state.view = view
end

-- Rests the ring on the entry a script asked for, where the ring holds it
-- and Lara carries it.
local function rest_on_request(state)
  local object = ring.take_request()
  if object == nil then
    return
  end
  local index = index_of(state.list, object)
  if
    index ~= nil
    and (state.ring_type == Ring.OPTION or trx.inventory.has(object))
  then
    state.view.current = index
    state.view.rot:set(state.view:rest_angle())
  end
end

-------------------------------------------------------------------------------
-- What the ring says
-------------------------------------------------------------------------------

local function format_count(fmt, count)
  local full = trx.locale.get("general/inventory_ring/item_count_fmt")
  return full:format(fmt):format(count)
end

-- The weapon whose pickup or ammunition an entry is, if any.
local function weapon_of(object)
  for _, weapon in ipairs(trx.weapons.all) do
    if
      weapon.object ~= nil and trx.inventory.icon_of(weapon.object) == object
    then
      return weapon, false
    end
    if
      weapon.ammo_object ~= nil
      and trx.inventory.icon_of(weapon.ammo_object) == object
    then
      return weapon, true
    end
  end
  return nil, false
end

-- The count that stands with the caption: what a weapon has left in shots,
-- the rounds a box of ammunition holds, and how many there are of anything
-- else Lara carries more than one of. A weapon that never runs out has none.
local function count_of(content)
  local object = content.object
  local weapon, is_ammo = weapon_of(object)
  if weapon ~= nil then
    if weapon.has_infinite_ammo then
      return nil
    end
    if is_ammo then
      return format_count("%d", content.count * weapon.ammo.box_label_qty)
    end
    local fmt = "%5d"
    if trx.game.version == 1 and weapon.ammo_icon ~= nil then
      fmt = "%5d " .. weapon.ammo_icon
    end
    return format_count(fmt, trx.inventory.shots(weapon.id))
  end
  if object == objects.FLARES_BOX_OPTION then
    local flare = trx.weapons[trx.catalog.weapons.FLARE]
    if flare ~= nil and flare.has_infinite_ammo then
      return nil
    end
    return format_count("%d", content.count)
  end
  if content.count > 1 then
    return format_count("%d", content.count)
  end
  return nil
end

local function name_of(object)
  if object == objects.PASSPORT_OPTION then
    return nil
  end
  -- In the save pickup mode the crystal in the inventory is the save crystal
  -- Lara picked up, rather than a plain collectible.
  if object == objects.SAVE_CRYSTAL_OPTION then
    local ok, mode = pcall(trx.config.get, "gameplay.save_crystal_mode")
    if ok and mode == "save_pickup" then
      object = objects.SAVE_CRYSTAL_ITEM
    end
  end
  local info = trx.objects[object]
  local name = info ~= nil and info.name or nil
  if name == nil or name == "" then
    return nil
  end
  return name
end

-- The hints for what the player may do with the entry the ring rests on.
local function hints_of(state)
  local actions = state.actions
  if actions == nil or not (actions.examine or actions.combine) then
    return {}
  end
  local result = {}
  if actions.examine then
    result[#result + 1] = {
      input = Role.MENU_SHOW_INFO,
      text = trx.locale.get("general/actions/examine_item"),
    }
  end
  if actions.use or actions.combine then
    result[#result + 1] = {
      input = Role.MENU_CONFIRM,
      text = actions.combine and trx.locale.get(
        "general/actions/combine_item"
      ) or trx.locale.get("general/actions/use_item"),
    }
  end
  return result
end

-- The name of the ring on show, which the menu calls game over once Lara
-- has died.
local function heading_of(state)
  if state.ring_type == Ring.MAIN then
    return trx.locale.get("general/inventory_ring/heading_inventory")
  elseif state.ring_type == Ring.KEYS then
    return trx.locale.get("general/inventory_ring/heading_items")
  elseif state.mode == trx.ui.MenuMode.DEATH then
    return trx.locale.get("general/inventory_ring/heading_game_over")
  end
  return trx.locale.get("general/inventory_ring/heading_option")
end

local function show_heading(state, shown)
  if shown ~= true then
    ring.set_heading(nil)
    return
  end
  local fmt = trx.locale.get("general/inventory_ring/heading_fmt")
  ring.set_heading(fmt:format(heading_of(state)))
end

-- Sets the caption, where it changes.
local function caption(state, text)
  if text ~= state.caption then
    state.caption = text
    trx.overlay.set_caption(text)
  end
end

local function show_arrows(state, shown)
  local up = false
  local down = false
  if shown and state.mode == trx.ui.MenuMode.GAME then
    up = state.ring_type == Ring.OPTION
      or (
        state.ring_type == Ring.MAIN
        and entries.is_available(Ring.KEYS, state.mode)
      )
    down = state.ring_type == Ring.KEYS
      or (
        state.ring_type == Ring.MAIN
        and entries.is_available(Ring.OPTION, state.mode)
      )
  end
  if up == state.arrow_up and down == state.arrow_down then
    return
  end
  state.arrow_up = up
  state.arrow_down = down
  trx.overlay.show_arrow(Arrow.TOP_LEFT, up)
  trx.overlay.show_arrow(Arrow.TOP_RIGHT, up)
  trx.overlay.show_arrow(Arrow.BOTTOM_LEFT, down)
  trx.overlay.show_arrow(Arrow.BOTTOM_RIGHT, down)
end

-- Names the entry the ring rests on and what can be done with it, while the
-- ring stands still. The caption stays the script's while it holds the
-- entry's screen, and once a screen the player left the ring from closes.
local function say(state)
  local shown = SHOWN[state.status] and state.mode ~= trx.ui.MenuMode.TITLE
  show_heading(state, shown)
  show_arrows(state, SHOWN[state.status] == true)

  local view = state.view
  local quiet = not SHOWN[state.status]
    or view:is_turning()
    or (state.turning_held and #state.list > 1)
  local scripted = (state.screen ~= nil and state.action ~= Action.EXAMINE)
    or (state.status == Status.SELECTED and state.action == Action.COMBINE)
    or state.left_by_screen
  if quiet then
    caption(state, nil)
    ring.set_hint({})
    ring.set_count(nil)
    ring.show_health_bar(false)
    return
  end
  if scripted then
    -- The script sets the caption meanwhile, so the ring sets its own again
    -- once it takes it back.
    state.caption = false
    ring.show_health_bar(false)
    return
  end

  local content = state.list[view.current]
  caption(state, name_of(drawn_object(state, view.current)))
  ring.show_health_bar(
    content.object == objects.SMALL_MEDIPACK_OPTION
      or content.object == objects.LARGE_MEDIPACK_OPTION
  )
  -- A screen open on the entry, such as the examine screen, shows hints of its
  -- own.
  if state.screen == nil then
    ring.set_hint(state.status == Status.OPEN and hints_of(state) or {})
  end
  ring.set_count(count_of(content))
end

-------------------------------------------------------------------------------
-- The ring's statuses
-------------------------------------------------------------------------------

local function set_status(state, status, motion_name)
  state.status = status
  if motion_name ~= nil then
    state.view:start(motion_name)
  end
end

local function is_forced_mode(mode)
  local MenuMode = trx.ui.MenuMode
  return mode == MenuMode.SAVE
    or mode == MenuMode.SAVE_CRYSTAL
    or mode == MenuMode.LOAD
    or mode == MenuMode.DEATH
end

local function leaves_after_save(mode)
  local MenuMode = trx.ui.MenuMode
  return mode == MenuMode.SAVE
    or mode == MenuMode.SAVE_CRYSTAL
    or mode == MenuMode.LOAD
end

-- Closes the ring, to black where the game leaves, and back to the game
-- otherwise, its backdrop going with it.
local function close(state)
  if trx.game.is_leaving or state.demo_needed then
    set_status(state, Status.CLOSING, Motion.CLOSE)
    state.after_close = Status.FADING_OUT
  else
    set_status(state, Status.CLOSING, Motion.CLOSE)
    state.after_close = Status.DONE
    -- Without a fade the backdrop stays until the ring has closed, and goes
    -- with the screen.
    local ticks = state.fades:ticks(FADE_TICKS)
    if ticks > 0 then
      state.backdrop:to(0, ticks)
    end
  end
end

local function finish(state)
  if state.status == Status.DONE then
    return
  end
  state.status = Status.DONE
  ring.set_hint({})
  ring.set_count(nil)
  ring.show_health_bar(false)
  if state.demo_needed then
    trx.game.play_demo()
  elseif state.chosen == objects.PHOTO_OPTION then
    if trx.game.gym ~= nil then
      trx.game.play_gym()
    end
  elseif state.chosen ~= nil and not trx.game.is_leaving then
    trx.inventory.use(state.chosen)
  end
  if state.ctx.is_held then
    state.ctx:confirm()
  end
end

-- The sound the ring makes as the player picks an entry.
local function pick_sound(object)
  if object == objects.COMPASS_OPTION then
    return samples.MENU_COMPASS
  elseif object == objects.STOPWATCH_OPTION then
    return nil
  elseif object == objects.PHOTO_OPTION then
    return samples.MENU_LARA_HOME
  elseif object == objects.CONTROLS_OPTION then
    return trx.game.version == 1 and samples.MENU_GAMEBOY
      or samples.MENU_CHOOSE
  end
  local weapon, is_ammo = weapon_of(object)
  if weapon ~= nil and not is_ammo then
    return samples.MENU_GUNS
  end
  return samples.MENU_CHOOSE
end

-- Picks the entry the ring rests on, to use it or to examine it.
local function pick(state, examine)
  if is_forced_mode(state.mode) then
    state.passport_shown = true
  end
  keep_place(state)
  local view = state.view
  local content = state.list[view.current]
  local spec = content.spec
  local anim = state.anims[view.current]
  if examine then
    state.action = Action.EXAMINE
    anim.goal = 0
  elseif state.actions ~= nil and state.actions.combine then
    state.action = Action.COMBINE
    anim.goal = 0
  else
    state.action = Action.USE
    anim.goal = spec.open_frame
  end
  anim.direction = 1
  state.offered = false
  set_status(state, Status.SELECTING, Motion.SELECT)
  local sound = pick_sound(content.object)
  if sound ~= nil then
    trx.sound.play(sound)
  end
end

-- Starts to put the entry back, to the ring or out of it.
local function close_item(state, exits)
  state.close_to = exits and Status.EXITING or Status.DESELECTING
  state.status = Status.CLOSING_ITEM
end

-- Swaps the ring for the one above, by `up`, or below.
local function leave_for(state, ring_type, up)
  state.next_ring = ring_type
  set_status(
    state,
    Status.LEAVING,
    up and Motion.LEAVE_UP or Motion.LEAVE_DOWN
  )
end

-- What the player does with the ring while it rests open.
local function control_open(state, keys)
  local view = state.view
  if view:is_turning() then
    return
  end
  local count = #state.list
  state.turning_held = keys:held(Role.MENU_LEFT) or keys:held(Role.MENU_RIGHT)
  if keys:held(Role.MENU_RIGHT) and count > 1 then
    state.stale = nil
    view:turn_to(-1)
    trx.sound.play(samples.MENU_ROTATE)
    return
  end
  if keys:held(Role.MENU_LEFT) and count > 1 then
    state.stale = nil
    view:turn_to(1)
    trx.sound.play(samples.MENU_ROTATE)
    return
  end

  local MenuMode = trx.ui.MenuMode
  if keys:pressed(Role.MENU_BACK) and state.mode ~= MenuMode.TITLE then
    state.chosen = nil
    keep_place(state)
    trx.sound.play(samples.MENU_SPINOUT)
    close(state)
    return
  end

  local examine = keys:pressed(Role.MENU_SHOW_INFO)
    and state.actions ~= nil
    and state.actions.examine
  if keys:pressed(Role.MENU_CONFIRM) or examine then
    pick(state, examine)
    return
  end

  if state.mode == MenuMode.TITLE or state.mode == MenuMode.KEYS then
    return
  end
  if keys:pressed(Role.MENU_UP) then
    if state.ring_type == Ring.MAIN then
      if entries.is_available(Ring.KEYS, state.mode) then
        leave_for(state, Ring.KEYS, true)
      end
    elseif state.ring_type == Ring.OPTION then
      if entries.is_available(Ring.MAIN, state.mode) then
        leave_for(state, Ring.MAIN, true)
      end
    end
  elseif keys:pressed(Role.MENU_DOWN) then
    if state.ring_type == Ring.MAIN then
      if entries.is_available(Ring.OPTION, state.mode) then
        leave_for(state, Ring.OPTION, false)
      end
    elseif state.ring_type == Ring.KEYS then
      if entries.is_available(Ring.MAIN, state.mode) then
        leave_for(state, Ring.MAIN, false)
      end
    end
  end
end

-- The game's plain items, such as the keys and the binoculars, which the
-- engine names the generic inventory options. They are found once a level.
---@type table<integer, boolean>?
local generic = nil

local function is_generic(object)
  if generic == nil then
    generic = {}
    for _, id in ipairs(trx.objects.query:family("generic_inv_option"):ids()) do
      generic[id] = true
    end
  end
  return generic[object] == true
end

-- The entries that do what they are for at once when no script draws them:
-- a weapon or a medipack is used, and anything else the player carries a
-- number of is used as well, as is an entry declared to be.
local function is_used_at_once(content)
  if content.spec.is_used_at_once then
    return true
  end
  local weapon, is_ammo = weapon_of(content.object)
  if weapon ~= nil then
    return not is_ammo
  end
  return content.object == objects.SMALL_MEDIPACK_OPTION
    or content.object == objects.LARGE_MEDIPACK_OPTION
    or (content.spec.inv_pos >= 100 and content.spec.inv_pos < 200)
    or content.object == objects.SCION_OPTION
    or is_generic(content.object)
end

-- What the player does with an entry that no script's screen holds.
local function control_entry(state, keys, busy)
  local content = state.list[state.view.current]
  local anim = state.anims[state.view.current]
  local confirm = keys:pressed(Role.MENU_CONFIRM)
  local back = keys:pressed(Role.MENU_BACK)

  if state.action ~= Action.USE then
    -- Nothing examines or combines it, so any key puts it away.
    if not busy and (confirm or keys:pressed(Role.MENU_SHOW_INFO)) then
      back, confirm = true, false
    end
  elseif is_used_at_once(content) then
    confirm = not busy
  elseif
    weapon_of(content.object) == nil
    and not busy
    and (confirm or back)
  then
    anim.goal = 0
    anim.direction = -1
  end

  if busy then
    return
  end
  if back then
    close_item(state, leaves_after_save(state.mode))
  elseif confirm then
    state.chosen = state.stale ~= nil and state.stale.object or content.object
    keep_place(state)
    local stays = state.mode == trx.ui.MenuMode.TITLE
      and (
        content.object == objects.GRAPHICS_OPTION
        or content.object == objects.SOUND_OPTION
        or content.object == objects.PDA_OPTION
        or content.object == objects.CONTROLS_OPTION
      )
    close_item(state, not stays)
  end
end

-- Offers the screen of what the player does with the entry, once.
local function offer(state)
  if state.offered then
    return
  end
  state.offered = true
  local screen = SCREEN_OF[state.action]
  local object = drawn_object(state, state.view.current)
  ---@type trx.ui.ScreenContext?
  local ctx
  ctx = trx.ui.screens.open(screen, { object = object }, function(confirmed)
    if state.screen ~= ctx then
      return
    end
    state.screen = nil
    if state.action == Action.EXAMINE then
      state.view:let_go()
    end
    if state.status ~= Status.SELECTED then
      return
    end
    -- A ring opened to save or load holds nothing else worth going back to,
    -- so putting the entry away leaves it. A combined entry goes back into
    -- the ring, which builds itself again around what the script made.
    local leaves = (confirmed and state.action ~= Action.COMBINE)
      or leaves_after_save(state.mode)
    if confirmed then
      keep_place(state)
    end
    state.left_by_screen = leaves
    close_item(state, leaves)
  end)
  state.screen = ctx
  state.unhandled = ctx == nil
end

-------------------------------------------------------------------------------
-- The tick
-------------------------------------------------------------------------------

-- Runs the selected entry's animation two steps a tick. Returns whether it is
-- still on its way.
local function animate(state)
  local index = state.view.current
  local anim = state.anims[index]
  local total = state.list[index].spec.frames_total
  local busy = false
  for _ = 1, 2 do
    busy = anim.frame ~= anim.goal
    if busy then
      if anim.count > 0 then
        anim.count = anim.count - 1
      else
        anim.count = state.list[index].spec.anim_speed
        anim.frame = anim.frame + anim.direction
        if anim.frame >= total then
          anim.frame = 0
        elseif anim.frame < 0 then
          anim.frame = total - 1
        end
      end
    end
  end
  return busy
end

-- Builds the ring again where the entry the player put back is gone, as
-- combining two takes them, and rests it on what the script asked for.
local function refresh(state)
  local content = state.list[state.view.current]
  if trx.inventory.has(content.object) or state.ring_type == Ring.OPTION then
    return
  end
  fill(state, state.ring_type, state.view.current)
  local view = state.view
  view.radius:set(motion.RING.radius)
  view.camera_y:set(motion.RING.camera_height)
  rest_on_request(state)
  view.rot:set(view:rest_angle())
end

local function is_passport(state)
  local content = state.list[state.view.current]
  return content ~= nil and content.object == objects.PASSPORT_CLOSED
end

-- The frame the picked entry rests open on, or nil while it moves.
local function rest_frame_of(state)
  if state.status ~= Status.SELECTED then
    return nil
  end
  local anim = state.anims[state.view.current]
  if anim == nil or anim.frame ~= anim.goal then
    return nil
  end
  return anim.frame
end

-- Moves the ring on by what ended this tick and what waits on nothing but
-- time.
local function run(state)
  local view = state.view
  local status = state.status
  for _, anim in ipairs(state.anims) do
    anim.last = anim.frame
  end

  if status == Status.SELECTED then
    if is_passport(state) and not state.passport_open then
      state.passport_open = true
    end
    offer(state)
    if state.screen ~= nil and trx.game.is_leaving then
      state.chosen = state.list[view.current].object
      keep_place(state)
      local screen = state.screen
      state.screen = nil
      screen:cancel()
      close_item(state, true)
    end
  elseif status == Status.CLOSING_ITEM then
    if not animate(state) then
      local index = view.current
      if state.passport_open then
        state.passport_open = false
        state.anims[index].frame = 0
        state.anims[index].last = 0
      end
      if state.close_to == Status.DESELECTING then
        refresh(state)
        trx.sound.play(samples.MENU_SPINOUT)
        set_status(state, Status.DESELECTING, Motion.DESELECT)
      else
        set_status(state, Status.EXITING, Motion.DESELECT)
      end
    end
  elseif status == Status.EXITING and not view:is_moving() then
    close(state)
  end
  -- Putting an entry back can build the ring again around fewer entries.
  view = state.view

  if state.status == Status.SELECTED then
    local entry = view.entries[view.current]
    local spec = state.list[view.current].spec
    if wrap(entry.y_rot:latest()) == wrap(spec.y_rot_sel or 0) then
      state.busy = animate(state)
    else
      state.busy = false
    end
  end

  local spin = Spin.IDLE
  if
    state.status == Status.OPENING
    or state.status == Status.DONE
    or state.status == Status.FADING_OUT
  then
    spin = Spin.STILL
  elseif HELD[state.status] then
    spin = Spin.HELD
  elseif state.turning_held and #state.list > 1 then
    spin = Spin.STILL
  end

  if state.status ~= Status.DONE and not state.waiting then
    if view:tick(spin) then
      local ended = state.status
      if ended == Status.OPENING or ended == Status.DESELECTING then
        state.status = Status.OPEN
      elseif ended == Status.SELECTING then
        state.status = Status.SELECTED
      elseif ended == Status.LEAVING then
        keep_place(state)
        local place = places[state.next_ring]
        local stale = stale_place(state.next_ring, place, state.mode)
        fill(
          state,
          state.next_ring,
          stale ~= nil and stale.index or place ~= nil and place.index or 1
        )
        state.stale = stale
        set_status(state, Status.OPENING, Motion.ARRIVE)
      elseif ended == Status.CLOSING then
        if state.after_close == Status.FADING_OUT then
          state.status = Status.FADING_OUT
          state.fades:leave({
            ticks = BLACK_TICKS,
            hold = BLACK_HOLD_TICKS,
            on_done = function()
              finish(state)
            end,
          })
        else
          finish(state)
        end
      end
    end
  end

  -- The compass needle swings while Lara's facing shows, which it does over
  -- the play and once the ring has opened.
  if
    state.mode ~= trx.ui.MenuMode.TITLE
    and not state.waiting
    and state.status ~= Status.OPENING
  then
    local index = index_of(state.list, objects.COMPASS_OPTION)
    if index ~= nil then
      swing_needle(state, view.entries[index])
    end
  end

  if state.status ~= Status.DONE then
    if SHOWN[state.status] then
      state.actions = trx.inventory.actions(state.list[view.current].object)
    end
    say(state)
    ring.set_rest_frame(rest_frame_of(state))
  end
end

-- Reads the keys the player pressed while the ring is on top.
local function control(state, keys)
  if state.status == Status.OPEN then
    if is_forced_mode(state.mode) and not state.passport_shown then
      pick(state, false)
      return
    end
    control_open(state, keys)
  elseif state.status == Status.SELECTED and state.unhandled then
    control_entry(state, keys, state.busy)
  end

  -- The title opens on the next demo once nobody has touched it for a while.
  if state.mode == trx.ui.MenuMode.TITLE then
    if trx.input.is_anything_held() then
      state.idle_ticks = 0
    elseif state.status == Status.OPEN then
      state.idle_ticks = state.idle_ticks + 1
      if state.idle_ticks >= state.demo_ticks and state.demo_ticks > 0 then
        state.demo_needed = true
        state.chosen = nil
        trx.sound.play(samples.MENU_SPINOUT)
        close(state)
      end
    end
  end
end

-------------------------------------------------------------------------------
-- The screen
-------------------------------------------------------------------------------

-- How many ticks the title waits for the player before it plays a demo, or
-- 0 where it plays none.
local function demo_ticks()
  local ok, enabled = pcall(trx.config.get, "gameplay.enable_demo")
  if not ok or not enabled or #trx.game.demos == 0 then
    return 0
  end
  local delay_ok, delay = pcall(trx.config.get, "flow.demo_delay")
  return math.floor((delay_ok and delay or 0) * trx.game.LOGIC_FPS)
end

-- The ring a mode opens on, and the entry it rests on.
local function first_ring(state)
  local MenuMode = trx.ui.MenuMode
  local mode = state.mode
  if mode == MenuMode.KEYS then
    local list = entries.contents(Ring.KEYS, mode)
    return Ring.KEYS, resting_place(Ring.KEYS, list, 1)
  elseif mode ~= MenuMode.GAME then
    return Ring.OPTION, 1
  end
  local list = entries.contents(Ring.MAIN, mode)
  if #list > 0 then
    return Ring.MAIN, resting_place(Ring.MAIN, list, 1)
  end
  return Ring.OPTION, 1
end

---@param ctx trx.ui.MenuContext
local function open(ctx)
  local MenuMode = trx.ui.MenuMode
  ---@type table<string, any>
  local state = {
    ctx = ctx,
    mode = ctx.mode,
    status = Status.OPENING,
    needle = 0,
    needle_speed = 0,
    idle_ticks = 0,
    demo_ticks = ctx.mode == MenuMode.TITLE and demo_ticks() or 0,
    turning_held = false,
    passport_open = false,
    passport_shown = false,
    kept = {},
  }

  if
    ctx.mode == MenuMode.KEYS and not entries.is_available(Ring.KEYS, ctx.mode)
  then
    return nil
  end

  -- The ring opens where it was left, and the keys ring is sent back to its
  -- first entry where the duplication glitch is fixed and the player did not
  -- ask the rings to remember.
  if fixes_duplication() and not is_remembered(Ring.KEYS) then
    places[Ring.KEYS] = nil
  end
  places[Ring.MAIN] = nil
  places[Ring.OPTION] = nil

  if ctx.object ~= nil then
    ring.request(ctx.object)
  end
  local ring_type, current = first_ring(state)
  if #entries.contents(ring_type, ctx.mode) == 0 then
    return nil
  end
  fill(state, ring_type, current)
  rest_on_request(state)
  ring.attach(state)

  state.fades = fade.screen(ctx, {
    enabled = function()
      return fade.enabled("ui.inventory_fade_effects")
    end,
  })

  local function spin_in()
    state.waiting = false
    trx.sound.play(samples.MENU_SPININ)
    state.view:start(Motion.OPEN)
  end

  state.style = trx.config.get("ui.inventory_background_style")
  state.backdrop = trx.ui.tween(0):to(1, state.fades:ticks(FADE_TICKS))

  if ctx.mode == MenuMode.TITLE then
    trx.overlay.show_version(true)
    -- The title comes in from black before its ring spins in.
    state.waiting = true
    state.hidden = true
    state.fades:enter({
      ticks = FADE_TICKS,
      on_done = function()
        state.hidden = false
        spin_in()
      end,
    })
  else
    spin_in()
  end

  local ticking = timer.every(1, function()
    run(state)
  end, { clock = timer.Clock.UI })
  -- The ring holds still as the game exits, under the fade to black.
  local exiting = trx.events.on_exit(function()
    ticking:cancel()
  end)

  ctx:on_end(function()
    ticking:cancel()
    exiting:detach()
    if state.screen ~= nil then
      local screen = state.screen
      state.screen = nil
      screen:cancel()
    end
    local content = state.list[state.view.current]
    if content ~= nil then
      left_on[state.ring_type] = content.object
    end
    show_heading(state, false)
    show_arrows(state, false)
    trx.overlay.set_caption()
    ring.set_hint({})
    ring.set_count(nil)
    ring.show_health_bar(false)
    ring.set_rest_frame(nil)
    trx.overlay.show_version(false)
    ring.detach(state)
  end)

  return ctx:push({
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
end

-- Draws the game's menu as the inventory ring.
function M.setup()
  trx.events.on_game_start(forget)
  trx.events.on_level_unload(function()
    generic = nil
  end)
  trx.ui.screens.define(trx.ui.Screen.MENU, open)
end

return M
