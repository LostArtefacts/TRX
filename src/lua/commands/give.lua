-- Puts things in Lara's backpack.
--
-- Usages:
--   /give uzi        the object a name reaches
--   /give 5 flare    five of them
--   /give keys       every plot item the level has a place for
--   /give guns       every weapon the level allows, with ammunition
--   /give moreguns   every weapon, even one the level does not carry
--   /give all        one of everything, with the counts a cheat gives
--
-- /keys, /guns and /moreguns reach the last three on their own.

trx.locale.declare({
  ["console/cmd/give/added"] = "Added %s to Lara's inventory",
  ["console/cmd/give/all_given"] = "Lara's backpack just got way heavier!",
  ["console/cmd/give/bad_count"] = "The count has to be one or more",
  ["console/cmd/give/guns_given"] = "Lock'n'load - Lara's armed to the teeth!",
  ["console/cmd/give/guns_help"] = "Gives Lara every weapon the level allows, with ammunition.",
  ["console/cmd/give/help"] = "Adds a given item to Lara's inventory.",
  ["console/cmd/give/invalid"] = "Unknown item: %s",
  ["console/cmd/give/keys_given"] = "Surprise! Every key item Lara needs is now in her backpack.",
  ["console/cmd/give/keys_help"] = "Gives Lara every plot item the level has a place for.",
  ["console/cmd/give/moreguns_help"] = "Gives Lara every weapon, including the ones the level leaves out.",
  ["console/cmd/give/nothing"] = "This level carries nothing Lara can be given",
  ["console/cmd/give/what_help"] = "what to give: all, keys, guns, moreguns, or an item by name",
})

-- The keywords /give answers to besides a name. Each is also a command of its
-- own, so the player can type the short form.
local KEYWORDS = { "all", "keys", "guns", "moreguns" }

-- Every weapon a cheat hands over, in the order it hands them over, and how
-- much ammunition each comes with. A new game plus run tops every one of them
-- up instead. The pistols are not here: they come regardless of what the level
-- says, and never run down.
local NGPLUS_AMMO = 10001
local ARSENAL = {
  { trx.catalog.weapons.SHOTGUN, 300 },
  { trx.catalog.weapons.MAGNUMS, 1000 },
  { trx.catalog.weapons.AUTOS, 1000 },
  { trx.catalog.weapons.DESERT_EAGLE, 1000 },
  { trx.catalog.weapons.UZIS, 2000 },
  { trx.catalog.weapons.HARPOON, 300 },
  { trx.catalog.weapons.M16, 300 },
  { trx.catalog.weapons.MP5, 300 },
  { trx.catalog.weapons.GRENADE, 300 },
  { trx.catalog.weapons.ROCKET, 300 },
  { trx.catalog.weapons.CROSSBOW, 300 },
  { trx.catalog.weapons.REVOLVER, 1000 },
}

-- The savegame crystal is not a pickup: Lara does not walk over one, and the
-- stats count it apart. It has an inventory icon all the same, so the cheat can
-- hand her one - by name only, since it is in none of the families a group
-- reaches.
local CRYSTAL = trx.catalog.objects.SAVE_CRYSTAL

-- The lead bar fills no numbered slot, so its family is `tool` alongside the
-- crowbar and the binoculars. A level that carries one has a Midas hand to put
-- it in, which is what makes it a plot item to the player looking for it, so it
-- is named here rather than reached through a family.
local LEADBAR = trx.catalog.objects.LEAD_BAR

-- A pickup with several states goes into one backpack entry - the scion whether
-- or not Lara carries it, a waterskin at each fill level - and answers to one
-- name through all of them. Whichever a group or a name reaches, she is given
-- it once.

-- How many went in, so a cheat that found nothing to hand over can say so
-- rather than announcing a backpack that never got heavier.
local function add_once(seen, id, count)
  local icon = trx.inventory:icon_of(id)
  if seen[icon] then
    return 0
  end
  seen[icon] = true
  return trx.inventory:give(id, count or 1)
end

local function can_add(id)
  return trx.inventory:can_add(id)
end

-- What can be given at all: a pickup the level carries the inventory model for,
-- which is not the same as one lying about in it, and the crystal. A secret's
-- trinket is left out - it goes into no backpack entry, and the level counts
-- its secrets apart, so /secret is what hands one over.
local function givable()
  local q = trx.objects.query
  return (q:pickup() | q:where(function(id)
    return id == CRYSTAL
  end)):where(can_add) & ~q:secret()
end

-- The plot items: everything filling a numbered slot, which is what a level
-- asks Lara to go and find, and the lead bar. The rest of the tools she carries
-- and uses are not among them - those come with `all`, or by name.
local function plot_items()
  local q = trx.objects.query
  return q:key()
    | q:puzzle()
    | q:quest()
    | q:examine()
    | q:collectible()
    | q:where(function(id)
      return id == LEADBAR
    end)
end

local function give_gun(weapon, ammo, ignore_exclusions)
  if not ignore_exclusions and not trx.weapons.is_available(weapon) then
    return 0
  end
  local object = trx.weapons.object(weapon)
  if object == nil or trx.inventory:give(object) == 0 then
    return 0
  end
  trx.inventory:set_shots(weapon, trx.game.is_ngplus and NGPLUS_AMMO or ammo)
  return 1
end

local function give_guns(ignore_exclusions)
  local given = trx.inventory:give(trx.catalog.objects.PISTOLS)
  for _, entry in ipairs(ARSENAL) do
    given = given + give_gun(entry[1], entry[2], ignore_exclusions)
  end
  return given
end

local function give_plot_items(seen)
  local given = 0
  for _, id in ipairs((givable() & plot_items()):ids()) do
    given = given + add_once(seen, id)
  end
  return given
end

local function give_supplies()
  -- What a cheat hands over rather than one of.
  local SUPPLY_COUNT = 10
  local SUPPLIES = {
    trx.catalog.objects.SMALL_MEDIPACK,
    trx.catalog.objects.LARGE_MEDIPACK,
  }

  local given = 0
  for _, object in ipairs(SUPPLIES) do
    given = given + trx.inventory:give(object, SUPPLY_COUNT)
  end
  -- Flares come only where they are Lara's to carry.
  if trx.weapons.is_available(trx.catalog.weapons.FLARE) then
    given = given
      + trx.inventory:give(trx.catalog.objects.FLARES_BOX, SUPPLY_COUNT)
  end
  return given
end

-- The tools Lara carries and uses. The weapons and their ammunition are left to the
-- weapon path, which sets the ammunition, and the supplies to their counts.
--
-- Named rather than reached by subtracting the other groups from the pickups: a
-- pickup in none of the families is a second state of something Lara already
-- has, which is not hers to be given again. The savegame crystal is left out
-- too, being the one thing a player has to ask for by name.
local function give_tools(seen)
  local given = 0
  for _, id in ipairs((givable() & trx.objects.query:tool()):ids()) do
    given = given + add_once(seen, id)
  end
  return given
end

local function give_named(name, count)
  local ids = givable():by_name(name):best()
  if #ids == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/give/invalid", name)
  end

  local seen = {}
  for _, id in ipairs(ids) do
    if add_once(seen, id, count) > 0 then
      trx.console.log(
        trx.locale.format(
          "console/cmd/give/added",
          trx.objects[id].names[1] or name
        )
      )
    end
  end
  return trx.console.Result.OK
end

-- A group hands over what the level carries the inventory models for, and a
-- level may carry none of them. Saying so beats the sound and the boast over an
-- unchanged backpack.
local function announce(given, key, sample)
  if given == 0 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/give/nothing")
  end
  trx.sound.play(sample)
  return trx.console.Result.OK, trx.locale.get(key)
end

local function give_all()
  local given = give_guns(false)
  -- One `seen` for the whole cheat: a variant reached by one group and its
  -- base by another are still the one thing.
  local seen = {}
  given = given + give_plot_items(seen)
  given = given + give_tools(seen)
  given = given + give_supplies()
  return announce(
    given,
    "console/cmd/give/all_given",
    trx.catalog.samples.LARA_HOLSTER
  )
end

local function give_keys()
  return announce(
    give_plot_items({}),
    "console/cmd/give/keys_given",
    trx.catalog.samples.LARA_KEY
  )
end

local function give_all_guns(ignore_exclusions)
  return announce(
    give_guns(ignore_exclusions),
    "console/cmd/give/guns_given",
    trx.catalog.samples.LARA_RELOAD
  )
end

local function run(what, count)
  if not trx.game.is_playable then
    return trx.console.Result.UNAVAILABLE
  end

  -- The parser takes any whole number, so the lower bound is answered here
  -- rather than by trx.inventory:give, which raises.
  if count < 1 then
    return trx.console.Result.FAILURE,
      trx.locale.get("console/cmd/give/bad_count")
  end

  local keyword = what:lower()
  if keyword == "all" then
    return give_all()
  elseif keyword == "keys" then
    return give_keys()
  elseif keyword == "guns" then
    return give_all_guns(false)
  elseif keyword == "moreguns" then
    return give_all_guns(true)
  end

  return give_named(what, count)
end

trx.console.register({
  name = "give",
  help = "console/cmd/give/help",
  args = function(parser)
    parser:positional("count", { type = "integer", optional = true })
    parser:rest("what", {
      help = "console/cmd/give/what_help",
      suggest = function()
        local out = {}
        for _, keyword in ipairs(KEYWORDS) do
          out[#out + 1] = keyword
        end
        for _, name in ipairs(givable():names()) do
          out[#out + 1] = name
        end
        return out
      end,
    })
  end,
  run = function(args)
    return run(args.what, args.count or 1)
  end,
})

-- The keyword commands. Each is the same cheat, reached by the word alone.
for _, spec in ipairs({
  { name = "keys", help = "console/cmd/give/keys_help", give = give_keys },
  {
    name = "guns",
    help = "console/cmd/give/guns_help",
    give = function()
      return give_all_guns(false)
    end,
  },
  {
    name = "moreguns",
    help = "console/cmd/give/moreguns_help",
    give = function()
      return give_all_guns(true)
    end,
  },
}) do
  trx.console.register({
    name = spec.name,
    help = spec.help,
    run = function()
      if not trx.game.is_playable then
        return trx.console.Result.UNAVAILABLE
      end
      return spec.give()
    end,
  })
end
