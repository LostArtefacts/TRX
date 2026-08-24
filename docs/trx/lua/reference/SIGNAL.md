---
title: Signals
order: 38
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: just lua-api-dump
  The public API is declared next to its implementation, in
  src/lua/api/signal.lua. Edit it there.
-->

## <a id="signal" name="signal"></a>Signals module

A value that can notify listeners when it changes.

A signal lets a script react to changes without polling every frame. You can
read the current value, listen for changes, and combine signals with `&`, `|`
and `~`.

Setting a signal to its current value does nothing: listeners do not run, and
signals derived from it do not update. This keeps combined signals cheap to
listen to. A combined signal fires only when its own result changes, not every
time one of its inputs changes.

A derived signal is read like any other signal, so one expression can provide
both the current result and change notifications.

Signals should carry numbers, strings or booleans, not handles. Handles are
created fresh on each read, so two reads of the same handle do not compare
equal and would make the signal report a change every frame. When a signal
represents an engine-owned object, it carries the object's numeric id; listeners
can then read the handle when they need it.

Signals stay idle until something uses them: a polled signal starts reading only
when it is created.

### Properties

- <a id="signal.tick" name="signal.tick"></a>**`trx.signal.tick`** ([trx.signal.Signal](#signal.Signal)). A signal that increments on every engine tick, regardless of what is on screen.
  Anything listening to it runs every tick.

  Use this when a script needs to poll state that has no dedicated signal, such as
  Lara's current position. A dedicated signal is cheaper when one exists, because
  this one wakes listeners even when the state they care about has not changed. *(read-only)*

### Structures

- <a id="signal.Signal" name="signal.Signal"></a>[lua]`trx.signal.Signal`

    A value that notifies listeners when it changes.

    Operators:
    - **`signal & signal`**. Both signals are true. Fires when the result changes.
    - **`~signal`**. The signal is not true: `~trx.cutscenes.signals.is_playing`.
    - **`signal | signal`**. Either signal is true. Fires when the result changes.

    Methods:

    - <a id="signal.Signal.above" name="signal.Signal.above"></a>[lua]`signal:above(amount)`  
      Whether the signal holds more than this number.

      Parameters:
      - <a id="signal.Signal.above.amount" name="signal.Signal.above.amount"></a>**`amount`** (number). What to compare against.

      Returns: [trx.signal.Signal](#signal.Signal). The derived signal.

    - <a id="signal.Signal.eq" name="signal.Signal.eq"></a>[lua]`signal:eq(value)`  
      Whether the signal holds this value.

      Parameters:
      - <a id="signal.Signal.eq.value" name="signal.Signal.eq.value"></a>**`value`** (any). What to compare against.

      Returns: [trx.signal.Signal](#signal.Signal). The derived signal.

    - <a id="signal.Signal.get" name="signal.Signal.get"></a>[lua]`signal:get()`  
      The value the signal holds now.

      Returns: any. What it holds.

    - <a id="signal.Signal.map" name="signal.Signal.map"></a>[lua]`signal:map(fn)`  
      Creates a signal by applying a function to this signal's value.

      Use this for derived values that are not simple boolean combinations, such as a
      bar fill amount or resolved key text.

      Parameters:
      - <a id="signal.Signal.map.fn" name="signal.Signal.map.fn"></a>**`fn`** (function). The function that computes the derived value.
        Called with:
        - <a id="signal.Signal.map.value" name="signal.Signal.map.value"></a>**`value`** (any). This signal's value.

      Returns: [trx.signal.Signal](#signal.Signal). The derived signal.

    - <a id="signal.Signal.on" name="signal.Signal.on"></a>[lua]`signal:on(fn)`  
      Calls the handler with the new value whenever the signal changes. Attaching a listener does not call it immediately; read the signal directly when you need its current value.

      Parameters:
      - <a id="signal.Signal.on.fn" name="signal.Signal.on.fn"></a>**`fn`** (function). The function to call when the signal changes.
        Called with:
        - <a id="signal.Signal.on.value" name="signal.Signal.on.value"></a>**`value`** (any). The new value.

      Returns: [trx.signal.Listener](#signal.Listener). The listener handle used to detach later.

    - <a id="signal.Signal.set" name="signal.Signal.set"></a>[lua]`signal:set(value)`  
      Sets the signal's value. Setting the current value again does nothing, so repeated writes are cheap.

      Parameters:
      - <a id="signal.Signal.set.value" name="signal.Signal.set.value"></a>**`value`** (any). The new value.

      Returns: boolean. Whether the value changed and listeners ran.

    - <a id="signal.Signal.stop" name="signal.Signal.stop"></a>[lua]`signal:stop()`  
      Stops a derived signal from following its sources. It keeps its last value and will not update again. Signals made by level scripts stop when the level ends; global scripts can call this to stop one earlier.

      Returns: boolean. Whether the signal was still following any sources.

- <a id="signal.Listener" name="signal.Listener"></a>[lua]`trx.signal.Listener`

    A signal listener that can be detached later.

    Methods:

    - <a id="signal.Listener.detach" name="signal.Listener.detach"></a>[lua]`listener:detach()`  
      Detaches the listener so it no longer receives changes.

      Returns: boolean. Whether it was still listening.

### Functions

- <a id="signal.polled" name="signal.polled"></a>[lua]`trx.signal.polled(read)`  
  Creates a signal by reading a value once per tick and notifying listeners only
  when that value changes.

  Use this for state that has no dedicated engine signal. The read function runs
  every tick, but listeners run only on changes, so several listeners on one
  polled signal share one read.

  Parameters:
  - <a id="signal.polled.read" name="signal.polled.read"></a>**`read`** (function). The function to read each tick. Tables compare by identity, so returning a fresh table every tick reports a change every tick.

  Returns: [trx.signal.Signal](#signal.Signal). The polled signal.

- <a id="signal.config" name="signal.config"></a>[lua]`trx.signal.config(key)`  
  Returns a signal for a config setting.

  The signal holds the setting's current value and updates whenever the player or
  a script changes it. Asking for the same setting twice returns the same signal.

  Parameters:
  - <a id="signal.config.key" name="signal.config.key"></a>**`key`** (string). Dotted setting path, as accepted by [`trx.config.get`](CONFIG.md#config.get).

  Returns: [trx.signal.Signal](#signal.Signal). The setting's signal.

- <a id="signal.combine" name="signal.combine"></a>[lua]`trx.signal.combine(..., fn)`  
  Creates a signal by applying a function to several source signals.

  Pass the signals first and the function last. [`trx.signal.Signal:map`](#signal.Signal.map) is the
  one-signal version. For boolean combinations, `&`, `|` and `~` are shorter.

  Parameters:
  - <a id="signal.combine...." name="signal.combine...."></a>**`...`** ([trx.signal.Signal](#signal.Signal)). The signals to read, in the order the function takes them.
  - <a id="signal.combine.fn" name="signal.combine.fn"></a>**`fn`** (function). The function that computes the derived value.

  Returns: [trx.signal.Signal](#signal.Signal). The derived signal.

  Example:
  ```lua
  local fill = trx.signal.combine(
    trx.lara.signals.hp,
    trx.lara.signals.max_hp,
    function(hp, max_hp)
      return hp / max_hp
    end
  )
  ```

- <a id="signal.new" name="signal.new"></a>[lua]`trx.signal.new(value)`  
  Creates a script-owned signal with an initial value.

  Parameters:
  - <a id="signal.new.value" name="signal.new.value"></a>**`value`** (any). The initial value.

  Returns: [trx.signal.Signal](#signal.Signal). The new signal.
