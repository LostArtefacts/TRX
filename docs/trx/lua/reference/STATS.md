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

The module is the level being played, so [`trx.stats.pickups.count`](#stats.Category.count) is what she
has picked up in it. Any other level's counters are reached the same way through
[`trx.game.Level.stats`](GAME.md#game.Level.stats). At the title screen there is no level, and everything
here reads `nil`.

### Structures

- <a name="stats.SecretNum"></a>[lua]`trx.stats.SecretNum`

    The secret's number, as the player counts them. Counted from 1.

- <a name="stats.Category"></a>[lua]`trx.stats.Category`

    One thing a level is counted on, which is one row of the statistics screen. `raw` is `max` plus `unobtainable`: the game flow can declare part of a level out of reach, and what it writes off is left out of what counts towards completion while still being in the level.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a name="stats.Category.count"></a>**`count`**: integer. How many of them Lara has. The secrets cannot be set this way: they are held one by one, so `give_secret` and `take_secret` are how they change.
    - <a name="stats.Category.max"></a>**`max`**: integer. How many of them count towards completing the level. *(read-only)*
    - <a name="stats.Category.raw"></a>**`raw`**: integer. How many of them the level holds, obtainable or not. *(read-only)*
    - <a name="stats.Category.unobtainable"></a>**`unobtainable`**: integer. How many of them the game flow declares out of reach, and so must not be held against the player. *(read-only)*

- <a name="stats.Stats"></a>[lua]`trx.stats.Stats`

    What one level keeps count of. The counters are the level's own and can be written, which is what a script correcting or seeding them wants.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a name="stats.Stats.ammo_hits"></a>**`ammo_hits`**: integer. How many of them hit something.
    - <a name="stats.Stats.ammo_used"></a>**`ammo_used`**: integer. How many rounds Lara has fired.
    - <a name="stats.Stats.deaths"></a>**`deaths`**: integer. How many times Lara has died. Unlike the rest, this is not cleared when the level is entered again: a death stays with the level it happened on.
    - <a name="stats.Stats.distance_travelled"></a>**`distance_travelled`**: integer. How far Lara has travelled, in world units.
    - <a name="stats.Stats.medipacks_used"></a>**`medipacks_used`**: number. How many medipacks Lara has used, a small one counting as half of one.
    - <a name="stats.Stats.timer"></a>**`timer`**: integer. How long the level has been played, in game frames.

    Computed properties (derived, not stored on the object):
    - <a name="stats.Stats.allies_hurt"></a>**`allies_hurt`**: boolean. Whether Lara has turned on an ally in this level.
    - <a name="stats.Stats.crystals"></a>**`crystals`**: [trx.stats.Category](#stats.Category). The save crystals, where the game has them.
    - <a name="stats.Stats.kills"></a>**`kills`**: [trx.stats.Category](#stats.Category). The enemies the level counts, allies among them.
    - <a name="stats.Stats.max_ally_kills"></a>**`max_ally_kills`**: integer. How many of `kills.max` are allies. The statistics screen holds them against the player only once `allies_hurt`, so a screen written in Lua wants to do the same: `max_enemy_kills`, and these as well once she has turned on one.
    - <a name="stats.Stats.max_enemy_kills"></a>**`max_enemy_kills`**: integer. How many of `kills.max` are enemies rather than allies.
    - <a name="stats.Stats.pickups"></a>**`pickups`**: [trx.stats.Category](#stats.Category). The items lying in the level for Lara to take.
    - <a name="stats.Stats.secrets"></a>**`secrets`**: [trx.stats.Category](#stats.Category). The level's secrets. Which ones Lara holds is `secret_list`.

    Methods:

    - <a name="stats.Stats.give_secret"></a>[lua]`stats:give_secret(secret_num)`  
      Marks a secret as found, as walking into its trigger would.

      Parameters:
      - **`secret_num`** ([trx.stats.SecretNum](#stats.SecretNum)).

      Returns: boolean. `false` if the level has no such secret, or Lara already has it.

      Example:
      ```lua
      trx.stats.give_secret(1)
      ```

    - <a name="stats.Stats.secret_list"></a>[lua]`stats:secret_list()`  
      The level's secrets, in order, as a list of `{ num, found }`. `num` is the number the player says, and `found` is whether Lara has it.

      Returns: table. The secrets, one by one.

      Example:
      ```lua
      for _, secret in ipairs(trx.stats.secret_list()) do
        trx.log.info(secret.num .. ": " .. tostring(secret.found))
      end
      ```

    - <a name="stats.Stats.take_secret"></a>[lua]`stats:take_secret(secret_num)`  
      Takes a secret back, leaving it to be found again.

      Parameters:
      - **`secret_num`** ([trx.stats.SecretNum](#stats.SecretNum)).

      Returns: boolean. `false` if the level has no such secret, or Lara does not have it.
