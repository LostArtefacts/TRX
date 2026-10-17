-- What a settings row does that the setting itself cannot say: how far a press
-- moves a number, the order an enum's values cycle in, when the row is greyed
-- out or left out, and any row that is stepped in a way of its own.
--
-- Usage:
--   local handlers = require("common.ui.settings.handlers")
--   local handler = handlers.get("audio.music_volume")

local M = {}

-- Reads a setting, or nil where the game lacks it, as a game does whose
-- scripts leave out the module that declares it.
local function get(key)
  local ok, value = pcall(trx.config.get, key)
  if ok then
    return value
  end
  return nil
end

local function weapon_available(id)
  local weapon = trx.weapons.get(id)
  return weapon ~= nil and weapon.is_available
end

local function any_weapon(test)
  for _, weapon in ipairs(trx.weapons.all) do
    if weapon.is_available and test(weapon) then
      return true
    end
  end
  return false
end

local function loaded(object)
  local entry = trx.objects[object]
  return entry ~= nil and entry.loaded
end

local function bars()
  return get("ui.show_bars")
end

local function sprint()
  return get("gameplay.enable_sprint")
end

local function sprint_bar()
  return bars() and sprint()
end

local function enemy_bar()
  return bars() and get("ui.enemy_healthbar_show_mode") ~= "never"
end

local function ally_bar()
  return bars()
    and get("ui.enemy_healthbar_show_mode") == "always"
    and get("gameplay.enable_ally_targeting")
end

local function crawl()
  return get("gameplay.enable_crawling")
end

local function fades()
  return get("visuals.enable_fade_effects")
end

local function flare()
  return any_weapon(function(weapon)
    return weapon.kind == trx.weapons.Kind.FLARE
  end)
end

local function machine_gun()
  return any_weapon(function(weapon)
    return weapon.is_machine_gun
  end)
end

local function touch()
  return trx.input.has_touchscreen
end

local function touch_on()
  return touch() and get("input.enable_touch_controls")
end

local function ps1_bars()
  return trx.ui.is_ps1_bar(trx.ui.BarType.LARA_HP)
end

local function pc_bars()
  return not ps1_bars()
end

local function music_in_inventory()
  return get("audio.enable_music_in_inventory")
end

local TEXTURE_FILTER_ORDER = { "point", "bilinear" }

-- A level's loading image is listed so that a settings file naming it still
-- has a row to show, but it is never offered.
local BACKGROUND_STYLE_ORDER = {
  "none",
  "transparent",
  "transparent_dark",
  "black",
  "pattern_static",
  "pattern_wave",
  "image",
  "monochrome",
  "monochrome_cool",
  "monochrome_warm",
}

local function background_style_offered(value)
  if value == "pattern_static" or value == "pattern_wave" then
    return loaded(trx.catalog.objects.INV_BACKGROUND)
  end
  return value ~= "image"
end

-- A volume steps as any number does, with a sound that says how loud it now
-- is.
local function volume(extra)
  local handler = {
    slow = 1,
    fast = 10,
    after_change = function()
      trx.sound.play(trx.catalog.samples.MENU_PASSPORT)
    end,
  }
  for name, value in pairs(extra or {}) do
    handler[name] = value
  end
  return handler
end

-- A row that steps through a list of choices rather than through the values
-- the setting itself lists. A value no longer on the list steps back to the
-- first choice.
local function choices(list, format, refuse_alone)
  local function index_of(value)
    for i, choice in ipairs(list()) do
      if value ~= nil and trx.strings.equivalent(choice, value) then
        return i
      end
    end
    return nil
  end
  return {
    key_format = format,
    can_change = function(key, dir)
      local all = list()
      local index = index_of(get(key))
      if index == nil then
        return true
      end
      if refuse_alone and #all < 2 then
        return false
      end
      return index + dir >= 1 and index + dir <= #all
    end,
    change = function(key, dir)
      local all = list()
      local index = index_of(get(key))
      local target = index == nil and 1 or index + dir
      if all[target] == nil then
        return false
      end
      trx.config.set(key, all[target])
      return true
    end,
  }
end

local function language_name(code)
  return trx.locale.language_name(code) or code
end

local VOICE_AUTO = "auto"
local VOICE_RETAIL = "retail"

local function voice_choices()
  local list = { VOICE_AUTO, VOICE_RETAIL }
  for _, code in ipairs(trx.locale.voice_languages()) do
    list[#list + 1] = code
  end
  return list
end

local function format_voice(code)
  if code == nil or code == "" or trx.strings.equivalent(code, VOICE_AUTO) then
    return trx.locale.get("settings/audio.media_language/auto")
  elseif trx.strings.equivalent(code, VOICE_RETAIL) then
    return trx.locale.get("settings/audio.media_language/retail")
  end
  return language_name(code)
end

local HANDLERS = {
  ["ui.text_scale"] = { slow = 1, fast = 5 },
  ["ui.pickup_scale"] = { slow = 1, fast = 5 },
  ["visuals.fog_start"] = { slow = 1, fast = 10 },
  ["visuals.fog_end"] = { slow = 1, fast = 10 },
  ["visuals.fov"] = { slow = 1, fast = 5 },
  ["gameplay.start_lara_hitpoints"] = { slow = 10, fast = 100 },
  ["gameplay.lara_damage_scale"] = { slow = 5, fast = 25 },
  ["gameplay.enemy_health_scale"] = { slow = 5, fast = 25 },
  ["gameplay.maximum_save_slots"] = { slow = 1, fast = 10 },
  ["gameplay.maximum_quick_save_slots"] = { slow = 1, fast = 10 },
  ["rendering.fps"] = { slow = 30, fast = 30 },
  ["rendering.anisotropy_filter"] = { slow = 10, fast = 100 },
  ["visuals.game_brightness"] = { slow = 1, fast = 5 },
  ["visuals.ui_brightness"] = { slow = 1, fast = 5 },
  ["visuals.background_brightness"] = { slow = 1, fast = 5 },
  ["visuals.gamma"] = { slow = 10, fast = 50 },
  ["rendering.upscaling_factor"] = { slow = 1, fast = 1 },
  -- The two ways of anti-aliasing the scene are alternatives, and asking for
  -- both costs what the one that wins would have.
  ["rendering.supersampling_factor"] = {
    slow = 1,
    fast = 1,
    available = function()
      return get("rendering.multisampling_factor") <= 1
    end,
  },
  ["rendering.multisampling_factor"] = {
    slow = 1,
    fast = 1,
    available = function()
      return get("rendering.supersampling_factor") <= 1
    end,
  },
  ["rendering.borders"] = { slow = 1, fast = 5 },
  ["rendering.aspect_mode"] = { order = { "4:3", "16:9", "16:10", "any" } },
  ["rendering.texture_filter"] = { order = TEXTURE_FILTER_ORDER },
  ["rendering.ui_filter"] = { order = TEXTURE_FILTER_ORDER },
  ["rendering.upscaling_filter"] = { order = TEXTURE_FILTER_ORDER },
  ["rendering.fmv_filter"] = { order = TEXTURE_FILTER_ORDER },
  ["gameplay.ally_hostility_policy"] = { order = { "individual", "shared" } },
  ["gameplay.enable_idle_pose_camera"] = {
    available = function()
      return get("gameplay.idle_pose_timeout") > 0
    end,
  },
  ["gameplay.jump_lock_mode"] = {
    available = function()
      return get("gameplay.enable_tr2_jumping")
    end,
  },
  ["gameplay.enable_responsive_crawl"] = { available = crawl },
  ["gameplay.enable_crawl_jump"] = { available = crawl },
  ["gameplay.enable_crawl_tilt"] = { available = crawl },
  ["gameplay.enable_crouch_roll"] = { available = crawl },
  ["gameplay.enable_toggle_crouch"] = { available = crawl },
  ["gameplay.enable_responsive_sprint"] = { available = sprint },
  ["gameplay.enable_toggle_sprint"] = { available = sprint },
  ["input.enable_touch_controls"] = {
    available = touch,
    can_change = touch,
  },
  ["input.touch_opacity"] = { available = touch_on, can_change = touch_on },
  ["input.touch_button_scale"] = {
    available = touch_on,
    can_change = touch_on,
  },
  ["input.touch_dpad_scale"] = { available = touch_on, can_change = touch_on },
  ["input.touch_dpad_deadzone"] = {
    available = touch_on,
    can_change = touch_on,
  },
  ["gameplay.fix_flare_throw_priority"] = { available = flare },
  ["gameplay.fix_m16_accuracy"] = { available = machine_gun },
  ["gameplay.m16_aim_mode"] = { available = machine_gun },
  ["gameplay.fix_wade_sprint"] = {
    available = function()
      return get("gameplay.enable_wading") and sprint()
    end,
  },
  ["gameplay.fix_wade_wall_hit"] = {
    available = function()
      return get("gameplay.enable_wading")
    end,
  },
  ["gameplay.fix_underwater_crawl"] = { available = crawl },
  ["gameplay.fix_step_glitch"] = {
    available = function()
      return get("gameplay.enable_smooth_wall_deflect")
    end,
  },
  ["gameplay.fix_free_flare_glitch"] = { available = flare },
  ["gameplay.enable_bouncy_grenades"] = {
    available = function()
      return weapon_available(trx.catalog.weapons.GRENADE)
    end,
  },
  ["gameplay.harpoon_recoil"] = {
    slow = 1,
    fast = 1,
    available = function()
      return weapon_available(trx.catalog.weapons.HARPOON)
    end,
  },
  ["debug.enable_endless_flare_time"] = { available = flare },
  ["gameplay.projectile_area_damage"] = {
    available = function()
      return any_weapon(function(weapon)
        return weapon.is_launcher
      end)
    end,
  },
  ["ui.inventory_background_style"] = {
    order = BACKGROUND_STYLE_ORDER,
    offered = background_style_offered,
  },
  ["ui.inventory_fade_effects"] = { available = fades },
  ["ui.pause_background_style"] = {
    order = BACKGROUND_STYLE_ORDER,
    offered = background_style_offered,
  },
  ["ui.pause_fade_effects"] = { available = fades },
  ["ui.stats_background_style"] = {
    order = BACKGROUND_STYLE_ORDER,
    offered = background_style_offered,
  },
  ["ui.stats_fade_effects"] = { available = fades },
  ["ui.background_blur"] = { slow = 5, fast = 10 },
  ["visuals.enable_exit_fade_effects"] = { available = fades },
  ["ui.bar_scale"] = { slow = 1, fast = 5, available = bars },
  ["ui.bar_look"] = { available = bars },
  ["ui.enable_smooth_bars"] = { available = bars },
  ["ui.enable_bar_side_borders"] = {
    available = function()
      return bars() and ps1_bars()
    end,
  },
  ["ui.enable_bar_flashing"] = { available = bars },
  ["ui.healthbar_color"] = { available = bars, visible = pc_bars },
  ["ui.healthbar_color_ps1"] = { available = bars, visible = ps1_bars },
  ["ui.healthbar_poison_color"] = { available = bars, visible = pc_bars },
  ["ui.healthbar_poison_color_ps1"] = { available = bars, visible = ps1_bars },
  ["ui.airbar_color"] = { available = bars, visible = pc_bars },
  ["ui.airbar_color_ps1"] = { available = bars, visible = ps1_bars },
  ["ui.sprintbar_color"] = { available = sprint_bar, visible = pc_bars },
  ["ui.sprintbar_color_ps1"] = { available = sprint_bar, visible = ps1_bars },
  ["ui.exposurebar_color"] = { available = bars, visible = pc_bars },
  ["ui.exposurebar_color_ps1"] = { available = bars, visible = ps1_bars },
  ["ui.enemy_healthbar_color"] = { available = enemy_bar, visible = pc_bars },
  ["ui.enemy_healthbar_color_ps1"] = {
    available = enemy_bar,
    visible = ps1_bars,
  },
  ["ui.enemy_healthbar_color_allies"] = {
    available = ally_bar,
    visible = pc_bars,
  },
  ["ui.enemy_healthbar_color_allies_ps1"] = {
    available = ally_bar,
    visible = ps1_bars,
  },
  ["ui.progressbar_color"] = {
    visible = function()
      return get("ui.show_loading_bar") and pc_bars()
    end,
  },
  ["ui.progressbar_color_ps1"] = {
    visible = function()
      return get("ui.show_loading_bar") and ps1_bars()
    end,
  },
  ["ui.enemy_healthbar_show_mode"] = {
    available = bars,
    order = { "never", "boss_only", "always" },
  },
  ["ui.healthbar_location"] = { available = bars },
  ["ui.airbar_location"] = { available = bars },
  ["ui.sprintbar_location"] = { available = sprint_bar },
  ["ui.exposurebar_location"] = { available = bars },
  ["ui.enemy_healthbar_location"] = { available = enemy_bar },
  -- Pickup mode counts crystals and nothing else, so it shows them however
  -- this is set.
  ["ui.stats.show_crystals"] = {
    available = function()
      local mode = get("gameplay.save_crystal_mode")
      return trx.stats.has_crystals() and mode ~= "off" and mode ~= "pickup"
    end,
  },
  ["visuals.fog_color"] = {
    available = function()
      return not get("visuals.fog_transparency")
    end,
  },
  ["visuals.enable_fog_bulbs"] = {
    visible = function()
      return trx.game.tr_version == 4
    end,
  },
  ["visuals.breeze_mode"] = {
    available = function()
      return get("visuals.braid_status") ~= "off"
        or get("visuals.enable_weather")
        or get("visuals.enable_droplets")
    end,
  },
  ["visuals.sunglasses_reflective"] = {
    available = function()
      return get("visuals.enable_reflections")
    end,
  },
  ["visuals.enable_ps1_crystals"] = {
    available = function()
      return get("gameplay.save_crystal_mode") ~= "off"
    end,
  },
  ["visuals.shadow_type"] = {
    offered = function(value)
      return value ~= "sprite" or loaded(trx.catalog.objects.SHADOW)
    end,
  },
  -- TR3 and TR4 always draw the gun glow.
  ["visuals.enable_gun_glow"] = {
    visible = function()
      return trx.game.tr_version <= 2
    end,
  },
  ["audio.master_volume"] = volume(),
  ["audio.sound_volume"] = volume(),
  ["audio.music_volume"] = volume(),
  ["audio.inventory_music_volume"] = volume({
    available = music_in_inventory,
  }),
  ["audio.underwater_music_volume"] = volume(),
  ["audio.ambient_volume"] = volume(),
  ["audio.inventory_ambient_volume"] = volume({
    available = music_in_inventory,
  }),
  ["audio.underwater_ambient_volume"] = volume(),
  ["audio.cutscene_volume"] = volume(),
  ["audio.fmv_volume"] = volume(),
  ["language"] = choices(trx.locale.text_languages, language_name, true),
  ["audio.media_language"] = choices(voice_choices, format_voice, false),
}

local NONE = {}

-- What a setting's row does beyond the setting itself. A setting that needs
-- nothing of its own gets an empty handler.
--
-- A handler's fields, each optional:
--   slow, fast    how far a press moves a number, with and without fine
--                 adjustment
--   order         the order an enum's values cycle in
--   offered(v)    whether an enum value can be picked now
--   available()   whether the row can be changed, rather than greyed out
--   visible()     whether the row is shown at all
--   key_format(v) what the row prints for a value
--   can_change(key, dir), change(key, dir)
--                 a row that steps in a way of its own
--   takes_over(key, dir)
--                 a row whose script may take a press over, leaving the
--                 setting to the script
--   after_change() what follows a change the player makes
---@param key string
---@return table
function M.get(key)
  return HANDLERS[key] or NONE
end

-- Calls one of a declared row's functions. A function that raises is logged
-- and answered as though the row had none, so that a broken script does not
-- stop the dialog.
local function call(key, row, name, ...)
  local ok, result = pcall(row[name], ...)
  if not ok then
    trx.log.error(
      ("settings row %s: %s handler failed: %s"):format(key, name, result)
    )
    return false, nil
  end
  return true, result
end

local function predicate(key, row, name)
  if row[name] == nil then
    return nil
  end
  return function(_, dir)
    local ok, result = call(key, row, name, get(key), dir)
    return not ok or result == true
  end
end

-- The handler of a row that a script declared with its setting.
---@param key string
---@param row trx.config.Row
---@return table
function M.from_row(key, row)
  local available = predicate(key, row, "is_available")
  local visible = predicate(key, row, "is_visible")
  local handler = {
    slow = row.delta_slow,
    fast = row.delta_fast,
    can_change = predicate(key, row, "can_change_value"),
    available = available and function()
      return available()
    end,
    visible = visible and function()
      return visible()
    end,
  }
  if row.format_value ~= nil then
    handler.key_format = function(value)
      local ok, text = call(key, row, "format_value", value)
      return ok and text ~= nil and tostring(text) or nil
    end
  end
  if row.request_change_value ~= nil then
    handler.takes_over = function(_, dir)
      local ok, taken = call(key, row, "request_change_value", get(key), dir)
      return ok and taken == true
    end
  end
  return handler
end

-- Gives the rows that scripts declared the handlers that their declarations
-- describe.
function M.add_declared()
  for _, entry in ipairs(trx.config.declared_rows()) do
    HANDLERS[entry.key] = M.from_row(entry.key, entry.row)
  end
end

return M
