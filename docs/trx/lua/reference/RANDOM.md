---
title: Random
order: 31
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/random.lua. Edit it there.
-->

## <a id="random" name="random"></a>Random module

Random numbers, drawn from the sequence the simulation itself runs on.

The savegame carries that sequence, so what a script draws comes back the same
after a reload, and a script needs no seed of its own. The sequence moves on
for the engine as well: a script drawing every frame changes what the creatures
decide next.

Lua's own `math.random` is a separate generator that nothing saves. It has no
place in anything the simulation reads.

### Functions

- <a id="random.random" name="random.random"></a>[lua]`trx.random.random()`  
  A fraction of one, the whole number itself excepted.

  Returns: number. A value in [0, 1).

- <a id="random.randint" name="random.randint"></a>[lua]`trx.random.randint(a, b)`  
  A whole number between two bounds, both of them included.

  Parameters:
  - <a id="random.randint.a" name="random.randint.a"></a>**`a`** (integer). Lowest value.
  - <a id="random.randint.b" name="random.randint.b"></a>**`b`** (integer). Highest value. Below the lowest raises.

  Returns: integer. A value in [a, b].

  Example:
  ```lua
  local pips = trx.random.randint(1, 6)
  ```

- <a id="random.choice" name="random.choice"></a>[lua]`trx.random.choice(seq)`  
  One item out of a list, each as likely as the next.

  Parameters:
  - <a id="random.choice.seq" name="random.choice.seq"></a>**`seq`** (a list of any). What to choose from. An empty list raises.

  Returns: any. The item chosen.

  Example:
  ```lua
  local sample = trx.random.choice({
    trx.catalog.samples.LARA_NO,
    trx.catalog.samples.LARA_YES,
  })
  ```

- <a id="random.choices" name="random.choices"></a>[lua]`trx.random.choices(seq, [weights], [k])`  
  Several items out of a list, drawn one after another so that the same item can come up more than once. Weights give some items a greater share than others.

  Parameters:
  - <a id="random.choices.seq" name="random.choices.seq"></a>**`seq`** (a list of any). What to choose from. An empty list raises.
  - <a id="random.choices.weights" name="random.choices.weights"></a>**`weights`** (a list of number, optional). One share per item, none of them negative and not all zero. Defaults to an equal share each.
  - <a id="random.choices.k" name="random.choices.k"></a>**`k`** (integer, optional, default `1`). How many to draw. Below 0 raises.

  Returns: a list of any. The items chosen.

  Example:
  ```lua
  local drops = trx.random.choices({ "medipack", "ammo" }, { 1, 3 }, 5)
  ```

- <a id="random.angle" name="random.angle"></a>[lua]`trx.random.angle()`  
  A direction, anywhere around the turn.

  Returns: [trx.math.Angle](MATH.md#math.Angle). An angle within one turn.

  Example:
  ```lua
  trx.lara.item.rot = { x = 0, y = trx.random.angle(), z = 0 }
  ```

- <a id="random.chance" name="random.chance"></a>[lua]`trx.random.chance(p)`  
  Whether something with the given likelihood happens this time.

  Parameters:
  - <a id="random.chance.p" name="random.chance.p"></a>**`p`** (number). How likely, from 0 for never to 1 for always.

  Returns: boolean. Whether it happens.

  Example:
  ```lua
  if trx.random.chance(0.25) then
    trx.sound.play(trx.catalog.samples.LARA_NO)
  end
  ```
