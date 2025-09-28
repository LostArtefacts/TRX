---
title: Events
---

## Events module

Lua scripts can listen for game events using the global `Events` API and the
`TRX.EventType` enum.

### Functions

- [lua]`TRX.Events.Listen(event_type, callback)`  
  Trigger the `callback` function on a specific item.

### Supported events

- [lua]`TRX.EventType.LEVEL_LOAD`  
  Fired when a level finishes loading. Listener receives the level number.
- [lua]`TRX.EventType.PICKUP`  
  Fired when the player picks up an item; listener receives the pickup item
  number.

### Example usage

Register listeners in Lua as follows:
```lua
TRX.Events.Listen(TRX.EventType.LEVEL_LOAD, function(level)
  -- handle level load
end)

TRX.Events.Listen(TRX.EventType.PICKUP, function(item)
  -- handle pickup event
end)
```

Listeners declared in level scripts are automatically removed when the level
unloads.
