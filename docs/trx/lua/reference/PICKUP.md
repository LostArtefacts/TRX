---
title: Pickup
order: 15
---

## Pickup module

Module for controlling pickups.

### Enums

- [lua]`trx.pickup.Mode`
    Values: `NORMAL`, `PLINTH_LOW`, `PLINTH_HIGH`.

### Examples
- [lua]`trx.items[39].properties.pickup_mode = trx.pickup.Mode.PLINTH_LOW`  
    Lara will use her low pedestal animation for this pickup.
