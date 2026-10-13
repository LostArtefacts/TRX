-- The photo mode help panel: what each key does while photo mode is open.
--
-- The panel is built once and updated through signals, like the rest of the
-- overlay. The engine draws the red border around the picture in the game view.

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

local M = {}

-- Builds the photo mode panel and places it. Called once, by the overlay.
function M.setup()
  local input = trx.input
  local ui = trx.ui
  local signal = trx.signal

  local state = {
    language = signal.config("language"),
    open = trx.game.signals.is_photo_mode,
  }

  state.shown = state.open & signal.config("ui.enable_photo_mode_ui")

  -- Read only while photo mode is open, which is the only time they show.
  state.target = signal.polled_while(state.open, function()
    return trx.game.photo_mode_target
  end, nil)

  state.can_pose = signal.polled_while(state.open, function()
    return trx.lara.can_pose
  end, false)

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
      ui.widgets.InputHint({
        input = { input.Role.STEP_LEFT, input.Role.STEP_RIGHT },
      }),
    },
  })

  -------------------------------------------------------------------------------
  -- the keys, and what each of them does
  -------------------------------------------------------------------------------

  local Role = input.Role
  local hint = ui.input_hint_text

  -- A key held with slow, or on its own.
  local function slow_or(role)
    return "[" .. hint(Role.SLOW) .. "+]" .. hint(role)
  end

  -- Name both exit keys when both are bound, or just the one that is available.
  local exit_key = signal.combine(state.language, function()
    local photo = input.has_glyph(input.Role.TOGGLE_PHOTO_MODE)
    local option = input.has_glyph(input.Role.OPTION)
    if photo and option then
      return hint(input.Role.TOGGLE_PHOTO_MODE)
        .. "/"
        .. hint(input.Role.OPTION)
    elseif photo then
      return hint(input.Role.TOGGLE_PHOTO_MODE)
    end
    return hint(input.Role.OPTION)
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
      ui.widgets.InputHint({
        input = {
          Role.CAMERA_UP,
          Role.CAMERA_DOWN,
          Role.CAMERA_FORWARD,
          Role.CAMERA_BACK,
          Role.CAMERA_LEFT,
          Role.CAMERA_RIGHT,
        },
      }),
      ui.widgets.InputHint({
        input = { Role.LEFT, Role.FORWARD, Role.BACK, Role.RIGHT },
      }),
      ui.widgets.Label({
        text = hint(Role.SLOW) .. "+" .. hint(Role.CAMERA_UP) .. "/" .. hint(
          Role.CAMERA_DOWN
        ),
      }),
      ui.widgets.InputHint({ input = Role.ROLL }),
      ui.widgets.InputHint({ input = Role.LOOK }),
      ui.widgets.Label({ text = slow_or(Role.DRAW) }),
      ui.widgets.Label({
        text = slow_or(Role.FLY_CHEAT),
        shown = state.can_pose,
      }),
      ui.widgets.Label({ text = slow_or(Role.PAUSE) }),
      ui.widgets.InputHint({ input = Role.TOGGLE_UI }),
      ui.widgets.InputHint({ input = Role.ACTION }),
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
end

return M
