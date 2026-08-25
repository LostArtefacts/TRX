-- The give cheat, through the console.
--
-- The fake level holds the two objects of its own - a vase and a key, neither in
-- any pickup family - and real pickups the families do name: a story key, a
-- puzzle piece, a crowbar, a lead bar, a medipack, the scion, a part-full
-- waterskin and a secret's trinket. Its weapons carry no inventory models of
-- their own, so the weapon paths are watched through trx.weapons
-- rather than through what lands in the backpack.

local h = require("harness")
local test = h.test

local function give(args)
  return fake.run("give", args or "")
end

local function count(object)
  return trx.inventory:count(object)
end

local UZIS = trx.catalog.weapons.UZIS

test("a name puts one in the backpack", function()
  assert(give("vase") == trx.console.Result.OK)
  assert(count(fake.VASE) == 1)
  assert(fake.calls().log.message == "Added vase to Lara's inventory")
end)

test("a leading count gives that many", function()
  assert(give("5 vase") == trx.console.Result.OK)
  assert(count(fake.VASE) == 5)
end)

-- A family answers to its own name, so a name that is one hands over every
-- member of it rather than the one object that happens to share the word.
test("a family name gives every member of it", function()
  assert(give("key") == trx.console.Result.OK)
  assert(count(fake.STORY_KEY) == 1)
  assert(count(fake.PUZZLE) == 0, "a puzzle piece is not a key")
end)

-- A player types part of a name, and not always part of the name the object
-- goes by: "big medi" is how a large medipack is asked for.
test("a partial name reaches what answers to it", function()
  assert(give("big ur") == trx.console.Result.OK)
  assert(count(fake.VASE) == 1, "big ur should have reached the vase")
end)

test("a name nothing answers to is refused", function()
  assert(give("kettle") == trx.console.Result.FAILURE)
  assert(fake.calls().log.message == "Unknown item: kettle")
end)

test("nothing at all is refused by the parser", function()
  assert(give("") == trx.console.Result.FAILURE, "what is required")
end)

test("a level that carries no inventory models gives nothing", function()
  fake.set_can_add(false)
  assert(give("key") == trx.console.Result.FAILURE, "nothing is givable")
  assert(count(fake.KEY) == 0)
end)

-- A group that hands over nothing says so, rather than playing the cheat's
-- sound over a backpack that did not change.
test("a group with nothing to hand over says so", function()
  fake.set_can_add(false)
  for _, keyword in ipairs({ "all", "keys", "guns", "moreguns" }) do
    assert(
      give(keyword) == trx.console.Result.FAILURE,
      keyword .. " should report that it gave nothing"
    )
  end
  assert(
    fake.calls().log.message == "This level carries nothing Lara can be given"
  )
  assert(fake.calls().play.count == 0, "and nothing is heard")
end)

-- The parser takes any whole number, so the count answers for its own lower
-- bound rather than letting the backpack raise through it.
test("a count below one is refused in the command's own words", function()
  for _, args in ipairs({ "0 vase", "-1 vase", "0 all" }) do
    assert(give(args) == trx.console.Result.FAILURE, args .. " should fail")
    assert(fake.calls().log.message == "The count has to be one or more")
  end
  assert(count(fake.VASE) == 0)
end)

test("keys gives the plot items and leaves the tools", function()
  assert(give("keys") == trx.console.Result.OK)
  assert(count(fake.STORY_KEY) == 1, "a key fills a numbered slot")
  assert(count(fake.PUZZLE) == 1, "so does a puzzle piece")
  assert(count(fake.TOOL) == 0, "a crowbar is a tool, not a story item")
  assert(fake.calls().play.count == 1, "a cheat is heard as well as seen")
end)

-- The lead bar is in the tool family with the crowbar, and a level that carries
-- one sends Lara to find it.
test("keys gives the lead bar", function()
  assert(give("keys") == trx.console.Result.OK)
  assert(count(fake.LEADBAR) == 1)
  assert(count(fake.TOOL) == 0, "and no other tool comes with it")
end)

test("guns respects what the level allows, moreguns does not", function()
  assert(give("guns") == trx.console.Result.OK)
  assert(trx.inventory:shots(UZIS) == 0, "the level leaves the uzis out")

  assert(give("moreguns") == trx.console.Result.OK)
  assert(trx.inventory:shots(UZIS) == 2000, "and moreguns hands them over")
end)

test("a weapon the level allows comes with its ammunition", function()
  fake.set_weapon_available(UZIS, true)
  assert(give("guns") == trx.console.Result.OK)
  assert(trx.inventory:shots(UZIS) == 2000)
end)

test("a new game plus run tops every weapon up", function()
  fake.set_ngplus(true)
  fake.set_weapon_available(UZIS, true)

  assert(give("moreguns") == trx.console.Result.OK)
  assert(trx.inventory:shots(UZIS) == 10001)
end)

test("all gives one of everything, and the supplies by the ten", function()
  fake.set_weapon_available(UZIS, true)
  assert(give("all") == trx.console.Result.OK)

  assert(count(fake.STORY_KEY) == 1, "the story item")
  assert(count(fake.TOOL) == 1, "and the tool, once")
  assert(trx.inventory:shots(UZIS) == 2000, "with the guns loaded")
  assert(
    count(trx.catalog.objects.SMALL_MEDIPACK) == 10,
    "the supplies come by the ten"
  )
end)

-- A pickup with more than one state answers to its name through every one of
-- them, and they all go into the same backpack entry.
test("the scion comes once, whichever of its states answered", function()
  assert(give("scion") == trx.console.Result.OK)

  assert(count(fake.SCION) == 1, "one scion, not one per state")
  assert(count(fake.SCION_2) == 1, "and it is the same entry either way")
  assert(fake.calls().log.count == 1, "said once, too")
end)

test("a count still reaches a pickup with several states", function()
  assert(give("3 scion") == trx.console.Result.OK)
  assert(count(fake.SCION) == 3)
end)

-- The one pickup no group hands over, so a player who wants one asks for it.
test("the crystal comes when it is named", function()
  assert(give("crystal") == trx.console.Result.OK)
  assert(count(fake.CRYSTAL) == 1)
  assert(fake.calls().log.message == "Added crystal to Lara's inventory")
end)

-- A pickup in none of the families is a second state of something Lara already
-- carries, which is not hers to be handed over again, so the groups name what
-- they give rather than sweeping up what is left.
test("all leaves what no group names alone", function()
  assert(give("all") == trx.console.Result.OK)
  assert(count(fake.VASE) == 0, "a pickup in no family at all")
end)

-- A secret's trinket goes into no backpack entry, and the level counts its
-- secrets apart, so /secret is what hands one over.
test("a secret is not an item /give knows", function()
  assert(give("secret") == trx.console.Result.FAILURE)
  assert(fake.calls().log.message == "Unknown item: secret")

  assert(give("all") == trx.console.Result.OK)
  assert(count(fake.TRINKET) == 0)
end)

test("nor is one offered for completion", function()
  for _, candidate in ipairs(fake.complete_args("give", "")) do
    assert(candidate ~= "secret", "a secret should not be offered")
  end
end)

-- A tool with more than one state shares the base's backpack entry, so it is
-- handed over once and as the base.
test("a part-full waterskin does not come as well as the empty one", function()
  assert(give("all") == trx.console.Result.OK)
  assert(count(fake.TOOL) == 1)
  assert(count(fake.WATERSKIN) == 1, "the same entry, read the other way")
end)

test("all gives the scion and never the crystal", function()
  assert(give("all") == trx.console.Result.OK)
  assert(count(fake.SCION) == 1, "the scion is a pickup, and comes with all")
  assert(count(fake.CRYSTAL) == 0, "the crystal is not, and does not")
end)

test("nor does keys, which the crystal sits beside", function()
  assert(give("keys") == trx.console.Result.OK)
  assert(count(fake.CRYSTAL) == 0)
  assert(count(fake.SCION) == 1, "the scion is what TR1 sends her for")
end)

test("the keyword commands reach the same cheats", function()
  assert(fake.run("keys", "") == trx.console.Result.OK)
  assert(count(fake.STORY_KEY) == 1)

  fake.set_weapon_available(UZIS, true)
  assert(fake.run("guns", "") == trx.console.Result.OK)
  assert(trx.inventory:shots(UZIS) == 2000)

  assert(fake.run("moreguns", "") == trx.console.Result.OK)
end)

test("completion offers the keywords and the names together", function()
  local out = fake.complete_args("give", "")
  local seen = {}
  for _, candidate in ipairs(out) do
    seen[candidate] = true
  end
  for _, keyword in ipairs({ "all", "keys", "guns", "moreguns" }) do
    assert(seen[keyword], keyword .. " should be offered")
  end
  assert(seen["vase"], "and what the level can give")
end)

test("every one of them needs a level", function()
  fake.set_current_level(nil)
  for _, command in ipairs({ "give", "keys", "guns", "moreguns" }) do
    local args = command == "give" and "key" or ""
    assert(
      fake.run(command, args) == trx.console.Result.UNAVAILABLE,
      command .. " should be unavailable"
    )
  end
end)

return h.report()
