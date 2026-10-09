local raw = trxc.stats
local h = require("trx.internal.helpers")

---@class (partial) trx
---@field stats trx.stats

---Module for what a level keeps count of: what Lara has found in it, and how
---much there was to find.
---
---The module is the level being played, so `trx.stats.pickups.count` is what
---she has picked up in it. Any other level's counters are reached the same way
---through `trx.game.Level.stats`. At the title screen there is no level, and
---everything here reads `nil`.
---@trx.module 29
---@class (exact) trx.stats: trx.stats.Stats
local M = h.module("stats")

-- The categories are ordered as the engine keeps them, so a name here stands
-- for the number the C side addresses one by.
local CATEGORY = {
  pickups = 0,
  kills = 1,
  secrets = 2,
  crystals = 3,
}

---One thing a level is counted on, which is one row of the statistics screen.
---`trx.stats.Category.raw` is `trx.stats.Category.max` plus
---`trx.stats.Category.unobtainable`: the game flow can declare part of a level
---out of reach, and what it writes off is left out of what counts towards
---completion while still being in the level.
---@class (exact) trx.stats.Category
---@trx.readonly max, raw, unobtainable
---@field count integer How many of them Lara has. The secrets cannot be set this way: they are held one by one, so `trx.stats.give_secret` and `trx.stats.take_secret` are how they change.
---@field max integer How many of them count towards completing the level.
---@field raw integer How many of them the level holds, obtainable or not.
---@field unobtainable integer How many of them the game flow declares out of reach, and so must not be held against the player.
local Category = h.handle("stats.Category", "STATS_CATEGORY", {
  fields = {
    count = "count",
    max = "max",
    raw = "raw",
    unobtainable = "unobtainable",
  },
  writable = { "count" },
})

local function category(name)
  return function(stats)
    return raw.category(stats, CATEGORY[name])
  end
end

---The secret's number, as the player counts them.
---@trx.base 1
---@alias trx.stats.SecretNum integer

---What one level keeps count of. The counters are the level's own and can be
---written, which is what a script correcting or seeding them wants.
---@class (exact) trx.stats.Stats
---@field timer trx.game.Frames How long the level has been played.
---@field deaths integer How many times Lara has died. Unlike the rest, this is not cleared when the level is entered again: a death stays with the level it happened on.
---@field ammo_used integer How many rounds Lara has fired.
---@field ammo_hits integer How many of them hit something.
---@field distance_travelled trx.math.Distance How far Lara has travelled.
---@field medipacks_used number How many medipacks Lara has used, a small one counting as half of one.
---@field pickups trx.stats.Category The items lying in the level for Lara to take.
---@field kills trx.stats.Category The enemies the level counts, allies among them.
---@field secrets trx.stats.Category The level's secrets. Which ones Lara holds is `trx.stats.Stats.secret_list`.
---@field crystals trx.stats.Category The save crystals, where the game has them.
---@field max_ally_kills integer How many of `trx.stats.Stats.kills.max` are allies. The statistics screen holds them against the player only once `trx.stats.Stats.allies_hurt`, so a screen written in Lua wants to do the same: `trx.stats.Stats.max_enemy_kills`, and these as well once she has turned on one.
---@field max_enemy_kills integer How many of `trx.stats.Stats.kills.max` are enemies rather than allies.
---@field allies_hurt boolean Whether Lara has turned on an ally in this level.
local Stats = h.handle("stats.Stats", "LEVEL_STATS", {
  fields = {
    timer = "timer",
    deaths = "death_count",
    ammo_used = "ammo_used",
    ammo_hits = "ammo_hits",
    distance_travelled = "distance_travelled",
    medipacks_used = "medipacks_used",
  },
  writable = {
    "timer",
    "deaths",
    "ammo_used",
    "ammo_hits",
    "distance_travelled",
    "medipacks_used",
  },
  extensions = {
    pickups = category("pickups"),
    kills = category("kills"),
    secrets = category("secrets"),
    crystals = category("crystals"),

    max_ally_kills = function(stats)
      return (raw.kill_split(stats))
    end,
    max_enemy_kills = function(stats)
      return select(2, raw.kill_split(stats))
    end,
    allies_hurt = raw.allies_hurt,
  },
})

---@class (exact) trx.stats.Stats.secret_list.secret
---@field num trx.stats.SecretNum Which secret it is.
---@field found boolean Whether Lara has it.
---@field icon? integer Which secret glyph draws it, as `\\{secret N}` names one, or `nil` for a secret that is a place to reach rather than an item to pick up.

---The level's secrets, in order.
---@return trx.stats.Stats.secret_list.secret[] # The secrets, one by one.
function Stats:secret_list()
  return h.native()
end

---Marks a secret as found, as walking into its trigger would.
---@param secret_num trx.stats.SecretNum
---@return boolean # `false` if the level has no such secret, or Lara already has it.
function Stats:give_secret(secret_num)
  return h.native()
end

---Takes a secret back, leaving it to be found again.
---@param secret_num trx.stats.SecretNum
---@return boolean # `false` if the level has no such secret, or Lara does not have it.
function Stats:take_secret(secret_num)
  return h.native()
end

h.mirror(M, "stats", raw.get_current, "stats.Stats")

---Asks `trx.stats.Stats:secret_list` of the level being played. Where none is
---being played, the list is empty.
---
---```lua
---for _, secret in ipairs(trx.stats.secret_list()) do
---  trx.log.info(secret.num .. ": " .. tostring(secret.found))
---end
---```
---@return trx.stats.Stats.secret_list.secret[] # The secrets, one by one.
function M.secret_list()
  local stats = raw.get_current()
  return stats ~= nil and stats:secret_list() or {}
end

---Asks `trx.stats.Stats:give_secret` of the level being played.
---
---```lua
---trx.stats.give_secret(1)
---```
---@param secret_num trx.stats.SecretNum
---@return boolean # `false` if no level is being played, the level has no such secret, or Lara already has it.
function M.give_secret(secret_num)
  local stats = raw.get_current()
  return stats ~= nil and stats:give_secret(secret_num)
end

---Asks `trx.stats.Stats:take_secret` of the level being played.
---@param secret_num trx.stats.SecretNum
---@return boolean # `false` if no level is being played, the level has no such secret, or Lara does not have it.
function M.take_secret(secret_num)
  local stats = raw.get_current()
  return stats ~= nil and stats:take_secret(secret_num)
end

local _ = Category
