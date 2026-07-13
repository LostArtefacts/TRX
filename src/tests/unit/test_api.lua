-- Unit tests for data/scripting/api.lua, run under a plain Lua interpreter with
-- a stubbed C bridge. No engine, no binary, no level.

local ROOT = (arg[1] or ".") .. "/"

local failures = 0
local passed = 0

local function test(name, fn)
  local ok, err = pcall(fn)
  if ok then
    passed = passed + 1
    print("  PASS  " .. name)
  else
    failures = failures + 1
    print("  FAIL  " .. name)
    print("        " .. tostring(err))
  end
end

-- Records what api.type() asked the C binder to expose, so the tests can assert
-- on the declaration rather than on a real metatable.
local function fresh_env()
  local exposed = { fields = {}, methods = {}, computed = {} }
  _G.trxc = {
    struct = {
      expose_field = function(t, public, from, writable)
        exposed.fields[public] = { backing = t, from = from, writable = writable }
      end,
      expose_method = function(t, public, from)
        exposed.methods[public] = { backing = t, from = from }
      end,
      expose_computed = function(t, public, fn)
        exposed.computed[public] = fn
      end,
      members = function()
        return {
          { name = "visible", type = "INT32", writable = true },
          { name = "secret", type = "INT32", writable = true },
        }
      end,
    },
    -- Stands in for the ENUM_MAP reflection. Deliberately not in numeric order,
    -- and with a gap, so the tests pin what api.lua does with what C hands it.
    enum = {
      values = function(backing)
        assert(backing == "WIDGET_STATE", "unknown enum: " .. tostring(backing))
        return {
          { name = "BROKEN", value = 7 },
          { name = "OFF", value = 0 },
          { name = "ON", value = 1 },
        }
      end,
    },
  }
  _G.trx = { log = { debug = function() end, warn = function() end } }
  _G.require = function() end

  dofile(ROOT .. "data/scripting/api.lua")
  return trx.api, exposed
end

test("define exposes the raw impl, with no dispatch wrapper", function()
  local api = fresh_env()
  local impl = function(a)
    return a * 2
  end
  api.define("things.double", { params = { { name = "a", type = "integer" } }, impl = impl })

  -- The public function IS the implementation. Anything else means the spec has
  -- crept into the call path, which measured 26x slower.
  assert(trx.things.double == impl, "define wrapped the impl")
  assert(trx.things.double(21) == 42)
end)

test("strict mode rejects bad args and accepts good ones", function()
  local api = fresh_env()
  api.define("things.spawn", {
    params = {
      { name = "id", type = "integer" },
      { name = "pos", type = "vec3" },
      { name = "angle", type = "integer", optional = true, default = 0 },
    },
    impl = function(id, pos, angle)
      return angle
    end,
  })

  api.strict(true)
  assert(not pcall(trx.things.spawn, "not an integer", { x = 1, y = 1, z = 1 }))
  assert(not pcall(trx.things.spawn, 1, "not a vec3"))
  assert(pcall(trx.things.spawn, 1, { x = 1, y = 1, z = 1 }))

  -- An omitted optional argument takes its declared default.
  assert(trx.things.spawn(1, { x = 1, y = 1, z = 1 }) == 0)
  assert(trx.things.spawn(1, { x = 1, y = 1, z = 1 }, 7) == 7)

  api.strict(false)
  assert(pcall(trx.things.spawn, "anything goes now", nil))
end)

test("type() passes the declaration to the C binder", function()
  local api, exposed = fresh_env()
  api.type("things.Widget", {
    backing = "WIDGET",
    fields = {
      shown = { from = "visible", type = "integer", description = "..." },
      locked = { from = "visible", type = "integer", writable = false },
    },
    methods = { poke = { description = "..." } },
    extensions = {
      derived = {
        type = "integer",
        impl = function()
          return 1
        end,
      },
    },
  })

  -- The public name and the C member name are separate: that is the whole point.
  assert(exposed.fields.shown.from == "visible")
  assert(exposed.fields.shown.writable == true)
  assert(exposed.fields.locked.writable == false, "writable=false must be honoured")
  assert(exposed.methods.poke.from == "poke")
  assert(exposed.computed.derived ~= nil)

  -- `secret` is reachable by C and named by nobody, so it must not be exposed.
  assert(exposed.fields.secret == nil, "an undeclared member was exposed")
end)

test("describe() reports fields, methods and extensions", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.type("things.Widget", {
    backing = "WIDGET",
    description = "A widget.",
    fields = { shown = { from = "visible", type = "integer" } },
    methods = { poke = { description = "..." } },
    extensions = {
      derived = {
        type = "integer",
        impl = function() end,
      },
    },
  })
  api.define("things.count", { impl = function() end })

  local d = api.describe()
  assert(#d.types == 1, "type missing from describe()")
  local t = d.types[1]

  -- The bug that started all this: methods and extensions existed but were
  -- absent from the dump, so the generated docs silently omitted them.
  assert(#t.fields == 1, "fields missing")
  assert(#t.methods == 1, "methods missing from describe()")
  assert(#t.extensions == 1, "extensions missing from describe()")
  assert(#d.functions == 1, "functions missing")
end)

-- The whole point of api.enum: the numbers live in C and nowhere else, so a Lua
-- declaration that restated them could not go stale, because it cannot state
-- them at all.
test("enum() reflects the constants out of C", function()
  local api = fresh_env()
  api.enum("things.State", {
    backing = "WIDGET_STATE",
    values = { OFF = "off.", ON = "on.", BROKEN = "broken." },
  })

  assert(trx.things.State.OFF == 0)
  assert(trx.things.State.ON == 1)
  -- Not 2: the value is C's to decide, gaps and all.
  assert(trx.things.State.BROKEN == 7)
end)

test("enum() rejects a constant nobody documented", function()
  local api = fresh_env()
  -- BROKEN exists in C. Leaving it out here would quietly drop it from the docs.
  local ok, err = pcall(api.enum, "things.State", {
    backing = "WIDGET_STATE",
    values = { OFF = "off.", ON = "on." },
  })
  assert(not ok, "an undocumented constant was accepted")
  assert(tostring(err):find("BROKEN"), "the error should name the constant: " .. tostring(err))
end)

test("enum() rejects documentation for a constant that does not exist", function()
  local api = fresh_env()
  local ok, err = pcall(api.enum, "things.State", {
    backing = "WIDGET_STATE",
    values = { OFF = "off.", ON = "on.", BROKEN = "broken.", IMAGINARY = "not a real constant." },
  })
  assert(not ok, "documentation for a nonexistent constant was accepted")
  assert(tostring(err):find("IMAGINARY"), "the error should name the constant: " .. tostring(err))
end)

test("describe() reports enum values in numeric order", function()
  local api = fresh_env()
  api.module("things", {})
  api.enum("things.State", {
    backing = "WIDGET_STATE",
    description = "A state.",
    values = { OFF = "off.", ON = "on.", BROKEN = "broken." },
  })

  local d = api.describe()
  assert(#d.enums == 1, "enum missing from describe()")
  local e = d.enums[1]
  assert(e.path == "things.State")
  assert(e.description == "A state.")

  -- C hands them over in hash order; the docs must not come out shuffled.
  assert(#e.values == 3)
  assert(e.values[1].name == "OFF" and e.values[1].value == 0)
  assert(e.values[2].name == "ON" and e.values[2].value == 1)
  assert(e.values[3].name == "BROKEN" and e.values[3].value == 7)
  assert(e.values[3].description == "broken.", "prose missing from describe()")
end)

test("to_json round-trips the surface", function()
  local api = fresh_env()
  api.module("things", {})
  api.define("things.count", { impl = function() end, description = 'has "quotes" and \\ slashes' })
  local json = api.to_json()
  assert(json:find('"things.count"'), "path missing from json")
  assert(json:find('\\"quotes\\"'), "quotes not escaped")
end)

test("a module property reads through its getter and writes through its setter", function()
  local api = fresh_env()
  api.module("things", {})

  local air = 100
  api.property("things.air", {
    type = "integer",
    description = "Air.",
    get = function()
      return air
    end,
    set = function(value)
      air = value
    end,
  })
  api.property("things.pos", {
    type = "vec3",
    description = "Where.",
    get = function()
      return { x = 1, y = 2, z = 3 }
    end,
  })

  assert(trx.things.air == 100, "the getter did not run")
  trx.things.air = 50
  assert(air == 50, "the setter did not run")
  assert(trx.things.pos.x == 1, "computed property")

  -- No setter means read-only, and the message says so.
  local ok, err = pcall(function()
    trx.things.pos = { x = 0 }
  end)
  assert(not ok and tostring(err):find("read-only", 1, true), "a getter-only property must not be writable")

  -- The registry owns __newindex, so a name nobody declared cannot be created.
  -- This is the whole point: a metatable getter never shows up in pairs(), so
  -- seal()'s audit could never have caught one.
  ok = pcall(function()
    trx.things.undeclared = 1
  end)
  assert(not ok, "writing an undeclared property must raise")
  assert(trx.things.undeclared == nil, "reading an undeclared property must be nil")
end)

test("properties reach describe()", function()
  local api = fresh_env()
  api.module("things", {})
  api.property("things.air", {
    type = "integer",
    description = "Air.",
    get = function() end,
    set = function() end,
  })
  api.property("things.pos", { type = "vec3", description = "Where.", get = function() end })

  local out = api.describe()
  assert(#out.properties == 2, "properties missing from describe()")
  assert(out.properties[1].path == "things.air")
  assert(out.properties[1].writable == true, "a property with a setter is writable")
  assert(out.properties[2].writable == false, "a getter-only property is read-only")
end)

test("a namespace groups members one level down, and can itself be callable", function()
  local api = fresh_env()
  api.module("console", {})

  local logged = {}
  api.namespace("console.log", {
    description = "Logging.",
    call = function(message)
      return trx.console.log.info(message)
    end,
  })
  api.define("console.log.info", {
    description = "Info.",
    impl = function(message)
      logged[#logged + 1] = message
      return "ok"
    end,
  })

  assert(trx.console.log.info("a") == "ok", "the member is not reachable")
  assert(trx.console.log("b") == "ok", "the namespace is not callable")
  assert(logged[1] == "a" and logged[2] == "b", "the call did not reach info()")
end)

test("seal audits inside a namespace", function()
  local api = fresh_env()
  api.module("console", {})
  api.namespace("console.log", { description = "Logging." })
  api.define("console.log.info", { description = "Info.", impl = function() end })
  api.seal()

  -- A member one level down is exactly the surface the old audit could not see:
  -- it walked the module table and found `log`, a declared namespace, and stopped.
  local api2 = fresh_env()
  api2.module("console", {})
  api2.namespace("console.log", { description = "Logging." })
  rawset(trx.console.log, "sneaky", function() end)
  local ok, err = pcall(api2.seal)
  assert(not ok, "an undeclared member inside a namespace must fail the seal")
  assert(tostring(err):find("trx.console.log.sneaky", 1, true), "the audit must name it: " .. tostring(err))
end)

test("a path deeper than a namespace is rejected", function()
  local api = fresh_env()
  api.module("console", {})
  assert(not pcall(api.define, "console.log.deep.deeper", { impl = function() end }), "3 levels deep")
end)

test("seal blocks further declarations", function()
  local api = fresh_env()
  api.seal()

  -- api.type() reaches into the C struct binder. Leaving it callable would let a
  -- level script re-expose the members the declarations deliberately withheld.
  assert(not pcall(api.type, "things.Widget", { backing = "WIDGET" }), "type() after seal")
  assert(not pcall(api.define, "things.evil", { impl = function() end }), "define() after seal")
  assert(not pcall(api.enum, "things.State", { backing = "WIDGET_STATE", values = {} }), "enum() after seal")
  assert(not pcall(api.property, "things.air", { get = function() end }), "property() after seal")
end)

print(("\n%d passed, %d failed, %d total"):format(passed, failures, passed + failures))
os.exit(failures == 0 and 0 or 1)
