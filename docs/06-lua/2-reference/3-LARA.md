---
title: Lara
---

## Lara module

Module for interacting with the Lara's object.

### Functions

- [lua]`trx.lara.get_item()`  
    Returns [lua]`trx.items.Item` associated with Lara, or [lua]`nil` if the
    Lara object is not available.

- [lua]`trx.lara.exposure_timer`  
    Reads/writes exposure timer (cold water bar). The maximum value is 600.
    If the cold bar setting is disabled on the game flow level, the health must
    be managed manually from LUA.
