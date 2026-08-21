---
title: Stats
order: 28
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/stats.lua. Edit it there.
-->

## <a id="stats" name="stats"></a>Stats module

Module for what a level keeps count of: what Lara has found in it, and how much
there was to find.

The module is the level being played, so [`trx.stats.pickups.count`](#stats.Category.count) is what she
has picked up in it. Any other level's counters are reached the same way through
[`trx.game.Level.stats`](GAME.md#game.Level.stats). At the title screen there is no level, and everything
here reads `nil`.

### Structures

- <a id="stats.SecretNum" name="stats.SecretNum"></a>[lua]`trx.stats.SecretNum`

    The secret's number, as the player counts them. Counted from 1.

- <a id="stats.Category" name="stats.Category"></a>[lua]`trx.stats.Category`

    One thing a level is counted on, which is one row of the statistics screen. [`raw`](#stats.Category.raw) is [`max`](#stats.Category.max) plus [`unobtainable`](#stats.Category.unobtainable): the game flow can declare part of a level out of reach, and what it writes off is left out of what counts towards completion while still being in the level.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="stats.Category.count" name="stats.Category.count"></a>**`count`**: integer. How many of them Lara has. The secrets cannot be set this way: they are held one by one, so [`trx.stats.give_secret`](#stats.Stats.give_secret) and [`trx.stats.take_secret`](#stats.Stats.take_secret) are how they change.
    - <a id="stats.Category.max" name="stats.Category.max"></a>**`max`**: integer. How many of them count towards completing the level. *(read-only)*
    - <a id="stats.Category.raw" name="stats.Category.raw"></a>**`raw`**: integer. How many of them the level holds, obtainable or not. *(read-only)*
    - <a id="stats.Category.unobtainable" name="stats.Category.unobtainable"></a>**`unobtainable`**: integer. How many of them the game flow declares out of reach, and so must not be held against the player. *(read-only)*

- <a id="stats.Stats" name="stats.Stats"></a>[lua]`trx.stats.Stats`

    What one level keeps count of. The counters are the level's own and can be written, which is what a script correcting or seeding them wants.

    Handles are live references: if the underlying object is destroyed,
    using the handle raises an error rather than silently reading an
    unrelated one.

    Properties:
    - <a id="stats.Stats.ammo_hits" name="stats.Stats.ammo_hits"></a>**`ammo_hits`**: integer. How many of them hit something.
    - <a id="stats.Stats.ammo_used" name="stats.Stats.ammo_used"></a>**`ammo_used`**: integer. How many rounds Lara has fired.
    - <a id="stats.Stats.deaths" name="stats.Stats.deaths"></a>**`deaths`**: integer. How many times Lara has died. Unlike the rest, this is not cleared when the level is entered again: a death stays with the level it happened on.
    - <a id="stats.Stats.distance_travelled" name="stats.Stats.distance_travelled"></a>**`distance_travelled`**: [trx.math.Distance](MATH.md#math.Distance). How far Lara has travelled.
    - <a id="stats.Stats.medipacks_used" name="stats.Stats.medipacks_used"></a>**`medipacks_used`**: number. How many medipacks Lara has used, a small one counting as half of one.
    - <a id="stats.Stats.timer" name="stats.Stats.timer"></a>**`timer`**: [trx.game.Frames](GAME.md#game.Frames). How long the level has been played.

    Computed properties (derived, not stored on the object):
    - <a id="stats.Stats.allies_hurt" name="stats.Stats.allies_hurt"></a>**`allies_hurt`**: boolean. Whether Lara has turned on an ally in this level.
    - <a id="stats.Stats.crystals" name="stats.Stats.crystals"></a>**`crystals`**: [trx.stats.Category](#stats.Category). The save crystals, where the game has them.
    - <a id="stats.Stats.kills" name="stats.Stats.kills"></a>**`kills`**: [trx.stats.Category](#stats.Category). The enemies the level counts, allies among them.
    - <a id="stats.Stats.max_ally_kills" name="stats.Stats.max_ally_kills"></a>**`max_ally_kills`**: integer. How many of [`kills.max`](#stats.Category.max) are allies. The statistics screen holds them against the player only once [`allies_hurt`](#stats.Stats.allies_hurt), so a screen written in Lua wants to do the same: [`max_enemy_kills`](#stats.Stats.max_enemy_kills), and these as well once she has turned on one.
    - <a id="stats.Stats.max_enemy_kills" name="stats.Stats.max_enemy_kills"></a>**`max_enemy_kills`**: integer. How many of [`kills.max`](#stats.Category.max) are enemies rather than allies.
    - <a id="stats.Stats.pickups" name="stats.Stats.pickups"></a>**`pickups`**: [trx.stats.Category](#stats.Category). The items lying in the level for Lara to take.
    - <a id="stats.Stats.secrets" name="stats.Stats.secrets"></a>**`secrets`**: [trx.stats.Category](#stats.Category). The level's secrets. Which ones Lara holds is [`secret_list`](#stats.Stats.secret_list).

    Methods:

    - <a id="stats.Stats.give_secret" name="stats.Stats.give_secret"></a>[lua]`stats:give_secret(secret_num)`  
      Marks a secret as found, as walking into its trigger would.

      Parameters:
      - <a id="stats.Stats.give_secret.secret_num" name="stats.Stats.give_secret.secret_num"></a>**`secret_num`** ([trx.stats.SecretNum](#stats.SecretNum)).

      Returns: boolean. `false` if the level has no such secret, or Lara already has it.

      Example:
      ```lua
      trx.stats.give_secret(1)
      ```

    - <a id="stats.Stats.secret_list" name="stats.Stats.secret_list"></a>[lua]`stats:secret_list()`  
      The level's secrets, in order.

      Returns: a list of table. The secrets, one by one.

        Each entry:
        - <a id="stats.Stats.secret_list.num" name="stats.Stats.secret_list.num"></a>**`num`** ([trx.stats.SecretNum](#stats.SecretNum)). Which secret it is.
        - <a id="stats.Stats.secret_list.found" name="stats.Stats.secret_list.found"></a>**`found`** (boolean). Whether Lara has it.

      Example:
      ```lua
      for _, secret in ipairs(trx.stats.secret_list()) do
        trx.log.info(secret.num .. ": " .. tostring(secret.found))
      end
      ```

    - <a id="stats.Stats.take_secret" name="stats.Stats.take_secret"></a>[lua]`stats:take_secret(secret_num)`  
      Takes a secret back, leaving it to be found again.

      Parameters:
      - <a id="stats.Stats.take_secret.secret_num" name="stats.Stats.take_secret.secret_num"></a>**`secret_num`** ([trx.stats.SecretNum](#stats.SecretNum)).

      Returns: boolean. `false` if the level has no such secret, or Lara does not have it.
