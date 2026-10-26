---
title: Locale
order: 24
---

<!--
  GENERATED FILE - do not edit.
  Regenerate with: tools/lint/gen/lua_docs
  The public API is declared next to its implementation, in
  src/lua/trx/locale.lua. Edit it there.
-->

## <a id="locale" name="locale"></a>Locale module

The text the player reads, in the player's own language.

### Properties

- <a id="locale.signals.changed" name="locale.signals.changed"></a>**`trx.locale.signals.changed`** ([trx.signal.Signal](SIGNAL.md#signal.Signal)). Says when the game text changes, such as when the player picks another language or a level brings text of its own. It holds a count that goes up with each change. *(read-only)*

### Structures

- <a id="locale.Text" name="locale.Text"></a>[lua]`trx.locale.Text`

    Text that looks itself up again in the player's language, for anything that
    shows text on screen. A label that is given one shows it in the current
    language, and changes when the player picks another one.

    Text resolves when it is turned into a string, and keeps the result until
    the game text changes, so showing it again costs nothing. Text made from the
    same key and arguments is equal.

    Operators:
    - **`text .. text`**. Joins text into a new text, whichever side of the `..` it is on.
    - **`text == text`**. Two texts are equal when they are made from the same parts.
    - **`#text`**. The length of the text in the player's language.
    - **`tostring(text)`**. The text in the player's language.

    Methods:

    - <a id="locale.Text.lower" name="locale.Text.lower"></a>[lua]`text:lower()`  
      The text in small letters, in the player's language, as
      [`trx.strings.lower`](STRINGS.md#strings.lower) turns it.

      Returns: [trx.locale.Text](#locale.Text). The text in small letters.

    - <a id="locale.Text.upper" name="locale.Text.upper"></a>[lua]`text:upper()`  
      The text in capitals, in the player's language, as [`trx.strings.upper`](STRINGS.md#strings.upper)
      turns it.

      Returns: [trx.locale.Text](#locale.Text). The text in capitals.

### Functions

- <a id="locale.signals" name="locale.signals"></a>[lua]`trx.locale.signals`  
  The signals the text speaks through, for a script that would rather hear
  about a change than ask after one.

- <a id="locale.declare" name="locale.declare"></a>[lua]`trx.locale.declare(strings)`  
  Declares game string keys and the text behind them.

  A key belongs with the script that shows it, so a command carries its own
  wording. What is declared here is a fallback, and it takes only for a key
  nothing else holds: the strings files, their translations, and any earlier
  declaration keep the text they already carry.

  Parameters:
  - <a id="locale.declare.strings" name="locale.declare.strings"></a>**`strings`** (table). Keys to their English text.

  Example:
  ```lua
  trx.locale.declare({
    ["console/cmd/heal/help"] = "Heals Lara back to full health.",
    ["console/cmd/heal/success"] = "Healed Lara back to full health",
  })
  ```

- <a id="locale.text_languages" name="locale.text_languages"></a>[lua]`trx.locale.text_languages()`  
  The languages that the game has strings files for, by the codes that the
  `language` setting takes.

  Returns: a list of string. The codes.

- <a id="locale.voice_languages" name="locale.voice_languages"></a>[lua]`trx.locale.voice_languages()`  
  The dubs installed beside the retail files, by the codes that the
  `audio.media_language` setting takes.

  Returns: a list of string. The codes.

- <a id="locale.language_name" name="locale.language_name"></a>[lua]`trx.locale.language_name(code)`  
  The name a language gives itself in its strings file.

  Parameters:
  - <a id="locale.language_name.code" name="locale.language_name.code"></a>**`code`** (string). The language code, such as `fr`.

  Returns: string or `nil`. The name, or `nil` where no strings file names it.

- <a id="locale.get" name="locale.get"></a>[lua]`trx.locale.get(key)`  
  The text behind a game string key.

  Parameters:
  - <a id="locale.get.key" name="locale.get.key"></a>**`key`** (string). The key, e.g. `general/misc/off`.

  Returns: string. The key itself if nothing is behind it, so a typo shows up on screen rather than as a nil further down.

  Example:
  ```lua
  trx.console.log(trx.locale.get("general/misc/off"))
  ```

- <a id="locale.format" name="locale.format"></a>[lua]`trx.locale.format(key, ...)`  
  The text behind a key with its placeholders filled in.

  Parameters:
  - <a id="locale.format.key" name="locale.format.key"></a>**`key`** (string). The key, e.g. `general/misc/off`.
  - <a id="locale.format...." name="locale.format...."></a>**`...`** (any). What to fill the placeholders with.

  Returns: string. The text with the arguments in it. A translation whose placeholders do not line up with the arguments comes back unformatted, with a warning in the log: a player is better served by text they can read than by a script that stops.

  Example:
  ```lua
  trx.console.log(trx.locale.format("general/misc/pagination_nav", 1, 5))
  ```

- <a id="locale.reload" name="locale.reload"></a>[lua]`trx.locale.reload()`  
  Reloads the current language's text from disk.

  Returns: boolean. Whether the reload succeeded.

- <a id="locale.text" name="locale.text"></a>[lua]`trx.locale.text(key, ...)`  
  The text behind a game string key, as text that follows the player's
  language. Use it for text that goes on screen, and [`trx.locale.get`](#locale.get) for
  text that a script reads.

  Parameters:
  - <a id="locale.text.key" name="locale.text.key"></a>**`key`** (string). The key, e.g. `general/misc/off`.
  - <a id="locale.text...." name="locale.text...."></a>**`...`** (any). What to fill the placeholders with. A [`trx.locale.Text`](#locale.Text) among them follows the player's language too.

  Returns: [trx.locale.Text](#locale.Text). The text.

  Example:
  ```lua
  local hint = trx.locale.text("general/misc/pagination_nav", 1, 5)
  ```

- <a id="locale.object_name" name="locale.object_name"></a>[lua]`trx.locale.object_name(object)`  
  The name the game shows for an object, as text that follows the player's
  language.

  Parameters:
  - <a id="locale.object_name.object" name="locale.object_name.object"></a>**`object`** ([trx.catalog.objects](CATALOG.md#catalog.objects)). The object.

  Returns: [trx.locale.Text](#locale.Text). The name, which is empty where the object has none.

- <a id="locale.level_title" name="locale.level_title"></a>[lua]`trx.locale.level_title(level)`  
  The name the game shows for a level, as text that follows the player's
  language.

  Parameters:
  - <a id="locale.level_title.level" name="locale.level_title.level"></a>**`level`** ([trx.game.Level](GAME.md#game.Level)). The level.

  Returns: [trx.locale.Text](#locale.Text). The name.
