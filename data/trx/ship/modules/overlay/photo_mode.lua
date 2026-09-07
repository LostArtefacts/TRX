-- The photo mode help panel: what each key does while photo mode is open.
--
-- The panel is built once and updated through signals, like the rest of the
-- overlay. The engine draws the red border around the picture in the game view.

local input = trx.input
local ui = trx.ui
local signal = trx.signal

trx.locale.declare({
  ["general/photo_mode/title_camera_pos"] = "Photo Mode",
  ["general/photo_mode/title_lara_pos"] = "Moving Lara",
  ["general/photo_mode/camera_move_prompt"] = "Move camera",
  ["general/photo_mode/camera_rotate_prompt"] = "Rotate camera",
  ["general/photo_mode/camera_roll_prompt"] = "Roll camera",
  ["general/photo_mode/camera_rotate_90_prompt"] = "Rotate camera 90°",
  ["general/photo_mode/camera_reset_prompt"] = "Reset camera",
  ["general/photo_mode/lara_move_prompt"] = "Move Lara",
  ["general/photo_mode/lara_rotate_prompt"] = "Rotate Lara",
  ["general/photo_mode/lara_roll_prompt"] = "Roll Lara",
  ["general/photo_mode/lara_rotate_90_prompt"] = "Rotate Lara 90°",
  ["general/photo_mode/lara_reset_prompt"] = "Reset Lara",
  ["general/photo_mode/fov_prompt"] = "Adjust FOV",
  ["general/photo_mode/change_lara_pose"] = "Change Lara's pose",
  ["general/photo_mode/advance_frame"] = "Advance frame",
  ["general/photo_mode/toggle_help"] = "Toggle help",
  ["general/photo_mode/snap_prompt"] = "Take picture",
})

local state = {
  language = signal.config("language"),
  open = trx.game.signals.is_photo_mode,
}

state.shown = state.open & signal.config("ui.enable_photo_mode_ui")

state.target = signal.polled(function()
  return trx.game.photo_mode_target
end)

state.can_pose = signal.polled(function()
  return trx.lara.can_pose
end)

-- Show a key glyph when the player's device has one, and hide the prompt when
-- there is no binding to draw.
local function has_glyph(role)
  return state.language:map(function()
    return input.has_glyph(role)
  end)
end

local function text(shown, read)
  return ui.widgets.Label({ text = state.language:map(read), shown = shown })
end

-- A prompt follows the current target, so the same row can say "move the
-- camera" or "move Lara". Each line supplies its own translated text.
local function per_target(camera, lara)
  return signal.combine(state.target, state.language, function(target)
    if target == trx.game.PhotoModeTarget.CAMERA then
      return camera()
    end
    return lara()
  end)
end

-------------------------------------------------------------------------------
-- the title, and the keys that switch between camera and Lara
-------------------------------------------------------------------------------

local title = ui.widgets.Stack({
  orientation = ui.Orientation.HORIZONTAL,
  align = ui.HAlign.DISTRIBUTE,
  spacing = 8,
  children = {
    ui.widgets.Label({
      text = per_target(function()
        return trx.locale.get("general/photo_mode/title_camera_pos")
      end, function()
        return trx.locale.get("general/photo_mode/title_lara_pos")
      end),
    }),
    ui.widgets.Label({ text = "\\{input step_left}\\{input step_right}" }),
  },
})

-------------------------------------------------------------------------------
-- the keys, and what each of them does
-------------------------------------------------------------------------------

-- Name both exit keys when both are bound, or just the one that is available.
local exit_key = signal.combine(state.language, function()
  local photo = input.has_glyph(input.Role.TOGGLE_PHOTO_MODE)
  local option = input.has_glyph(input.Role.OPTION)
  if photo and option then
    return "\\{input toggle_photo_mode}/\\{input option}"
  elseif photo then
    return "\\{input toggle_photo_mode}"
  end
  return "\\{input option}"
end)

local exit_shown = signal.combine(
  has_glyph(input.Role.TOGGLE_PHOTO_MODE),
  has_glyph(input.Role.OPTION),
  function(photo, option)
    return photo or option
  end
)

local keys = ui.widgets.Stack({
  children = {
    ui.widgets.Label({
      text = "\\{input camera_up}\\{input camera_down}"
        .. "\\{input camera_forward}\\{input camera_back}"
        .. "\\{input camera_left}\\{input camera_right}",
    }),
    ui.widgets.Label({
      text = "\\{input left}\\{input forward}\\{input back}\\{input right}",
    }),
    ui.widgets.Label({
      text = "\\{input slow}+\\{input camera_up}/\\{input camera_down}",
    }),
    ui.widgets.Label({ text = "\\{input roll}" }),
    ui.widgets.Label({ text = "\\{input look}" }),
    ui.widgets.Label({ text = "[\\{input slow}+]\\{input draw}" }),
    ui.widgets.Label({
      text = "[\\{input slow}+]\\{input fly_cheat}",
      shown = state.can_pose,
    }),
    ui.widgets.Label({ text = "[\\{input slow}+]\\{input pause}" }),
    ui.widgets.Label({ text = "\\{input toggle_ui}" }),
    ui.widgets.Label({ text = "\\{input action}" }),
    ui.widgets.Label({ text = exit_key, shown = exit_shown }),
  },
})

local actions = ui.widgets.Stack({
  children = {
    ui.widgets.Label({
      text = per_target(function()
        return trx.locale.get("general/photo_mode/camera_move_prompt")
      end, function()
        return trx.locale.get("general/photo_mode/lara_move_prompt")
      end),
    }),
    ui.widgets.Label({
      text = per_target(function()
        return trx.locale.get("general/photo_mode/camera_rotate_prompt")
      end, function()
        return trx.locale.get("general/photo_mode/lara_rotate_prompt")
      end),
    }),
    ui.widgets.Label({
      text = per_target(function()
        return trx.locale.get("general/photo_mode/camera_roll_prompt")
      end, function()
        return trx.locale.get("general/photo_mode/lara_roll_prompt")
      end),
    }),
    ui.widgets.Label({
      text = per_target(function()
        return trx.locale.get("general/photo_mode/camera_rotate_90_prompt")
      end, function()
        return trx.locale.get("general/photo_mode/lara_rotate_90_prompt")
      end),
    }),
    ui.widgets.Label({
      text = per_target(function()
        return trx.locale.get("general/photo_mode/camera_reset_prompt")
      end, function()
        return trx.locale.get("general/photo_mode/lara_reset_prompt")
      end),
    }),
    text(nil, function()
      return trx.locale.get("general/photo_mode/fov_prompt")
    end),
    text(state.can_pose, function()
      return trx.locale.get("general/photo_mode/change_lara_pose")
    end),
    text(nil, function()
      return trx.locale.get("general/photo_mode/advance_frame")
    end),
    text(nil, function()
      return trx.locale.get("general/photo_mode/toggle_help")
    end),
    text(nil, function()
      return trx.locale.get("general/photo_mode/snap_prompt")
    end),
    text(nil, function()
      return trx.locale.get("general/misc/exit")
    end),
  },
})

-------------------------------------------------------------------------------
-- the panel itself
-------------------------------------------------------------------------------

-- Keep the panel off the edge of the screen, so the frame only needs to carry
-- the margin between its edge and the text.
local panel = ui.widgets.Fit({
  shown = state.shown,
  child = ui.widgets.Frame({
    style = ui.FrameStyle.DIALOG,
    child = ui.widgets.Pad({
      x = 8,
      y = 6,
      child = ui.widgets.Stack({
        spacing = 8,
        align = ui.HAlign.SPAN,
        children = {
          title,
          ui.widgets.Stack({
            orientation = ui.Orientation.HORIZONTAL,
            spacing = 8,
            children = { keys, actions },
          }),
        },
      }),
    }),
  }),
})

ui.regions.place(ui.Region.TOP_LEFT, panel)
