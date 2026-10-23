-- The readouts of the in-game overlay: where Lara is and what she is doing,
-- and the frames drawn in the last second.

local accent = require("common.ui.accent")

trx.locale.declare({
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

local M = {}

-- Builds the readouts and places them. `state` holds what the rest of the
-- overlay reads too.
---@param state common.ui.hud.overlay.Shared
function M.setup(state)
  local ui = trx.ui
  local signal = trx.signal
  local lara = trx.lara
  local language = signal.config("language")

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
        text = signal.polled_while(shown, function()
          return accent.text(read(), accent.Role.READOUT)
        end, ""),
        visible = shown,
      })
    end

    local function lara_readout(shown, read)
      return readout(shown, function()
        local item = lara.item
        if item == nil then
          return ""
        end
        return read(item)
      end)
    end

    -- The key is named at the point of use, so the shipped strings can see it.
    local function title(shown, read)
      return ui.widgets.Label({
        text = language:map(read),
        visible = shown,
      })
    end

    local pos_shown = debuggable & signal.config("debug.enable_debug_pos")
    local anim_shown = debuggable & signal.config("debug.enable_debug_anim")
    local camera_shown = debuggable
      & signal.config("debug.enable_debug_camera")

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
      lara_readout(pos_shown, function(item)
        local x, y, z = tiles(item.pos)
        return string.format("%d, %d, %d / %d", x, y, z, item.room_num)
      end),

      lara_readout(pos_shown, function(item)
        local r = item.rot
        return string.format(
          "%d\u{00B0}, %d\u{00B0}, %d\u{00B0}",
          r.x * 360 // 65536,
          r.y * 360 // 65536,
          r.z * 360 // 65536
        )
      end),

      lara_readout(pos_shown, function(item)
        local it = lara.vehicle or item
        return string.format("%d, %d", it.speed, it.fall_speed)
      end),

      readout(pos_shown, function()
        return string.format(
          "%d / %d / %d",
          lara.interact_item_num,
          lara.is_interact_moving and 1 or 0,
          lara.interact_move_count
        )
      end),

      lara_readout(anim_shown, function(item)
        return string.format("%d, %d", item.anim_num, item.frame_num)
      end),

      lara_readout(anim_shown, function(item)
        return string.format(
          "%d, %d (%d)",
          item.anim_state,
          item.goal_anim_state,
          trx.catalog.to_slot(
            trx.catalog.Context.OBJECTS,
            lara.animation_object
          )
        )
      end),

      readout(anim_shown, function()
        return string.format(
          "%d, %d (%d)",
          lara.left_arm_anim_num,
          lara.left_arm_frame_num,
          lara.flare_control and 1 or 0
        )
      end),

      readout(anim_shown, function()
        return string.format(
          "%d, %d",
          lara.right_arm_anim_num,
          lara.right_arm_frame_num
        )
      end),

      readout(camera_shown, function()
        local x, y, z = tiles(trx.camera.pos)
        return string.format("%d, %d, %d / %d", x, y, z, trx.camera.room_num)
      end),

      readout(camera_shown, function()
        local x, y, z = tiles(trx.camera.target_pos)
        return string.format(
          "%d, %d, %d / %d",
          x,
          y,
          z,
          trx.camera.target_room_num
        )
      end),
    }

    -- The raw numbers behind the readouts, down the other side of the screen.
    local lines = {
      lara_readout(pos_shown, function(item)
        local p = item.pos
        return string.format("%d, %d, %d", p.x, p.y, p.z)
      end),

      lara_readout(pos_shown, function(item)
        local r = item.rot
        return string.format("%d, %d, %d", r.x, r.y, r.z)
      end),

      readout(camera_shown, function()
        local p = trx.camera.pos
        return string.format("%d, %d, %d", p.x, p.y, p.z)
      end),

      readout(camera_shown, function()
        local p = trx.camera.target_pos
        return string.format("%d, %d, %d", p.x, p.y, p.z)
      end),

      -- The mark that says nothing can hurt her.
      ui.widgets.Label({
        text = language:map(function()
          return trx.locale.get("general/overlay/debug_immune")
        end),
        scale = 0.8,
        visible = debuggable
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
    local fps = signal.polled_while(shown, function()
      return trx.game.measured_fps
    end, 0)

    ui.regions.place(
      ui.Region.TOP_LEFT,
      ui.widgets.Label({
        text = fps:map(function(value)
          return string.format("%d FPS", value)
        end),
        visible = shown,
      })
    )
  end
end

return M
