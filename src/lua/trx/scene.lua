local raw = trxc.scene
local h = require("trx.internal.helpers")

require("trx.math")

---@class trx
---@field scene trx.scene

---Outlines a script draws into the world the camera is looking at, over the
---level geometry rather than over the interface.
---
---The calls are available from `trx.events.on_scene_paint` and nowhere else,
---and raise anywhere else. Nothing is remembered between frames: a shape that
---is to stay on screen is drawn again every time the event fires.
---
---A shape is placed the way an item position and a zone are, so it needs no
---room and belongs to none. The outlines are drawn as wireframe, and one
---reaching further from its middle than a level is wide draws at that limit
---instead.
---@trx.module 44
---@class (exact) trx.scene
local M = h.module("scene")

---Draws the outline of a world-space box. The corners may come in any order.
---
---```lua
---trx.events.on_scene_paint(function()
---  trx.scene.box(
---    { x = 51200, y = -2048, z = 30720 },
---    { x = 53248, y = 0, z = 32768 },
---    "00ff00")
---end)
---```
---@param min trx.math.Vec3 One corner of the box.
---@param max trx.math.Vec3 The opposite corner of the box.
---@param color trx.math.Color The color of the outline.
---@param alpha? integer How solid the outline is, counted 0 to 255.
---@trx.default alpha 255
---@type fun(min: trx.math.Vec3, max: trx.math.Vec3, color: trx.math.Color, alpha?: integer)
M.box = raw.box

---Draws the outline of a sphere.
---
---```lua
---trx.events.on_scene_paint(function()
---  trx.scene.sphere(trx.lara.item.pos, 2048, "00ff00", 128)
---end)
---```
---@param centre trx.math.Vec3 Middle of the sphere.
---@param radius trx.math.Distance How far out it reaches.
---@param color trx.math.Color The color of the outline.
---@param alpha? integer How solid the outline is, counted 0 to 255.
---@trx.default alpha 255
---@type fun(centre: trx.math.Vec3, radius: trx.math.Distance, color: trx.math.Color, alpha?: integer)
M.sphere = raw.sphere
