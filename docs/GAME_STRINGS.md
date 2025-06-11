# TRX Game Strings Configuration

This document describes how to use the game strings. Game strings let users
translate the game and change the built-in object names for the purpose of
building custom levels. The configuration includes definitions for game levels,
objects, and all UI elements, and is structured in a JSON5 format which
permits comments.

## General structure

The document is organized as follows:

```json5
{
    "levels": {
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
    },
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
      <code>/tp</code> and <code>/give</code>
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
</table>

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

## Usage Guidelines
- Levels are zero-indexed and match the order with the game flow file.
- It doesn't make sense to ship a custom level with all of the strings defined.
  We encourage you to make your strings file as small as possible - the game
  will fall back to the built-in defaults for you! For example, the following
  document is perfectly fine:
  ```json5
  {
      "levels": [
          {
              "title": "City of Vilcabamba",
              "objects": {
                  {"name": "key_1": "Gold Key"},
              },
          },
      ]
  }
  ```
- Sometimes it makes sense to rename not just puzzle items and keys, but also
  other objects, such as enemies. A great example of this is TR2 object
  #&NoBreak;39 - usually it's a tiger, but in the snow levels it becomes a snow
  leopard.
- TRX gives a name to all objects. Even when the player cannot normally see
  names of objects other than pickups and special inventory ring items, these
  are used in console commands such as `/tp` and `/give` – for example, `/tp
  tiger` should teleport Lara to the object #&NoBreak;39.
- For a full list of object IDs for a particular engine, please refer to the
  game strings files shipped with relevant TRX builds.
