local raw = trxc.scene
local raw_events = trxc.events
local h = require("trx.internal.helpers")

require("trx.math")

---@class (partial) trx
---@field scene trx.scene

---Outlines a script draws into the world the camera is looking at, over the
---level geometry rather than over the interface.
---
---The calls are available from `trx.scene.on_paint` and nowhere else,
---and raise anywhere else. Nothing is remembered between frames: a shape that
---is to stay on screen is drawn again every time it happens.
---
---A shape is placed the way an item position and a zone are, so it needs no
---room and belongs to none. The outlines are drawn as wireframe, and one
---reaching further from its middle than a level is wide draws at that limit
---instead.
---@trx.module 44
---@class (exact) trx.scene
local M = h.module("scene")

-- The engine fires an event as it paints the scene. The types are reflected
-- out of ENUM_MAP, as trx.events reads them.
local types = {}
for _, constant in ipairs(trxc.enum.values("LUA_EVENT_TYPE")) do
  types[constant.name] = constant.value
end
local Listener = h.class_of("events.Listener")

---Happens on every drawn frame, after the rooms and everything standing in
---them, and before the interface. The drawing calls here work during it and
---raise anywhere else. It follows the frame rate, not the game clock.
---
---```lua
---trx.scene.on_paint(function()
---  trx.scene.sphere(trx.lara.item.pos, 512, "00ff00")
---end)
---```
---@param callback function What to run when it happens.
---@return trx.events.Listener # The attached handler.
---@type fun(callback: function): trx.events.Listener
function M.on_paint(callback)
  return setmetatable(
    { _id = raw_events.attach(types.SCENE_PAINT, callback) },
    Listener
  )
end

---Draws the outline of a world-space box. The corners may come in any order.
---
---```lua
---trx.scene.on_paint(function()
---  trx.scene.box(
---    { x = 51200, y = -2048, z = 30720 },
---    { x = 53248, y = 0, z = 32768 },
---    "00ff00")
---end)
---```
---@param min trx.math.Vec3 One corner of the box.
---@param max trx.math.Vec3 The opposite corner of the box.
---@param color trx.math.Color|string The color of the outline, or the hex text one is written as.
---@param alpha? integer How solid the outline is, counted 0 to 255.
---@trx.default alpha 255
---@type fun(min: trx.math.Vec3, max: trx.math.Vec3, color: trx.math.Color|string, alpha?: integer)
M.box = raw.box

---Draws the outline of a sphere.
---
---```lua
---trx.scene.on_paint(function()
---  trx.scene.sphere(trx.lara.item.pos, 2048, "00ff00", 128)
---end)
---```
---@param centre trx.math.Vec3 Middle of the sphere.
---@param radius trx.math.Distance How far out it reaches.
---@param color trx.math.Color|string The color of the outline, or the hex text one is written as.
---@param alpha? integer How solid the outline is, counted 0 to 255.
---@trx.default alpha 255
---@type fun(centre: trx.math.Vec3, radius: trx.math.Distance, color: trx.math.Color|string, alpha?: integer)
M.sphere = raw.sphere
