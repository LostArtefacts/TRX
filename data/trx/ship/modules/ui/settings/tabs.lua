-- Which settings each tab of the settings dialogs holds, and in what order.
-- A tab lists the settings of every game; a game shows the ones it has. A
-- blank line between two groups of rows is only for the reader.
--
-- Usage:
--   local tabs = require("common.ui.settings.tabs")
--   for _, key in ipairs(tabs.keys("sound_volume")) do ... end

local M = {}

local TABS = {
  gameplay_general = {
    "gameplay.enable_legal",
    "gameplay.enable_credits",
    "gameplay.enable_fmv",
    "gameplay.intro_fmv_mode",
    "gameplay.enable_demo",
    "gameplay.enable_cinematics",
    "gameplay.enable_cinematic_skips",
    "gameplay.enable_cutscenes",
    "gameplay.loading_screens",

    "gameplay.game_modes_policy",
    "gameplay.enable_play_previous_levels",
    "gameplay.save_crystal_mode",

    "gameplay.enable_auto_item_selection",
    "gameplay.enable_item_examining",
    "gameplay.enable_binoculars",

    "gameplay.enable_compass_stats",
    "gameplay.enable_total_stats",

    "gameplay.enable_timer_in_inventory",
    "gameplay.pause_on_focus_lost",

    "gameplay.maximum_save_slots",
    "gameplay.maximum_quick_save_slots",
    "gameplay.enable_enhanced_saves",
    "gameplay.enable_bouncy_grenades",
    "gameplay.remember_gun_status",
    "gameplay.ring_memory_mode",

    "gameplay.restore_ps1_enemies",
    "gameplay.change_pierre_spawn",
    "gameplay.disable_trex_collision",
    "gameplay.enable_enemy_rotation",
    "gameplay.enable_body_bags",
    "gameplay.enable_killer_pushblocks",
    "gameplay.enable_boulder_shake",
    "gameplay.enable_ally_kill_count",
    "gameplay.enable_ally_targeting",
    "gameplay.enable_environment_kill_count",
    "gameplay.ally_hostility_policy",
    "gameplay.creature_drown_policy",
  },
  gameplay_controls = {
    "input.enable_responsive_passport",
    "gameplay.enable_walk_to_items",
    "gameplay.enable_snap_interactions",
    "gameplay.enable_fast_pickups",
    "gameplay.enable_multiple_pickups",
    "gameplay.enable_continuous_pushblocks",
    "input.enable_tr3_sidesteps",
    "gameplay.enable_swamp_sidestepping",
    "gameplay.enable_monkey_bar_edge_deflect",
    "gameplay.enable_monkey_bar_shimmy_turn",
    "input.quick_guns_mode",
    "gameplay.enable_underwater_auto_draw",
    "gameplay.look_mode",
    "gameplay.enable_manual_camera",
    "gameplay.idle_pose_timeout",
    "gameplay.enable_idle_pose_camera",

    "gameplay.enable_tr2_jumping",
    "gameplay.jump_lock_mode",
    "gameplay.enable_jump_twists",
    "gameplay.enable_crawling",
    "gameplay.enable_responsive_crawl",
    "gameplay.enable_crawl_jump",
    "gameplay.enable_crawl_tilt",
    "gameplay.enable_crouch_roll",
    "gameplay.enable_toggle_crouch",
    "gameplay.enable_sprint",
    "gameplay.enable_responsive_sprint",
    "gameplay.enable_toggle_sprint",
    "gameplay.enable_alternative_turns",
    "gameplay.enable_slide_to_run",
    "gameplay.enable_back_slope_stumble",
    "gameplay.enable_lean_jumping",
    "gameplay.enable_slow_ledge_swing",
    "gameplay.enable_swing_cancel",
    "gameplay.enable_controlled_drops",
    "gameplay.enable_ledge_jumps",
    "gameplay.enable_corner_shimmying",
    "gameplay.enable_fast_shimmying",
    "gameplay.enable_fast_pull_up",
    "gameplay.enable_smooth_wall_deflect",
    "gameplay.enable_soft_statics",
    "gameplay.enable_step_roll_boost",
    "gameplay.enable_uw_roll",
    "gameplay.enable_tr2_swimming",
    "gameplay.enable_tr2_swim_cancel",
    "gameplay.enable_wading",

    "gameplay.target_mode",
    "gameplay.target_change_mode",
    "gameplay.m16_aim_mode",

    "gameplay.enable_inverted_look",
    "gameplay.camera_speed",
    "input.enable_buffering_func_keys",
    "input.enable_buffering_inventory",

    "input.enable_controller",

    "input.enable_touch_controls",
    "input.touch_opacity",
    "input.touch_button_scale",
    "input.touch_dpad_scale",
    "input.touch_dpad_deadzone",
  },
  gameplay_mods = {
    "gameplay.enable_cheats",
    "gameplay.enable_console",

    "gameplay.harpoon_recoil",

    "gameplay.start_lara_hitpoints",
    "gameplay.lara_damage_scale",
    "gameplay.enemy_health_scale",
    "gameplay.disable_healing_between_levels",
    "gameplay.disable_medpacks",
    "gameplay.disable_extra_guns",
    "debug.enable_endless_sprint",
    "debug.enable_endless_flare_time",
    "gameplay.projectile_area_damage",
  },
  gameplay_fixes = {
    "gameplay.fix_floor_data_issues",
    "visuals.fix_texture_issues",
    "visuals.fix_smoke_emitters",
    "visuals.fix_item_rots",

    "visuals.fix_animated_sprites",
    "gameplay.fix_bridge_collision",

    "gameplay.fix_walk_run_jump",
    "gameplay.fix_flare_throw_priority",

    "gameplay.fix_m16_accuracy",

    "gameplay.fix_descending_glitch",
    "gameplay.fix_falling_glitch",

    "gameplay.fix_wall_geometry",
    "gameplay.wall_glitch_mode",

    "gameplay.fix_wade_sprint",
    "gameplay.fix_wade_wall_hit",
    "gameplay.fix_water_exit",
    "gameplay.fix_underwater_crawl",
    "gameplay.fix_qwop_glitch",
    "gameplay.fix_step_glitch",
    "gameplay.fix_item_duplication_glitch",
    "gameplay.fix_lara_pickup_embed",
    "gameplay.fix_breakable_wall_clip",
    "gameplay.fix_free_flare_glitch",

    "gameplay.fix_alligator_ai",
    "gameplay.fix_bear_ai",
    "gameplay.fix_bird_start_pos",
    "gameplay.fix_enemy_route_delay",
    "gameplay.fix_monkey_pickup_priority",
    "gameplay.fix_pipeman_aim",
    "audio.fix_chainblock_secret_sound",
    "gameplay.fix_animated_spikes",
    "gameplay.fix_lift_collision",
  },
  graphic_visuals = {
    "visuals.fog_start",
    "visuals.fog_end",
    "visuals.fog_transparency",
    "visuals.fog_color",
    "visuals.enable_fog_bulbs",

    "visuals.water_color",

    "visuals.camera_mode",
    "visuals.enable_glide_cameras",
    "visuals.enable_photo_mode_collision",

    "visuals.fov",

    "visuals.enable_reflections",
    "visuals.enable_skybox",
    "visuals.enable_weather",
    "visuals.enable_poison_vision",
    "visuals.enable_footprints",
    "visuals.enable_droplets",
    "visuals.water_tint_mode",
    "visuals.blood_effects",
    "visuals.lara_outfit",
    "visuals.golden_lara",
    "visuals.braid_status",
    "visuals.sunglasses_mode",
    "visuals.sunglasses_reflective",
    "visuals.breeze_mode",
    "visuals.enable_3d_pickups",
    "gameplay.enable_pickup_aids",

    "visuals.enable_ps1_crystals",
    "visuals.enable_ps1_rain",

    "visuals.shadow_type",
    "visuals.enable_gun_lighting",
    "visuals.enable_gun_glow",
    "visuals.enable_fire_lighting",
    "visuals.enable_static_lighting",

    "visuals.enable_shotgun_flash",
  },
  graphic_ui = {
    "language",

    "ui.text_scale",
    "ui.pickup_scale",
    "ui.show_pickups_overlay",
    "ui.show_title_version",
    "ui.enable_wraparound",

    "ui.menu_style",

    "ui.inventory_background_style",
    "ui.inventory_fade_effects",
    "ui.instant_save_load_screen",
    "ui.pause_background_style",
    "ui.pause_fade_effects",
    "ui.stats_background_style",
    "ui.stats_fade_effects",
    "ui.background_blur",

    "visuals.enable_fade_effects",
    "visuals.enable_exit_fade_effects",

    "ui.ammo_counter_location",
  },
  graphic_ui_stats = {
    "ui.stats.style",
    "ui.stats.show_totals",
    "ui.stats.show_level_header",
    "ui.stats.show_time_taken",
    "ui.stats.show_secrets",
    "ui.stats.show_crystals",
    "ui.stats.show_pickups",
    "ui.stats.show_kills",
    "ui.stats.show_ammo",
    "ui.stats.show_medipacks_used",
    "ui.stats.show_distance_travelled",
    "ui.stats.show_deaths",
  },
  graphic_ui_bars = {
    "ui.show_bars",
    "ui.bar_scale",
    "ui.bar_look",
    "ui.enable_smooth_bars",
    "ui.enable_bar_side_borders",
    "ui.enable_bar_flashing",

    "ui.healthbar_color",
    "ui.healthbar_color_ps1",
    "ui.healthbar_poison_color",
    "ui.healthbar_poison_color_ps1",
    "ui.airbar_color",
    "ui.airbar_color_ps1",
    "ui.sprintbar_color",
    "ui.sprintbar_color_ps1",
    "ui.exposurebar_color",
    "ui.exposurebar_color_ps1",
    "ui.enemy_healthbar_color",
    "ui.enemy_healthbar_color_ps1",
    "ui.enemy_healthbar_color_allies",
    "ui.enemy_healthbar_color_allies_ps1",
    "ui.show_loading_bar",
    "ui.progressbar_color",
    "ui.progressbar_color_ps1",

    "ui.enemy_healthbar_show_mode",
    "ui.healthbar_location",
    "ui.airbar_location",
    "ui.sprintbar_location",
    "ui.exposurebar_location",
    "ui.enemy_healthbar_location",
  },
  graphic_rendering = {
    "window.is_fullscreen",

    "rendering.fps",
    "rendering.enable_trapezoid_filter",
    "rendering.texture_filter",
    "rendering.ui_filter",
    "rendering.fmv_filter",
    "rendering.anisotropy_filter",
    "rendering.enable_vsync",
    "visuals.game_brightness",
    "visuals.ui_brightness",
    "visuals.background_brightness",
    "visuals.gamma",

    "rendering.lighting_contrast",
    "rendering.lighting_curve",
    "rendering.sprite_lock_mode",

    "rendering.aspect_mode",
    "rendering.upscaling_filter",
    "rendering.upscaling_factor",
    "rendering.supersampling_factor",
    "rendering.multisampling_factor",
    "rendering.borders",
    "rendering.vertex_snap_mode",
    "rendering.enable_ps1_fog",
    "rendering.enable_affine_mapping",
    "rendering.dither_mode",
    "rendering.enable_palette_quantize",

    "rendering.screenshot_format",
  },
  sound_volume = {
    "audio.master_volume",
    "audio.sound_volume",
    "audio.music_volume",
    "audio.inventory_music_volume",
    "audio.underwater_music_volume",
    "audio.ambient_volume",
    "audio.inventory_ambient_volume",
    "audio.underwater_ambient_volume",
    "audio.cutscene_volume",
    "audio.fmv_volume",
  },
  sound_misc = {
    "audio.media_language",
    "audio.enable_music_in_menu",
    "audio.fix_secrets_killing_music",
    "audio.fix_speeches_killing_music",
    "audio.load_music_triggers",
    "audio.enable_ambient_crossfade",

    "audio.enable_music_in_inventory",
    "audio.enable_music_on_death",
    "audio.music_load_condition",
    "audio.enable_underwater_anim_sfx",
    "audio.underwater_muffle",
    "audio.mute_out_of_focus",

    "audio.enable_pitched_sounds",
    "audio.enable_ps1_sfx",
    "audio.enable_lara_mic",
    "audio.fix_sliding_sfx",
  },
}

-- Whether a tab of this name exists.
---@param name string
---@return boolean
function M.exists(name)
  return TABS[name] ~= nil
end

-- The settings a tab lists, in order, including ones the game does not have.
---@param name string
---@return string[]
function M.keys(name)
  return TABS[name] or {}
end

-- The last segment of a key, which is unique across the settings, so that a
-- whole path and a bare name find the same row.
local function leaf(key)
  return key:match("([^.]+)$")
end

local function find(rows, key)
  if key == nil then
    return nil
  end
  local wanted = leaf(key)
  for i, row in ipairs(rows) do
    if leaf(row.key) == wanted then
      return i
    end
  end
  return nil
end

---@class common.ui.settings.DeclaredRow
---@field key string The setting.
---@field before? string The setting the row sits directly above.
---@field after? string The setting the row sits directly below.

-- The settings a tab lists, with the rows that scripts declared placed among
-- them. A row sits beside the setting that its anchor names, and at the end
-- where it names none or one that the tab does not list. Rows declared against
-- one anchor read in the order they were declared in.
---@param name string
---@param declared common.ui.settings.DeclaredRow[] In the order of declaration.
---@return string[]
function M.arrange(name, declared)
  local rows = {}
  for i, key in ipairs(M.keys(name)) do
    rows[i] = { key = key }
  end
  for _, row in ipairs(declared) do
    local anchor = find(rows, row.before or row.after)
    local at = #rows + 1
    if anchor ~= nil and row.before ~= nil then
      at = anchor
    elseif anchor ~= nil then
      at = anchor + 1
      while rows[at] ~= nil and rows[at].declared do
        at = at + 1
      end
    end
    table.insert(rows, at, { key = row.key, declared = true })
  end
  local keys = {}
  for i, row in ipairs(rows) do
    keys[i] = row.key
  end
  return keys
end

return M
