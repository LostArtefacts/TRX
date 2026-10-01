local raw = trxc.inventory_ring
local api = trx.api

api.module("inventory_ring", {
  order = 5,
  title = "Inventory ring",
  description = [[
The rings the player browses, and the entries drawn on them.

This is the front of the inventory: which entries a ring holds, and how each is
drawn and turned. What Lara is carrying belongs to `trx.inventory`.]],
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
