local raw = trxc.inventory_ring
local api = trx.api

api.module("inventory_ring", {
  order = 5,
  title = "Inventory ring",
  description = [[
The rings the player browses, and the entries drawn on them.

This is the front of the inventory: which entries a ring holds, how each is
drawn and turned, and what the player has picked. What Lara is carrying belongs
to `trx.inventory`.

A script draws what an entry opens by defining the
`trx.ui.Screen.RING_ENTRY` screen.]],
})

api.enum("inventory_ring.Mode", {
  backing = "INVENTORY_MODE",
  description = "What the inventory ring was opened for.",
  values = {
    GAME = "Opened during play.",
    TITLE = "The title screen's menu.",
    KEYS = "The keys ring, opened against a locked door or receptacle.",
    SAVE = "Opened to save, with the save list already on show.",
    LOAD = "Opened to load, with the save list already on show.",
    DEATH = "Opened because Lara died.",
    SAVE_CRYSTAL = "Opened by a save crystal.",
    GLOBE_SELECT = "The globe the player picks a destination from.",
  },
})

api.define("inventory_ring.mode", {
  description = "What the open ring was opened for, or `nil` when no ring is open.",
  returns = {
    type = "inventory_ring.Mode",
    nullable = true,
    description = "What the ring was opened for.",
  },
  impl = raw.mode,
})

api.type("inventory_ring.EntryAnim", {
  description = "The animation state of the selected entry.",
  fields = {
    frame = { type = "integer", description = "The frame on show." },
    goal_frame = {
      type = "integer",
      description = "The frame the entry is animating towards.",
    },
    open_frame = {
      type = "integer",
      description = "The frame the entry rests on once it has opened.",
    },
    frame_count = {
      type = "integer",
      description = "How many frames the entry's animation holds.",
    },
    direction = {
      type = "integer",
      description = "Which way the animation runs: `1` forwards, `-1` backwards.",
    },
  },
})

api.define("inventory_ring.selection_anim", {
  description = "Where the ring's selected entry is in its animation, or `nil` when no ring is "
    .. "open.",
  returns = {
    type = "inventory_ring.EntryAnim",
    nullable = true,
    description = "The entry's animation state.",
  },
  impl = raw.selection_anim,
})

api.define("inventory_ring.animate_selection", {
  description = "Runs the ring's selected entry to a frame of its animation.",
  params = {
    {
      name = "goal_frame",
      type = "integer",
      description = "The frame to stop on.",
    },
    {
      name = "direction",
      type = "integer",
      description = "Which way to run: `1` forwards, `-1` backwards.",
    },
  },
  examples = {
    [[-- turn the passport to its second page
local anim = trx.inventory_ring.selection_anim()
trx.inventory_ring.animate_selection(anim.open_frame + 5, 1)]],
  },
  impl = raw.animate_selection,
})

api.define("inventory_ring.icon_of", {
  description = [[
Returns the inventory icon for a pickup, whether or not Lara has one.]],
  params = {
    {
      name = "object",
      type = "catalog.objects",
      description = "The pickup to check.",
    },
  },
  returns = {
    type = "catalog.objects",
    nullable = true,
    description = "The icon's object id, or `nil` for a pickup that has none.",
  },
  impl = raw.icon_of,
})

api.define("inventory_ring.item", {
  description = [[
Returns the ring entry for an object.

Returns `nil` when no entry names the object.]],
  params = {
    {
      name = "object",
      type = "catalog.objects",
      description = "The inventory icon to read.",
    },
  },
  returns = {
    type = "table",
    description = [[The entry's `object_id`, frame counts, rotations, offsets,
`scale`, and `draws_at_pivot`, or `nil`.
<!--noref: object_id-->
<!--noref: scale, draws_at_pivot-->]],
  },
  impl = raw.item,
})

api.define("inventory_ring.declare_item", {
  description = [[
Adds an object to the inventory ring and sets its display properties.

Declaring an existing object replaces its entry. Use the pickup object for an
entry that represents itself.]],
  params = {
    {
      name = "spec",
      type = "table",
      description = [[The entry's `object_id`, frame counts, rotations, offsets,
`scale`, and `draws_at_pivot`. An omitted value keeps the ring's default.
<!--noref: object_id-->
<!--noref: scale, draws_at_pivot-->]],
    },
  },
  examples = {
    [[trx.inventory_ring.declare_item({
  object_id = "mymod:lantern_item",
  frames_total = 1,
  anim_direction = 1,
  anim_speed = 1,
  scale = 1.0,
  meshes_sel = -1,
  meshes_drawn = -1,
  inv_pos = 20,
})]],
  },
  impl = function(spec)
    assert(
      type(spec) == "table",
      "trx.inventory_ring.declare_item expects a table"
    )
    return raw.declare_item(spec)
  end,
})
