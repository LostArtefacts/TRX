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

- [lua]`trx.lara.skin`  
    Reads/writes Lara's skin type. Skins are stored in saves, but writing this
    value does not change the global config setting, so subsequent levels will
    adhere to regular skin changes.

- [lua]`trx.lara.set_extra_equipment(lara_mesh_id, extra_mesh_id)`  
    Defines a specific extra mesh to be drawn at the same position as the given
    Lara mesh.

    The extra mesh must be present in the `O_LARA_SKIN_SWAP_EXTRA` object.
    Common useful IDs:
    - 6: The Dagger of Xian (hand)
    - 7: The Dagger of Xian (hips)
    - 8: Kayak oar
    - 9: Minecart spanner
    - 10: Drinks can (High Security Compound cutscene)

    Example:
    ```lua
    -- Put an oar in Lara's right hand
    trx.lara.set_extra_equipment(10, 8)
    ```

- [lua]`trx.lara.clear_equipment(lara_mesh_id)`  
    Removes any equipment on the given Lara mesh. This applies to guns and extra
    meshes.

- [lua]`trx.lara.holsters_visible`  
    Hides or shows Lara's holsters. This is used in OG TR1/2 gym and Home Sweet
    Home levels to maintain original outfit appearance. If Lara picks up a
    holster type weapon, or the weapon cheat is used, or a gun is given via the
    console, the holsters will automatically be made visible.

- [lua]`trx.lara.has_pistol_weapon`  
    Read-only - returns true if Lara has any pistol-type weapon currently in her
    inventory.

- [lua]`trx.lara.extra_anim`  
    Read-only - if Lara is currently in an extra anim state, returns the
    relative animation number of the `O_LARA_EXTRA` object.  
    Otherwise, returns -1.
