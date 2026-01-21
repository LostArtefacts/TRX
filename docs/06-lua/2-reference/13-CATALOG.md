---
title: Catalog
---

## Catalog module

A convenience module for accessing TRX catalog IDs.

### Structures

- [lua]`trx.catalog.objects`   
  This table contains each TRX object ID. Names match those defined in `catalog_objects.csv`, with the `O_` prefix stripped and lowercased.   
  Examples:   
  - [lua]`if item.object_id == trx.catalog.objects.wolf then ...`

- [lua]`trx.catalog.flip_effects`   
  This table contains each TRX flip effect action ID. Names match those defined in `catalog_item_actions.csv`, with the `ITEM_ACTION_` prefix stripped and lowercased.   
  Examples:   
  - [lua]`trx.rooms.flip_effect(trx.catalog.flip_effects.floor_shake, 10)`
