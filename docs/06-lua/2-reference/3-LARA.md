---
title: Lara
---

## Lara module

Module for interacting with the Lara's object.

### Functions

- [lua]`trx.lara.item`  
    Returns [lua]`trx.items.Item` associated with Lara, or [lua]`nil` if the
    Lara object is not available.

- [lua]`trx.lara.exposure_bar`  
    Reads/writes exposure timer (cold water bar). The maximum value is 600.
    If the cold bar setting is disabled on the game flow level, the health must
    be managed manually from LUA.

- [lua]`trx.lara.air_bar`  
    Reads/writes Lara's air timer. The maximum value is 1800.  
      
    Example:
    ```lua
    -- Infinite oxygen
    trx.events.on_control_post(function()
        trx.lara.air_bar = 1800
    end)
    ```
