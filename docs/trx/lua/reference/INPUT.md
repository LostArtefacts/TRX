---
title: Input
order: 41
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/input.lua. Edit it there.
-->

## <a id="input" name="input"></a>Input module

Module for reading input and working with player bindings.

Scripts ask about roles, not physical keys. A role is a game action such as
jumping, drawing a weapon, or opening a menu. The key or button that triggers it
depends on the player's device and layout.

Use `\{input ...}` in text to draw the binding for a role.

### Properties

- <a id="input.backend" name="input.backend"></a>**`trx.input.backend`** ([trx.input.Backend](#input.Backend)). The current input source. *(read-only)*
- <a id="input.layout" name="input.layout"></a>**`trx.input.layout`** ([trx.input.Layout](#input.Layout)). The current layout for the current input source. *(read-only)*
- <a id="input.is_listening" name="input.is_listening"></a>**`trx.input.is_listening`** (boolean). Whether script input capture is on. *(read-only)*

### Enums

- <a id="input.Role" name="input.Role"></a>[lua]`trx.input.Role` - 77 names

    A game action the player can bind to a key or button. In text, `\{input ...}` draws its current binding.


    <details><summary>Click here to see a list of all symbols.</summary>

    `ACTION`, `BACK`, `CAMERA_BACK`, `CAMERA_DOWN`, `CAMERA_FORWARD`,
    `CAMERA_LEFT`, `CAMERA_RESET`, `CAMERA_RIGHT`, `CAMERA_UP`, `CHANGE_OUTFIT`,
    `CHANGE_TARGET`, `CROUCH`, `CYCLE_LIGHTING_MODEL`, `DRAW`, `ENTER_CONSOLE`,
    `EQUIP_AUTOS`, `EQUIP_DESERT_EAGLE`, `EQUIP_GRENADE_LAUNCHER`,
    `EQUIP_HARPOON`, `EQUIP_M16`, `EQUIP_MAGNUMS`, `EQUIP_MP5`, `EQUIP_PISTOLS`,
    `EQUIP_ROCKET_LAUNCHER`, `EQUIP_SHOTGUN`, `EQUIP_UZIS`,
    `FAST_FORWARD_CHEAT`, `FLY_CHEAT`, `FORWARD`, `ITEM_CHEAT`, `JUMP`, `LEFT`,
    `LEVEL_SKIP_CHEAT`, `LOAD`, `LOOK`, `MENU_BACK`, `MENU_COARSE_ADJUST`,
    `MENU_CONFIRM`, `MENU_DOWN`, `MENU_FINE_ADJUST`, `MENU_LEFT`, `MENU_RIGHT`,
    `MENU_SHOW_INFO`, `MENU_SKIP`, `MENU_TAB_LEFT`, `MENU_TAB_RIGHT`, `MENU_UP`,
    `OPTION`, `PAUSE`, `QUICK_LOAD`, `QUICK_SAVE`, `RESET_BINDINGS`, `RIGHT`,
    `ROLL`, `SAVE`, `SCREENSHOT`, `SLOW`, `SLOW_MOTION_CHEAT`, `SPRINT`,
    `STEP_LEFT`, `STEP_RIGHT`, `SWITCH_BORDERS`, `SWITCH_UPSCALING`,
    `TOGGLE_BILINEAR_FILTER`, `TOGGLE_FPS_COUNTER`, `TOGGLE_FULLSCREEN`,
    `TOGGLE_PHOTO_MODE`, `TOGGLE_TEXTURES`, `TOGGLE_TRAPEZOID_FILTER`,
    `TOGGLE_UI`, `TOGGLE_WIREFRAME`, `TURBO_CHEAT`, `UNBIND_KEY`,
    `USE_BIG_MEDI`, `USE_BINOCULARS`, `USE_FLARE`, `USE_SMALL_MEDI`

    </details>

- <a id="input.Backend" name="input.Backend"></a>[lua]`trx.input.Backend`

    An input source, such as keyboard, controller, or touch.

    - `trx.input.Backend.KEYBOARD` = `0`  
        The keyboard, with the mouse.
    - `trx.input.Backend.CONTROLLER` = `1`  
        A game controller.
    - `trx.input.Backend.TOUCH` = `2`  
        The on-screen controls.

- <a id="input.Layout" name="input.Layout"></a>[lua]`trx.input.Layout`

    A saved set of bindings for one input source. The default layout is read-only;
    the custom layouts belong to the player.

    - `trx.input.Layout.DEFAULT` = `0`  
        The bindings the game ships with.
    - `trx.input.Layout.CUSTOM_1` = `1`  
        The player's first layout.
    - `trx.input.Layout.CUSTOM_2` = `2`  
        The player's second layout.
    - `trx.input.Layout.CUSTOM_3` = `3`  
        The player's third layout.

### Structures

- <a id="input.Slot" name="input.Slot"></a>[lua]`trx.input.Slot`

    Which of the two bindings a role can use. Counted from 1.

- <a id="input.Binding" name="input.Binding"></a>[lua]`trx.input.Binding`

    The slot, input source, and layout of a role binding.

    Properties:
    - <a id="input.Binding.backend" name="input.Binding.backend"></a>**`backend`**: [trx.input.Backend](#input.Backend), optional. Input source. Defaults to the current one.
    - <a id="input.Binding.layout" name="input.Binding.layout"></a>**`layout`**: [trx.input.Layout](#input.Layout), optional. Layout. Defaults to the current one.
    - <a id="input.Binding.slot" name="input.Binding.slot"></a>**`slot`**: [trx.input.Slot](#input.Slot), optional, default `1`. Binding slot. Defaults to the first.

- <a id="input.Capture" name="input.Capture"></a>[lua]`trx.input.Capture`

    A binding capture waiting for the player to press something.

    Methods:

    - <a id="input.Capture.cancel" name="input.Capture.cancel"></a>[lua]`capture:cancel()`  
      Stops the capture and leaves the binding as it was.

      Turning capture off is part of this, so a script that gives up does not have to
      do it itself.

      Returns: boolean. Whether the capture was still running.

### Functions

- <a id="input.signals" name="input.signals"></a>[lua]`trx.input.signals`  
  Input roles as signals.

  Each role has one shared signal, so several consumers of the same role use one
  read per tick.

- <a id="input.is_held" name="input.is_held"></a>[lua]`trx.input.is_held(role)`  
  Whether a role is active right now.

  This stays true while the player holds the bound key or button. Use it for
  actions that continue while held.

  Parameters:
  - <a id="input.is_held.role" name="input.is_held.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.

  Returns: boolean. Whether the role is active.

- <a id="input.is_pressed" name="input.is_pressed"></a>[lua]`trx.input.is_pressed(role)`  
  Whether a role became active this frame.

  This is true for one frame only. Use it for actions that happen once per press.

  Parameters:
  - <a id="input.is_pressed.role" name="input.is_pressed.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.

  Returns: boolean. Whether the role was pressed.

- <a id="input.is_anything_held" name="input.is_anything_held"></a>[lua]`trx.input.is_anything_held()`  
  Whether any role is active right now.

  Use this to wait for the player to let go before reading input for something
  else, such as a rebind.

  Returns: boolean. Whether anything is held.

- <a id="input.hold_off" name="input.hold_off"></a>[lua]`trx.input.hold_off(role)`  
  Keeps a handled role inactive until the player releases it.

  Use this after a script handles a press, so the same press does not reach other
  input code or fire again while held.

  Parameters:
  - <a id="input.hold_off.role" name="input.hold_off.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to take.

- <a id="input.is_backend_enabled" name="input.is_backend_enabled"></a>[lua]`trx.input.is_backend_enabled(backend)`  
  Whether an input source is enabled.

  Disabled sources are not read or shown in the controls dialog.

  Parameters:
  - <a id="input.is_backend_enabled.backend" name="input.is_backend_enabled.backend"></a>**`backend`** ([trx.input.Backend](#input.Backend)). The input source to ask about.

  Returns: boolean. Whether the input source is enabled.

- <a id="input.role_name" name="input.role_name"></a>[lua]`trx.input.role_name(role)`  
  The name the game shows for a role, in the player's language.

  Parameters:
  - <a id="input.role_name.role" name="input.role_name.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to name.

  Returns: string. The name of the role.

- <a id="input.layout_name" name="input.layout_name"></a>[lua]`trx.input.layout_name([layout])`  
  The name the game shows for a layout, in the player's language.

  Parameters:
  - <a id="input.layout_name.layout" name="input.layout_name.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to name. Defaults to the current one.

  Returns: string. The name of the layout.

- <a id="input.key_name" name="input.key_name"></a>[lua]`trx.input.key_name(role, [opts], [backend], [layout])`  
  Text for the key or button bound to a role.

  This is the text drawn by `\{input ...}`: a glyph when one exists, otherwise a
  key name. Empty bindings return nil.

  Use [`trx.input.Binding`](#input.Binding) to choose a slot, input source, or layout without
  placeholder nils. Positional arguments still work.

  Parameters:
  - <a id="input.key_name.role" name="input.key_name.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.
  - <a id="input.key_name.opts" name="input.key_name.opts"></a>**`opts`** ([trx.input.Slot](#input.Slot) or [trx.input.Binding](#input.Binding), optional). Binding to read. Defaults to the first slot on the current source and layout.
  - <a id="input.key_name.backend" name="input.key_name.backend"></a>**`backend`** ([trx.input.Backend](#input.Backend), optional). The input source to read. Defaults to the current one.
  - <a id="input.key_name.layout" name="input.key_name.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to read. Defaults to the current one.

  Returns: string or `nil`. The key text, or nil if the binding is empty.

- <a id="input.has_glyph" name="input.has_glyph"></a>[lua]`trx.input.has_glyph(role, [opts], [backend], [layout])`  
  Whether [`trx.input.key_name`](#input.key_name) has text to draw for a role binding.

  Use this to hide prompts for unbound roles.

  Use [`trx.input.Binding`](#input.Binding) to choose a slot, input source, or layout without
  placeholder nils. Positional arguments still work.

  Parameters:
  - <a id="input.has_glyph.role" name="input.has_glyph.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.
  - <a id="input.has_glyph.opts" name="input.has_glyph.opts"></a>**`opts`** ([trx.input.Slot](#input.Slot) or [trx.input.Binding](#input.Binding), optional). Binding to read. Defaults to the first slot on the current source and layout.
  - <a id="input.has_glyph.backend" name="input.has_glyph.backend"></a>**`backend`** ([trx.input.Backend](#input.Backend), optional). The input source to read. Defaults to the current one.
  - <a id="input.has_glyph.layout" name="input.has_glyph.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to read. Defaults to the current one.

  Returns: boolean. Whether the binding has text to draw.

- <a id="input.is_rebindable" name="input.is_rebindable"></a>[lua]`trx.input.is_rebindable(role)`  
  Whether the player can change a role's binding.

  Roles reserved by the game cannot be rebound and do not count as conflicts.

  Parameters:
  - <a id="input.is_rebindable.role" name="input.is_rebindable.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.

  Returns: boolean. Whether it can be bound.

- <a id="input.is_unbindable" name="input.is_unbindable"></a>[lua]`trx.input.is_unbindable(role)`  
  Whether the player can leave a role without a binding.

  Parameters:
  - <a id="input.is_unbindable.role" name="input.is_unbindable.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.

  Returns: boolean. Whether it can be left unbound.

- <a id="input.is_conflicted" name="input.is_conflicted"></a>[lua]`trx.input.is_conflicted(role, [opts], [layout])`  
  Whether another role uses the same binding in the same layout.

  Use [`trx.input.Binding`](#input.Binding) to choose an input source or layout without placeholder
  nils. Positional arguments still work.

  Parameters:
  - <a id="input.is_conflicted.role" name="input.is_conflicted.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to ask about.
  - <a id="input.is_conflicted.opts" name="input.is_conflicted.opts"></a>**`opts`** ([trx.input.Backend](#input.Backend) or [trx.input.Binding](#input.Binding), optional). Binding to check. Defaults to the current source and layout.
  - <a id="input.is_conflicted.layout" name="input.is_conflicted.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to read. Defaults to the current one.

  Returns: boolean. Whether the binding is used twice.

- <a id="input.listen" name="input.listen"></a>[lua]`trx.input.listen(enabled)`  
  Turns script input capture on or off.

  While capture is on, scripts can read or bind input without the game acting on
  the same input. Turn capture off as soon as the input is handled.

  Parameters:
  - <a id="input.listen.enabled" name="input.listen.enabled"></a>**`enabled`** (boolean). Whether script input capture is enabled.

- <a id="input.with_listen" name="input.with_listen"></a>[lua]`trx.input.with_listen(fn)`  
  Runs a function with script input capture on.

  Restores the previous capture state after the function returns or raises an
  error. Returns the function's results.

  Parameters:
  - <a id="input.with_listen.fn" name="input.with_listen.fn"></a>**`fn`** (function). Function to run while input is captured.

  Returns: any. What the function returned.

- <a id="input.bind_pressed" name="input.bind_pressed"></a>[lua]`trx.input.bind_pressed(role, [opts], [backend], [layout])`  
  Binds a role to the key or button the player is holding.

  Returns false if no input is held. Call it each frame while waiting for input,
  with [`trx.input.listen`](#input.listen) on or from inside [`trx.input.with_listen`](#input.with_listen). The default
  layout is read-only.

  Use [`trx.input.Binding`](#input.Binding) to choose a slot, input source, or layout without
  placeholder nils. Positional arguments still work.

  Parameters:
  - <a id="input.bind_pressed.role" name="input.bind_pressed.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to bind.
  - <a id="input.bind_pressed.opts" name="input.bind_pressed.opts"></a>**`opts`** ([trx.input.Slot](#input.Slot) or [trx.input.Binding](#input.Binding), optional). Binding to write. Defaults to the first slot on the current source and layout.
  - <a id="input.bind_pressed.backend" name="input.bind_pressed.backend"></a>**`backend`** ([trx.input.Backend](#input.Backend), optional). The input source to bind on. Defaults to the current one.
  - <a id="input.bind_pressed.layout" name="input.bind_pressed.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to write. Defaults to the current one.

  Returns: boolean. Whether a key was taken.

- <a id="input.capture" name="input.capture"></a>[lua]`trx.input.capture(role, [opts], [done])`  
  Binds a role to the next key or button the player presses.

  The capture spans frames: it waits for the player to let go of what is already
  down, turns capture on, and takes the first press after that. The previous
  capture state is restored when it lands or when the capture is cancelled.

  Use this instead of [`trx.input.listen`](#input.listen) and [`trx.input.bind_pressed`](#input.bind_pressed), which only
  answer for the frame they run on. The default layout is read-only.

  Use [`trx.input.Binding`](#input.Binding) to choose a slot, input source, or layout without
  placeholder nils.

  Parameters:
  - <a id="input.capture.role" name="input.capture.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to bind.
  - <a id="input.capture.opts" name="input.capture.opts"></a>**`opts`** ([trx.input.Slot](#input.Slot) or [trx.input.Binding](#input.Binding), optional). Binding to write. Defaults to the first slot on the current source and layout.
  - <a id="input.capture.done" name="input.capture.done"></a>**`done`** (function, optional). Called when the capture ends.
    Called with:
    - <a id="input.capture.bound" name="input.capture.bound"></a>**`bound`** (boolean). Whether a key was taken.

  Returns: [trx.input.Capture](#input.Capture). The running capture.

- <a id="input.unbind" name="input.unbind"></a>[lua]`trx.input.unbind(role, [opts], [backend], [layout])`  
  Clears one role binding.

  The default layout is read-only, and roles reserved by the game cannot be left
  unbound.

  Use [`trx.input.Binding`](#input.Binding) to choose a slot, input source, or layout without
  placeholder nils. Positional arguments still work.

  Parameters:
  - <a id="input.unbind.role" name="input.unbind.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to unbind.
  - <a id="input.unbind.opts" name="input.unbind.opts"></a>**`opts`** ([trx.input.Slot](#input.Slot) or [trx.input.Binding](#input.Binding), optional). Binding to clear. Defaults to the first slot on the current source and layout.
  - <a id="input.unbind.backend" name="input.unbind.backend"></a>**`backend`** ([trx.input.Backend](#input.Backend), optional). The input source to write. Defaults to the current one.
  - <a id="input.unbind.layout" name="input.unbind.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to write. Defaults to the current one.

- <a id="input.reset_layout" name="input.reset_layout"></a>[lua]`trx.input.reset_layout([opts], [layout])`  
  Restores a custom layout to the default bindings.

  Use [`trx.input.Binding`](#input.Binding) to choose an input source or layout without placeholder
  nils. Positional arguments still work.

  Parameters:
  - <a id="input.reset_layout.opts" name="input.reset_layout.opts"></a>**`opts`** ([trx.input.Backend](#input.Backend) or [trx.input.Binding](#input.Binding), optional). Layout to reset. Defaults to the current source and layout.
  - <a id="input.reset_layout.layout" name="input.reset_layout.layout"></a>**`layout`** ([trx.input.Layout](#input.Layout), optional). The layout to write. Defaults to the current one.

- <a id="input.signals.held" name="input.signals.held"></a>[lua]`trx.input.signals.held(role)`  
  A signal for whether a role is active.

  It is true while the player holds the bound key or button. It changes when the
  role becomes active and when it stops.

  Parameters:
  - <a id="input.signals.held.role" name="input.signals.held.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to follow.

  Returns: [trx.signal.Signal](SIGNAL.md#signal.Signal). The role's signal.

- <a id="input.signals.pressed" name="input.signals.pressed"></a>[lua]`trx.input.signals.pressed(role)`  
  A signal for when a role becomes active.

  It is true for one tick only, so listeners run once per press.

  Parameters:
  - <a id="input.signals.pressed.role" name="input.signals.pressed.role"></a>**`role`** ([trx.input.Role](#input.Role)). The role to follow.

  Returns: [trx.signal.Signal](SIGNAL.md#signal.Signal). The role's signal.
