---
title: Stats
order: 22
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/stats.lua. Edit it there.
-->

## Stats module

Module for what the level being played keeps count of: what Lara has found in
it, and how much there was to find.

The counts belong to the level, not to the session. Nothing here reads outside
one, so a script that runs at the title screen sees an empty list and zero
counts.

### Properties

- **`trx.stats.secrets`** (table). The secrets the level holds, in order, as a list of `{ num, found }`. `num` is
  the number the player says, counted from one, and `found` is whether Lara has
  it. *(read-only)*
- **`trx.stats.secret_count`** (integer). How many secrets Lara has found in this level. *(read-only)*
- **`trx.stats.max_secret_count`** (integer). How many secrets the level counts towards completion. Not the length of
  `secrets`: the game flow can declare some of a level's secrets unobtainable,
  and those are left out of this count while still standing in the list. *(read-only)*

### Functions

- [lua]`trx.stats.give_secret(num)`  
  Marks a secret as found, as walking into its trigger would.

  Parameters:
  - **`num`** (integer). The secret's number, counted from one.

  Returns: boolean. `false` if the level has no such secret, or Lara already has it.

  Example:
  ```lua
  trx.stats.give_secret(1)
  ```

- [lua]`trx.stats.take_secret(num)`  
  Takes a secret back, leaving it to be found again.

  Parameters:
  - **`num`** (integer). The secret's number, counted from one.

  Returns: boolean. `false` if the level has no such secret, or Lara does not have it.
