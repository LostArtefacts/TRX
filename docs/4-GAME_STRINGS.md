---
title: Game strings
---

# Game Strings

## Overview

This document describes how to use and translate the TRX game strings. Game
strings let level builders customize built-in object and level names in custom
levels, and translators translate the entire game (including UI) into other
languages. The configuration is structured in JSON5 format, which permits
comments and supports a layering mechanism for overrides.

## Audiences

This document serves two main audiences:

- **Level builders**: Customize object names, level titles, and UI strings for
  custom levels and mod packs.
- **Translators**: Translate the full game text (gameplay, UI, and menus) into
  different languages.

## Quick‑start example

```json5
{
    // Override only the key_1 pickup in Level 0
    "levels": [
        {
            "title": "City of Vilcabamba",
            "objects": { "key_1": { "name": "Gold Key" } }
        }
    ]
}
```

## Layering and Override Mechanism

TRX supports multiple layers of strings files that are loaded in a specific
order. Later layers override earlier ones, allowing expansion packs and custom
mods to selectively override base text. The default load order is:

1. `base_strings.json5` — Common defaults for all engines and languages.
2. `tr1/strings.json5` — Base strings for the main game.
3. `tr1-ub/strings.json5` — Overrides for the Unfinished Business expansion pack
   (or other expansion packs/mods).

   Depending on which mod or pack you run, the third layer may vary:
   - `tr1-ub/strings.json5` for the Unfinished Business expansion pack
   - `tr2-gm/strings.json5` for the Golden Mask TR2 expansion pack
   - `tr1-demo-pc/strings.json5` for the PC TR1 demo
   - `tr*-level/strings.json5` for the -l/--level command line switch

For example, if the same key exists in both `base_strings.json5` and
`tr1/strings.json5`, the value `tr1/strings.json5` will take precedence.

Each layer can also have a variant translated to other languages - see
[this section](./4-GAME_STRINGS.md#translating-each-layer).

## General structure

The document is organized as follows:

```json5
{
    "levels": [
        {
            "title": "City of Vilcabamba",
            "objects": {
                "key_1": {
                    "name": "Silver Key",
                    "description": "This shows when the player examines key1 in the inventory.",
                },
                "puzzle_1": {
                    "name": "Gold Idol",
                    "description": "You can use \n to make new lines and \f to make new pages.",
                },
                "key_2": {
                    "name": "Rusty Key",
                },
                // etc
            },
            "game_strings": {
                "STATS_TIME_TAKEN": "Time Taken",
                // etc
            },
        },
        // etc
    ],
    "objects": {
        "lara": {"name": "Lara"},
        "dog": {"names": ["Dog", "Doberman"]},
        // etc
    },
    "game_strings": {
        "STATS_TIME_TAKEN": "Time Taken",
        // etc
    },
}
```

<table>
  <tr valign="top" align="left">
    <th>Property</th>
    <th>Type</th>
    <th>Required</th>
    <th>Description</th>
  </tr>

  <tr valign="top">
    <td>
      <code>levels</code>
    </td>
    <td>Object&nbsp;array</td>
    <td>No</td>
    <td>
      This is where overrides for individual level details are defined. If a
      level doesn't override a string through its <code>objects</code> or
      <code>game_strings</code>, it'll be looked up in the global scope next.
      If the global scope doesn't define it either, it'll default to an
      internal default value shipped with the engine.
    </td>
  </tr>

  <tr valign="top">
    <td>
      <code>title</code>
    </td>
    <td>String</td>
    <td>Yes</td>
    <td colspan="2">The title of the level.</td>
  </tr>

  <tr valign="top">
    <td>
      <code>objects</code>
    </td>
    <td>Object&nbsp;array</td>
    <td>No</td>
    <td>
      Object-related strings.
    </td>
  </tr>

  <tr valign="top">
    <td>
      <code>game_strings</code>
    </td>
    <td>String-to-string map</td>
    <td>No</td>
    <td>
      General game/UI strings.
    </td>
  </tr>

  <tr valign="top">
    <td>
      <code>name</code>
    </td>
    <td>String</td>
    <td>No</td>
    <td colspan="2">
      Allows to rename any object, including key items and pickups.
    </td>
  </tr>

  <tr valign="top">
    <td>
      <code>names</code>
    </td>
    <td>String</td>
    <td>No</td>
    <td colspan="2">
      Allows to give more than a single name to any object. Objects that show
      up in the inventory ring will use the first name. Other than that, the
      additional names can be used with various console commands such as
      <code>/tp</code> and <code>/give</code>.
    </td>
  </tr>

  <tr valign="top">
    <td>
      <code>description</code>
    </td>
    <td>String</td>
    <td>No</td>
    <td colspan="2">
      Allows longer text descriptions to be defined for key and puzzle items.
      Players can examine items in the inventory when this text has been
      defined. Use <code>\n</code> in the text to create new lines; you can also
      use <code>\f</code> to force a page break. Long text will be automatically
      wrapped and paginated as necessary. If an empty string is defined, the UI
      will not be shown and the inventory item simply focused instead.
    </td>
  </tr>
  <tr valign="top">
    <td><code>language_name</code></td>
    <td>String</td>
    <td>No (only in common file)</td>
    <td colspan="2">
      The display name of the language (e.g., "English", "Français") shown in
      the language selection UI. Should only be defined in the
      <code>base_strings.json5</code> file.
    </td>
  </tr>
  <tr valign="top">
    <td><code>extends</code></td>
    <td>String</td>
    <td>No</td>
    <td colspan="2">
      Fallback to another language code for missing entries. For dialects (e.g., "fr-ca"),
      specify <code>"extends": "fr"</code> to inherit missing layers from the parent language.
    </td>
  </tr>
</table>

> [!NOTE]
> The `extends` property now refers to another language code, not a file path.
> When present in the common strings layer, the manager will first load the
> parent language (and any further `extends` chains), then apply the current
> file's overrides. Cyclic `extends` chains are detected and will emit a
> warning rather than infinite recursion.

## Common Object IDs and names

| JSON key   | Object ID (TR1) | Object ID (TR2) |
|------------|-----------------|-----------------|
| `key_1`    | 129 and 133     | 193 and 197     |
| `key_2`    | 130 and 134     | 194 and 198     |
| `key_3`    | 131 and 135     | 195 and 199     |
| `key_4`    | 132 and 136     | 196 and 200     |
| `pickup_1` | 141 and 148     | 205 and 207     |
| `pickup_2` | 142 and 149     | 206 and 208     |
| `puzzle_1` | 110 and 114     | 174 and 178     |
| `puzzle_2` | 111 and 115     | 175 and 179     |
| `puzzle_3` | 112 and 116     | 176 and 180     |
| `puzzle_4` | 113 and 117     | 177 and 181     |
| `secret_1` | -               | 190             |
| `secret_2` | -               | 191             |
| `secret_3` | -               | 192             |

> [!NOTE]
> Nearly all pickup items exist in two forms, as early games differentiate
> between a sprite displayed on the ground and a 3D object depicted in the
> inventory ring. Secrets are a notable exception, as they never appear in the
> inventory ring in the original game. For convenience, both forms are defined
> using a single key.

## Translation Workflow

### Translating each layer <a name="translating-each-layer"></a>

To provide localized translations, place language-specific overrides alongside
each base file using the naming pattern `<basename>-<lang>.json5`. Translation
files must live in the same directory as their base (e.g. `cfg/`). For example,
to add French translations:

```text
cfg/base_strings-fr.json5          # common strings
cfg/tr1/strings-fr.json5           # base game strings
cfg/tr1-demo-pc/strings-fr.json5   # TR1 demo overrides
cfg/tr1-level/strings-fr.json5     # custom-level pack overrides
cfg/tr1-ub/strings-fr.json5        # Unfinished Business overrides
cfg/tr2/strings-fr.json5           # base game strings
cfg/tr2-gm/strings-fr.json5        # Golden Mask TR2 overrides
cfg/tr2-level/strings-fr.json5     # custom-level pack overrides
```

When the game starts, TRX will detect these files and load them in place of the
default layer for that language code (`fr` in this example). Omit any
translation file for a layer you do not need; the game will fall back to the
English base for that layer by default.

### Live reloading (`/strings`)

To apply changes to string files without restarting the game, use the `/strings`
console command. It reloads all string layers for the current language (common,
base, and mod-specific), making it easy to test translation or custom overrides
on the fly.

In addition, languages can be switched at runtime without the need to use the
UI with the `/set language <code>` console command (e.g. `/set language fr`).

New language files are only detected at the game launch. If you create a new
layer file, you'll need to relaunch the game to see the effects.

### Review system

The development team uses AI-assisted tools to create initial translations.
Automated translations are tagged with a special marker `\{review}` indicating
that the text needs human review. By default, review markers are hidden in-game.
To enable review mode and display markers, run:

```
/set review 1
```

Translators should remove the `\{review}` tags once the translation has been
reviewed and finalized.

### `language_name`

Only supported in the common strings file (`base_strings.json5`), the
`language_name` property sets the display name of the language in the options
menu. For example:

```json5
{
    "language_name": "Français"
}
```

## Custom levels

### General tips

- **Zero-indexed levels**: Levels are zero-indexed and match the order in the
  game flow file.
- **Minimal overrides**: Only define the strings you need; the game will fall
  back to built-in defaults for any missing entries. For example:
  ```json5
  {
      "levels": [
          {
              "title": "City of Vilcabamba",
              "objects": {
                  "key_1": {"name": "Gold Key"}
              }
          }
      ]
  }
  ```
- **Renaming any object**: You can rename puzzle items, keys, enemies, or any
  other in-game object. For example, TR2 object #&NoBreak;39 (tiger) can be
  renamed to "Snow Leopard" in a winter-themed level.

### Console and object IDs

All objects have names, even if they are not shown in the UI. Use these names
with console commands such as `/tp` or `/give`. For example, `/tp tiger`
teleports Lara to object #&NoBreak;39. Console commands accept partial,
case-insensitive names, and will match unique substrings to objects (powered by
fuzzy matching). In case the player uses languages other than English, the
commands also accept builtin English names (so even though wolf is called a
wilk in Polish, on top of `/tp wilk` the players can still `/tp wolf`).

For a complete list of object IDs for a specific engine, refer to the game
strings files shipped with the relevant TRX builds.
