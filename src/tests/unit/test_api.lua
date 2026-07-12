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

test("to_json round-trips the surface", function()
  local api = fresh_env()
  api.module("things", {})
  api.define("things.count", { impl = function() end, description = 'has "quotes" and \\ slashes' })
  local json = api.to_json()
  assert(json:find('"things.count"'), "path missing from json")
  assert(json:find('\\"quotes\\"'), "quotes not escaped")
end)

test("seal blocks further declarations", function()
  local api = fresh_env()
  api.seal()

  -- api.type() reaches into the C struct binder. Leaving it callable would let a
  -- level script re-expose the members the declarations deliberately withheld.
  assert(not pcall(api.type, "things.Widget", { backing = "WIDGET" }), "type() after seal")
  assert(not pcall(api.define, "things.evil", { impl = function() end }), "define() after seal")
end)

print(("\n%d passed, %d failed, %d total"):format(passed, failures, passed + failures))
os.exit(failures == 0 and 0 or 1)
