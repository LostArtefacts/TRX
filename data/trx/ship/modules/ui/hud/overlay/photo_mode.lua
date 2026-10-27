-- The photo mode help panel, which says what each key does while photo mode
-- is open, and the red frame around the screen, which says photo mode is on.
--
-- The panel is built once and updated through signals, like the rest of the
-- overlay. The frame is painted only while photo mode is open.

local accent = require("common.ui.accent")

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

-- Builds the photo mode panel and the frame. Called once, by the overlay.
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

  local function text(shown, words)
    return ui.widgets.Label({
      text = words,
      visible = shown,
    })
  end

  -- A prompt follows the current target, so the same row can say "move the
  -- camera" or "move Lara".
  local function per_target(camera, lara)
    return state.target:map(function(target)
      if target == trx.game.PhotoModeTarget.CAMERA then
        return camera
      end
      return lara
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
        text = accent.signal(
          per_target(
            trx.locale.text("general/photo_mode/title_camera_pos"),
            trx.locale.text("general/photo_mode/title_lara_pos")
          )
        ),
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
        visible = state.can_pose,
      }),
      ui.widgets.Label({ text = slow_or(Role.PAUSE) }),
      ui.widgets.InputHint({ input = Role.TOGGLE_UI }),
      ui.widgets.InputHint({ input = Role.ACTION }),
      ui.widgets.Label({ text = exit_key, visible = exit_shown }),
    },
  })

  local actions = ui.widgets.Stack({
    children = {
      ui.widgets.Label({
        text = per_target(
          trx.locale.text("general/photo_mode/camera_move_prompt"),
          trx.locale.text("general/photo_mode/lara_move_prompt")
        ),
      }),
      ui.widgets.Label({
        text = per_target(
          trx.locale.text("general/photo_mode/camera_rotate_prompt"),
          trx.locale.text("general/photo_mode/lara_rotate_prompt")
        ),
      }),
      ui.widgets.Label({
        text = per_target(
          trx.locale.text("general/photo_mode/camera_roll_prompt"),
          trx.locale.text("general/photo_mode/lara_roll_prompt")
        ),
      }),
      ui.widgets.Label({
        text = per_target(
          trx.locale.text("general/photo_mode/camera_rotate_90_prompt"),
          trx.locale.text("general/photo_mode/lara_rotate_90_prompt")
        ),
      }),
      ui.widgets.Label({
        text = per_target(
          trx.locale.text("general/photo_mode/camera_reset_prompt"),
          trx.locale.text("general/photo_mode/lara_reset_prompt")
        ),
      }),
      text(nil, trx.locale.text("general/photo_mode/fov_prompt")),
      text(
        state.can_pose,
        trx.locale.text("general/photo_mode/change_lara_pose")
      ),
      text(nil, trx.locale.text("general/photo_mode/advance_frame")),
      text(nil, trx.locale.text("general/photo_mode/toggle_help")),
      text(nil, trx.locale.text("general/photo_mode/snap_prompt")),
      text(nil, trx.locale.text("general/misc/exit")),
    },
  })

  -------------------------------------------------------------------------------
  -- the panel itself
  -------------------------------------------------------------------------------

  -- Keep the panel off the edge of the screen, so the frame only needs to carry
  -- the margin between its edge and the text.
  local panel = ui.widgets.Fit({
    visible = state.shown,
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

  -------------------------------------------------------------------------------
  -- the red frame around the screen
  -------------------------------------------------------------------------------

  do
    local RED = { r = 255, g = 0, b = 0, a = 96 }
    local primitive = ui.primitive

    -- Paints the frame four units thick at the text size, rounded to whole
    -- screen pixels.
    local function paint()
      local thickness =
        math.floor(primitive.to_screen(4 * ui.text_scale) + 0.5)
      if thickness <= 0 then
        return
      end
      local t = primitive.to_canvas(thickness)
      local w = ui.screen.width
      local h = ui.screen.height
      primitive.quad(0, 0, 0, w, t, RED)
      primitive.quad(0, h - t, 0, w, t, RED)
      primitive.quad(0, t, 0, t, h - 2 * t, RED)
      primitive.quad(w - t, t, 0, t, h - 2 * t, RED)
    end

    ---@type trx.events.Listener?
    local painting = nil
    local function follow(open)
      if open and painting == nil then
        painting = ui.on_paint(paint)
      elseif not open and painting ~= nil then
        painting:detach()
        painting = nil
      end
    end
    follow(state.open:get())
    state.open:on(follow)
  end
end

return M
