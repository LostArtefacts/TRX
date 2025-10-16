---
title: Events
---

## Events module

Lua scripts can listen for game events using the global `events` API and the
`trx.EventType` enum.

### Functions

- [lua]`trx.events.Listen(event_type, callback)`  
  Trigger the `callback` function on a specific item.

### Supported events

- [lua]`trx.EventType.LEVEL_LOAD`  
  Fired when a level finishes loading, but prior to loading a potential
  savegame. Listener receives the level number.
- [lua]`trx.EventType.LEVEL_START`  
  Fired when a level finishes loading, and after potential savegame data has
  completed loading. Listener receives the level number.
- [lua]`trx.EventType.CONTROL`  
  Fired before every game control loop iteration.
- [lua]`trx.EventType.CONTROL_POST`  
  Fired after every game control loop iteration.
- [lua]`trx.EventType.PICKUP`  
  Fired when the player picks up an item; listener receives the pickup item
  number.

### Example usage

Register listeners in Lua as follows:
```lua
trx.events.listen(trx.EventType.LEVEL_LOAD, function(level)
  -- handle level load
end)

trx.events.listen(trx.EventType.PICKUP, function(item)
  -- handle pickup event
end)
 
trx.events.listen(trx.EventType.CONTROL, function(action)
  -- handle control loop event
end)
```

Listeners declared in level scripts are automatically removed when the level
unloads.
