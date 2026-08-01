---
title: Stats
order: 24
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/stats.lua. Edit it there.
-->

## Stats module

Module for what a level keeps count of: what Lara has found in it, and how much
there was to find.

The module is the level being played, so `trx.stats.pickups.count` is what she
has picked up in it. Any other level's counters are reached the same way through
`trx.game.Level.stats`. At the title screen there is no level, and everything
here reads `nil`.

### Structures

- [lua]`trx.stats.Category`

    One thing a level is counted on, which is one row of the statistics screen. `raw` is `max` plus `unobtainable`: the game flow can declare part of a level out of reach, and what it writes off is left out of what counts towards completion while still being in the level.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`count`**: integer. How many of them Lara has. The secrets cannot be set this way: they are held one by one, so `give_secret` and `take_secret` are how they change.
    - **`max`**: integer. How many of them count towards completing the level. *(read-only)*
    - **`raw`**: integer. How many of them the level holds, obtainable or not. *(read-only)*
    - **`unobtainable`**: integer. How many of them the game flow declares out of reach, and so must not be held against the player. *(read-only)*

- [lua]`trx.stats.Stats`

    What one level keeps count of. The counters are the level's own and can be written, which is what a script correcting or seeding them wants.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - **`ammo_hits`**: integer. How many of them hit something.
    - **`ammo_used`**: integer. How many rounds Lara has fired.
    - **`deaths`**: integer. How many times Lara has died. Unlike the rest, this is not cleared when the level is entered again: a death stays with the level it happened on.
    - **`distance_travelled`**: integer. How far Lara has travelled, in world units.
    - **`medipacks_used`**: number. How many medipacks Lara has used, a small one counting as half of one.
    - **`timer`**: integer. How long the level has been played, in game frames.

    Computed properties (derived, not stored on the object):
    - **`allies_hurt`**: boolean. Whether Lara has turned on an ally in this level.
    - **`crystals`**: Category. The save crystals, where the game has them.
    - **`kills`**: Category. The enemies the level counts, allies among them.
    - **`max_ally_kills`**: integer. How many of `kills.max` are allies. The statistics screen holds them against the player only once `allies_hurt`, so a screen written in Lua wants to do the same: `max_enemy_kills`, and these as well once she has turned on one.
    - **`max_enemy_kills`**: integer. How many of `kills.max` are enemies rather than allies.
    - **`pickups`**: Category. The items lying in the level for Lara to take.
    - **`secrets`**: Category. The level's secrets. Which ones Lara holds is `secret_list`.

    Methods:

    - [lua]`stats:give_secret(num)`  
      Marks a secret as found, as walking into its trigger would.

      Parameters:
      - **`num`** (integer). The secret's number, counted from one.

      Returns: boolean. `false` if the level has no such secret, or Lara already has it.

      Example:
      ```lua
      trx.stats.give_secret(1)
      ```

    - [lua]`stats:secret_list()`  
      The level's secrets, in order, as a list of `{ num, found }`. `num` is the number the player says, counted from one, and `found` is whether Lara has it.

      Returns: table. The secrets, one by one.

      Example:
      ```lua
      for _, secret in ipairs(trx.stats.secret_list()) do
        trx.log.info(secret.num .. ": " .. tostring(secret.found))
      end
      ```

    - [lua]`stats:take_secret(num)`  
      Takes a secret back, leaving it to be found again.

      Parameters:
      - **`num`** (integer). The secret's number, counted from one.

      Returns: boolean. `false` if the level has no such secret, or Lara does not have it.
