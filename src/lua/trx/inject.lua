local raw = trxc.inject
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field inject trx.inject

---The content a mod brings with it: meshes, animations, sounds and other data.
---
---A game flow names the injections its levels load. A script can name
---additional injections, so a mod can ship its content without changing a game
---flow.
---@trx.module 43 Injection
---@class (exact) trx.inject
local M = h.module("inject")

---Adds injections to every level, in addition to those named by its game flow.
---
---The function runs before each level loads its content. It can read the
---current settings and return a different list for each level.
---
---Return file names, not paths. A file beside the script is searched first.
---Other files are searched in the same order as game-flow injections: in the
---mod first, then in the base game. Use `trx.path.resolve` to check whether a
---file exists.
---
---```lua
---trx.inject.declare(function()
---  local files = { "mymod_models.bin" }
---  if trx.config.get("visuals.enable_ps1_crystals") then
---    files[#files + 1] = "wall_crystals.bin"
---  end
---  return files
---end)
---```
---@param declaration function Called before each level loads and returns a list of file names.
---@type fun(declaration: function)
M.declare = raw.declare
