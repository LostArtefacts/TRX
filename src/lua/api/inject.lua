local raw = trxc.inject
local api = trx.api

api.module("inject", {
  order = 42,
  title = "Injection",
  description = [[
The content a mod brings with it: meshes, animations, sounds and other data.

A game flow names the injections its levels load. A script can name additional
injections, so a mod can ship its content without changing a game flow.]],
})

api.define("inject.declare", {
  description = [[
Adds injections to every level, in addition to those named by its game flow.

The function runs before each level loads its content. It can read the current
settings and return a different list for each level.

Return file names, not paths. The game searches for them in the same order as
game-flow injections: in the mod first, then in the base game. Use
`trx.path.resolve` to check whether a file exists first.]],
  params = {
    {
      name = "declaration",
      type = "function",
      description = "Called before each level loads and returns a list of file names.",
    },
  },
  examples = {
    [[trx.inject.declare(function()
  local files = { "mymod_models.bin" }
  if trx.config.get("visuals.enable_ps1_crystals") then
    files[#files + 1] = "wall_crystals.bin"
  end
  return files
end)]],
  },
  impl = raw.declare,
})
