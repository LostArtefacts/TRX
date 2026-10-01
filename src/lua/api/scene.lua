local raw = trxc.scene
local api = trx.api

require("trx.math")

api.module("scene", {
  order = 44,
  description = [[
Outlines a script draws into the world the camera is looking at, over the level
geometry rather than over the interface.

The calls are available from `trx.events.on_scene_paint` and nowhere else, and
raise anywhere else. Nothing is remembered between frames: a shape that is to
stay on screen is drawn again every time the event fires.

A shape is placed the way an item position and a zone are, so it needs no room
and belongs to none. The outlines are drawn as wireframe, and one reaching
further from its middle than a level is wide draws at that limit instead.
]],
})

local COLOR = {
  name = "color",
  type = "math.Color",
  description = "The color of the outline.",
}

local ALPHA = {
  name = "alpha",
  type = "integer",
  optional = true,
  default = 255,
  description = "How solid the outline is, counted 0 to 255.",
}

api.define("scene.box", {
  description = "Draws the outline of a world-space box. The corners may come in any order.",
  params = {
    {
      name = "min",
      type = "math.Vec3",
      description = "One corner of the box.",
    },
    {
      name = "max",
      type = "math.Vec3",
      description = "The opposite corner of the box.",
    },
    COLOR,
    ALPHA,
  },
  examples = {
    [[trx.events.on_scene_paint(function()
  trx.scene.box(
    { x = 51200, y = -2048, z = 30720 },
    { x = 53248, y = 0, z = 32768 },
    "00ff00")
end)]],
  },
  impl = raw.box,
})

api.define("scene.sphere", {
  description = "Draws the outline of a sphere.",
  params = {
    {
      name = "centre",
      type = "math.Vec3",
      description = "Middle of the sphere.",
    },
    {
      name = "radius",
      type = "math.Distance",
      description = "How far out it reaches.",
    },
    COLOR,
    ALPHA,
  },
  examples = {
    [[trx.events.on_scene_paint(function()
  trx.scene.sphere(trx.lara.item.pos, 2048, "00ff00", 128)
end)]],
  },
  impl = raw.sphere,
})
