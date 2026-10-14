---
title: Examples
order: 1
---

## LUA script examples

### Adjusting enemy HP

This example sets all bats to start with random amount of hitpoints ranging
from 5 to 10, and all wolves to start with 30 hitpoints. Refer to [this
section](../../OBJECTS.md) as a reference for original values.

```lua
-- Adjust HP of certain enemies
trx.events.on_game_start(function()
  -- Randomize bats HP
  for _, item in pairs(trx.items) do
    if item.object_id == trx.catalog.objects.bat then
      item.properties.max_hit_points = math.random(5, 10)
    end
  end

  -- Boost all wolves HP
  trx.objects.wolf.properties.max_hit_points = 30
end)
```

### Teleporting Lara upon picking up a medipack

This will teleport Lara back to the starting point in TR1 Caves. Resetting the 
camera may be required in some cases.

```lua
trx.events.on_pickup(function(pickup_item)
  local lara = trx.lara.item
  lara.pos = {
    x = 73.5 * 1024,
    y = 3 * 1024,
    z = 3.5 * 1024,
  }
  trx.camera.reset()
end)
```

### Running code every control loop

This will run the provided function once every logical frame, meaning the
function will always run at 30 FPS regardless of the player's FPS settings.

```lua
trx.events.before_control(function()
  -- handle per-frame control logic
end)
```

### Changing water color in concrete rooms

This will change the color to crimson red if Lara is in room 15, and
demonstrates how to throttle updates to only happen if Lara goes from one room
to another.

```lua
local last_room = 0

trx.events.before_control(function()
  local lara = trx.lara.item
  if lara.room_num ~= last_room then
    last_room = lara.room_num
    if lara.room_num == 15 then
      trx.config.set("visuals.water_color", "ff0000")
    else
      trx.config.set("visuals.water_color", "0000ff")
    end
  end
end)
```

### User interface

Each of these files is a complete level script. The unit tests run each one as
it is, so the code works with the current API.

- [`ui/hud_widget.lua`](ui/hud_widget.lua) shows a widget in a corner of the
  screen for the whole level.
- [`ui/dialog.lua`](ui/dialog.lua) shows a note that the player closes.
- [`ui/menu.lua`](ui/menu.lua) asks the player to pick one entry from a list.
- [`ui/custom_widget.lua`](ui/custom_widget.lua) draws a widget with
  `trx.ui.primitive`.
- [`ui/screen.lua`](ui/screen.lua) draws the pause screen in place of the
  shipped one.
- [`ui/signals.lua`](ui/signals.lua) shows a value that changes, and hides it
  in photo mode.
- [`ui/tween.lua`](ui/tween.lua) moves a widget on the screen and then removes
  it.
