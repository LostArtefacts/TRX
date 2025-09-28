---
title: Examples
---

## LUA script examples

### Adjusting enemy HP

This will adjust all bats starting HP to be 20 hitpoints.

```lua
local O_BAT = 9;

TRX.Events.Listen(TRX.EventType.LEVEL_LOAD, function(level)
  for i = 1, TRX.Items.Count() do
    local item = TRX.Items.Get(i)
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
TRX.Events.Listen(TRX.EventType.PICKUP, function(pickup_item)
  local lara = TRX.Lara.GetItem()
  lara.pos = {
    x = 73.5 * 1024,
    y = 3 * 1024,
    z = 3.5 * 1024,
  }
end)
```
