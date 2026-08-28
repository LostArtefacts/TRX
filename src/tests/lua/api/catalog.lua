-- The catalog API as a script sees it. The catalogs are the real enums, so
-- every name below is one the engine has; only the slot mapping is faked.

local h = require("harness")
local test, raises = h.test, h.raises

test("a catalog holds the names the engine has", function()
  assert(trx.catalog.objects.WOLF ~= nil, "no wolf")
  assert(trx.catalog.objects.LARA ~= nil)
  assert(trx.catalog.samples.LARA_NO ~= nil)
  assert(trx.catalog.weapons.SHOTGUN ~= nil)
end)

test("a catalog answers to a name in any case", function()
  assert(trx.catalog.objects.wolf == trx.catalog.objects.WOLF)
  assert(trx.catalog.objects.Wolf == trx.catalog.objects.WOLF)
  assert(trx.catalog.weapons.desert_eagle == trx.catalog.weapons.DESERT_EAGLE)
end)

test("a name the engine does not have is nil, in any case", function()
  assert(trx.catalog.objects.WOMBAT == nil)
  assert(trx.catalog.objects.wombat == nil)
end)

test("a catalog cannot be written to", function()
  raises(function()
    trx.catalog.objects.WOLF = 5
  end)
  raises(function()
    trx.catalog.objects.wombat = 5
  end)
  assert(trx.catalog.objects.WOLF ~= 5, "the write must not have landed")
end)

test("pairs() yields the canonical spelling", function()
  local count = 0
  for name, value in pairs(trx.catalog.weapons) do
    assert(name == name:upper(), "pairs() must yield upper case: " .. name)
    assert(type(value) == "number")
    count = count + 1
  end
  assert(count > 1, "the catalog is empty")
end)

test("a slot is not a TRX id", function()
  local wolf = trx.catalog.objects.WOLF
  local slot = trx.catalog.to_slot(trx.catalog.Context.OBJECTS, wolf)
  assert(slot == wolf + fake.SLOT_OFFSET, "the slot did not come back")
  assert(slot ~= wolf, "a slot is not the id")
end)

test("a slot converts back to the id it came from", function()
  local wolf = trx.catalog.objects.WOLF
  local slot = trx.catalog.to_slot(trx.catalog.Context.OBJECTS, wolf)
  assert(
    trx.catalog.from_slot(trx.catalog.Context.OBJECTS, slot) == wolf,
    "the round trip broke"
  )
end)

test("a catalog this game has nothing in reports nil, not zero", function()
  -- Not every game has every object, and a slot of 0 is a real slot.
  assert(trx.catalog.to_slot(trx.catalog.Context.MUSIC, 1) == nil)
  assert(trx.catalog.from_slot(trx.catalog.Context.MUSIC, 1) == nil)
end)

test(
  "an id too wide for the engine's does not wrap into the catalog",
  function()
    -- Narrowed to the engine's id, 2^32 + wolf is wolf, and the slot for a wolf
    -- would come back for an id that is not one.
    local wolf = trx.catalog.objects.WOLF
    assert(
      trx.catalog.to_slot(trx.catalog.Context.OBJECTS, 4294967296 + wolf)
        == nil
    )
    assert(
      trx.catalog.from_slot(
        trx.catalog.Context.OBJECTS,
        4294967296 + wolf + fake.SLOT_OFFSET
      ) == nil
    )
  end
)

test("the contexts are named", function()
  assert(trx.catalog.Context.OBJECTS ~= nil)
  assert(trx.catalog.Context.SAMPLES ~= nil)
  assert(
    trx.catalog.Context.objects == trx.catalog.Context.OBJECTS,
    "contexts answer to any case too"
  )
end)

-- A mod that ships only a script has no catalog file of its own, and must not
-- edit the one the game owns, so it declares what it brings while the game runs.
test("a script mints an identity of its own", function()
  local id = trx.catalog.mint(trx.catalog.Context.OBJECTS, "oil_drum")
  assert(id ~= nil, "minting gives back an id")
  assert(
    trx.catalog.objects["oil_drum"] == id,
    "and the catalog answers to the name from then on"
  )
  assert(
    trx.catalog.key(trx.catalog.Context.OBJECTS, id) == "oil_drum",
    "and gives the name back for the id"
  )
end)

test("a name the catalog already holds is not minted twice", function()
  local id = trx.catalog.mint(trx.catalog.Context.OBJECTS, "oil_drum_twice")
  assert(id ~= nil)
  assert(
    trx.catalog.mint(trx.catalog.Context.OBJECTS, "oil_drum_twice") == nil,
    "the second mint says no rather than taking the name again"
  )
  assert(
    trx.catalog.mint(trx.catalog.Context.OBJECTS, "wolf") == nil,
    "and a name the engine carries is held as well"
  )
end)

test("a name an identity may not take is refused", function()
  assert(trx.catalog.mint(trx.catalog.Context.OBJECTS, "") == nil)
  assert(trx.catalog.mint(trx.catalog.Context.OBJECTS, "has space") == nil)
  assert(trx.catalog.mint(trx.catalog.Context.OBJECTS, "OilDrum") == nil)
end)

return h.report()
