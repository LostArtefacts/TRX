---
title: Examples
---

## LUA script examples

### Adjusting enemy HP

This will adjust all bats starting HP to be 20 hitpoints.

```lua
local O_BAT = 9;

trx.events.on_level_load(function(level)
  for i = 1, #trx.items do
    local item = trx.items[i]
    if item.object_id == O_BAT then
      item.hit_points = 20
      item.max_hit_points = 20
    end
  end
end)
```

### Teleporting Lara upon picking up a medipack

This will teleport Lara back to the starting point in TR1 Caves.

```lua
trx.events.on_pickup(function(pickup_item)
  local lara = trx.lara.get_item()
  lara.pos = {
    x = 73.5 * 1024,
    y = 3 * 1024,
    z = 3.5 * 1024,
  }
end)
```

### Running code every control loop

This will run the provided function once every logical frame, meaning the
function will always run at 30 FPS regardless of the player's FPS settings.

```lua
trx.events.on_control(function()
  -- handle per-frame control logic
end)
```

### Changing water color in concrete rooms

This will change the color to crimson red if Lara is in room 15, and
demonstrates how to throttle updates to only happen if Lara goes from one room
to another.

local last_room = 0

trx.events.on_control(function()
  local lara = trx.lara.get_item()
  if lara.room ~= last_room then
    last_room = lara.room
    if lara.room == 15 then
      trx.config.set("visuals.water_color", "ff0000")
    else
      trx.config.set("visuals.water_color", "0000ff")
    end
  end
end)
