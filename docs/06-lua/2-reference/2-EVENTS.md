---
title: Events
---

## Events module

Lua scripts can listen for game events using the global `events` API.

### API

- [lua]`trx.events.on_level_start(callback)`  
- [lua]`trx.events.on_level_load(callback)`  
- [lua]`trx.events.on_pickup(callback)`  
- [lua]`trx.events.on_control(callback)`  
- [lua]`trx.events.on_control_post(callback)`  
  Register a handler for a game event. Returns `listener_id`.
- [lua]`trx.events.detach(listener_id)`  
  Remove a previously registered event handler.

### Events

#### `on_level_start`
Happens after the level finishes loading, prior to loading information from a
savegame.

Arguments:
- `level_num`

#### `on_level_load`
Happens after the level finishes loading, after loading information from a
savegame. If the game is started normally, this duplicates `on_level_start`.

Arguments:
- `level_num`

#### `on_pickup`
Happens just after Lara picks up an item.

Arguments:
- `item_num`

#### `on_control`
Happens on every logical game frame, before executing main game logic.

Arguments: none  

#### `on_control_post`
Happens on every logical game frame, after executing main game logic.

Arguments: none  

### Examples

```lua
trx.events.on_level_load(function(level_num)
  -- handle level load
end

trx.events.on_pickup(function(item_num)
  trx.console.log(trx.items[item_num].object_id)
end

local control_handler = trx.events.on_control(function()
  -- handle control loop event
end
-- detach a handler
trx.events.detach(control_handler)
```
