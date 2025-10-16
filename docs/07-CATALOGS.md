---
title: Catalogs
---

# Catalogs

TombEditor normally lets builders manage WADs and object slots through its
catalogs. The snag is that the game engine itself still references certain
slots directly in code. That means if a builder repurposes one of those
hardcoded slots for an animation command, they might get an ugly surprise when
another in‑game object tries to use that same slot behind the scenes.

That's where TRX catalogs come in.

Before TR1X 4.16 and TR2X 1.6, all object IDs and music tracks were hardcoded.
Now, builders can freely re‑assign those IDs however they want. In the future,
the original lists will allow extensions by including objects from other games!

Under the hood, each entity is identified by its stable name string. The catalog
maps numeric slots to these name keys, so when the engine references an entity,
it can grab the correct sample or resource tied to that slot.

Catalogs are just comma‑separated value (CSV) files you can edit with any text
editor, including Notepad or Excel. They live in the `cfg/` folder and must be
present for the game to function properly.

TRX catalogs only include data that's directly referenced by the game's code.
Entries used *only* in animation commands or other editor‑controlled behaviors
aren't included, since those can already be managed freely within the level
editor.
