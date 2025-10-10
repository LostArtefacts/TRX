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
  Fired when a level finishes loading, but prior to loading a potential
  savegame. Listener receives the level number.
- [lua]`TRX.EventType.LEVEL_START`  
  Fired when a level finishes loading, and after potential savegame data has
  completed loading. Listener receives the level number.
- [lua]`TRX.EventType.CONTROL`  
  Fired before every game control loop iteration.
- [lua]`TRX.EventType.CONTROL_POST`  
  Fired after every game control loop iteration.
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
 
TRX.Events.Listen(TRX.EventType.CONTROL, function(action)
  -- handle control loop event
end)
```

Listeners declared in level scripts are automatically removed when the level
unloads.
