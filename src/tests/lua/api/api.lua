-- Unit tests for src/lua/api/api.lua, run under a plain Lua interpreter with a
-- stubbed C bridge. No engine, no binary, no level.

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

-- The context the catalog fake answers for.
local FAKE_CONTEXT = 3

-- Records what api.type() asked the C binder to expose, so the tests can assert
-- on the declaration rather than on a real metatable.
local function fresh_env()
  local exposed = { fields = {}, methods = {}, computed = {} }
  _G.trxc = {
    struct = {
      expose_field = function(t, public, from, writable)
        exposed.fields[public] =
          { backing = t, from = from, writable = writable }
      end,
      expose_method = function(t, public, from)
        exposed.methods[public] = { backing = t, from = from }
      end,
      expose_computed = function(t, public, fn)
        exposed.computed[public] = fn
      end,
      -- What strict mode wraps: the C function behind a method.
      method = function(t, from)
        return function() end
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
        if backing == "COLLIDING_STATE" then
          -- Two constants that fold onto the same name.
          return {
            { name = "ON", value = 1 },
            { name = "on", value = 2 },
          }
        end
        assert(
          backing == "WIDGET_STATE",
          "unknown enum: " .. tostring(backing)
        )
        return {
          { name = "BROKEN", value = 7 },
          { name = "OFF", value = 0 },
          { name = "ON", value = 1 },
        }
      end,
    },
    -- Stands in for a catalog. It reports what it holds rather than what the
    -- exe was built with, so a name minted while the game runs is reachable
    -- without appearing in the constant list.
    catalog = {
      values = function(context)
        assert(
          context == FAKE_CONTEXT,
          "unknown context: " .. tostring(context)
        )
        return {
          { name = "off", value = 0 },
          { name = "on", value = 1 },
        }
      end,
      from_key = function(context, key)
        assert(
          context == FAKE_CONTEXT,
          "unknown context: " .. tostring(context)
        )
        return key == "oil_drum" and 9 or nil
      end,
    },
    -- Where the registry hands C the entrypoints it keeps after the seal.
    api = {
      set_entrypoint = function() end,
    },
  }
  -- What describe() runs a description through. The real one is declared as
  -- trx.strings.dedent and tested where it is written, so this stands in as
  -- the identity; the test that cares which keys reach it swaps in its own.
  _G.trx = {
    log = { debug = function() end, warn = function() end },
    strings = {
      dedent = function(text)
        return text
      end,
    },
  }
  -- The registry requires the checking layer and the logger. This runs the
  -- real checker, because what a declaration accepts is half of what is under
  -- test; the logger declares an enum out of C and is left stubbed.
  _G.require = function(name)
    if name == "trx.check" then
      return dofile(ROOT .. "src/lua/api/check.lua")
    end
  end

  dofile(ROOT .. "src/lua/api/api.lua")

  -- What to_json() writes the dump out with. It declares itself through the
  -- registry like anything else, so it loads after, and it is the real one:
  -- there is no C behind it, and how the dump comes out is under test here.
  dofile(ROOT .. "src/lua/api/json.lua")

  return trx.api, exposed
end

test("define exposes the raw impl, with no dispatch wrapper", function()
  local api = fresh_env()
  local impl = function(a)
    return a * 2
  end
  api.define(
    "things.double",
    { params = { { name = "a", type = "integer" } }, impl = impl }
  )

  -- Anything else means the spec has crept into the call path.
  assert(trx.things.double == impl, "define wrapped the impl")
  assert(trx.things.double(21) == 42)
end)

test("strict mode rejects bad args and accepts good ones", function()
  local api = fresh_env()
  api.type("things.Pos", {
    record = true,
    description = "Where.",
    fields = {
      x = { type = "integer", description = "." },
      y = { type = "integer", description = "." },
      z = { type = "integer", description = "." },
    },
  })
  api.define("things.spawn", {
    params = {
      { name = "id", type = "integer" },
      { name = "pos", type = "things.Pos" },
      { name = "angle", type = "integer", optional = true, default = 0 },
    },
    impl = function(id, pos, angle)
      return angle
    end,
  })

  api.strict(true)
  assert(
    not pcall(trx.things.spawn, "not an integer", { x = 1, y = 1, z = 1 })
  )
  assert(not pcall(trx.things.spawn, 1, "not a position"))
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

  -- The public name and the C member name are separate: that is the whole
  -- point.
  assert(exposed.fields.shown.from == "visible")
  assert(exposed.fields.shown.writable == true)
  assert(
    exposed.fields.locked.writable == false,
    "writable=false must be honoured"
  )
  assert(exposed.methods.poke.from == "poke")
  assert(exposed.computed.derived ~= nil)

  -- `secret` is reachable by C and named by nobody, so it must not be exposed.
  assert(exposed.fields.secret == nil, "an undeclared member was exposed")
end)

-- Which of the keys describe() collects are prose. The dedenting itself is
-- trx.strings.dedent's, and is tested where that is written.
test(
  "describe() takes the indentation off prose and leaves code alone",
  function()
    local api = fresh_env()
    trx.strings.dedent = function(text)
      return "dedented: " .. text
    end
    api.module("things", { description = "A module." })
    api.define("things.poke", {
      description = "Pokes it.",
      params = { { name = "how", type = "string", description = "How hard." } },
      examples = { "trx.things.poke('gently')" },
      impl = function() end,
    })

    local dumped = api.describe()
    local module, poke
    for _, entry in ipairs(dumped.modules) do
      if entry.name == "things" then
        module = entry
      end
    end
    for _, entry in ipairs(dumped.functions) do
      if entry.path == "things.poke" then
        poke = entry
      end
    end

    assert(module.description == "dedented: A module.")
    assert(poke.description == "dedented: Pokes it.")
    assert(
      poke.params[1].description == "dedented: How hard.",
      "a parameter carries prose too"
    )
    assert(
      poke.examples[1] == "trx.things.poke('gently')",
      "an example is code, and its own indentation is the point"
    )
  end
)

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

  -- The docs are generated from the dump, so methods and extensions have to
  -- reach it.
  assert(#t.fields == 1, "fields missing")
  assert(#t.methods == 1, "methods missing from describe()")
  assert(#t.extensions == 1, "extensions missing from describe()")

  -- Named rather than counted: the registry declares its own functions too.
  local declared = {}
  for _, fn in ipairs(d.functions) do
    declared[fn.path] = true
  end
  assert(declared["things.count"], "functions missing")
end)

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
  -- BROKEN exists in C. Leaving it out here would quietly drop it from the
  -- docs.
  local ok, err = pcall(api.enum, "things.State", {
    backing = "WIDGET_STATE",
    values = { OFF = "off.", ON = "on." },
  })
  assert(not ok, "an undocumented constant was accepted")
  assert(
    tostring(err):find("BROKEN"),
    "the error should name the constant: " .. tostring(err)
  )
end)

test(
  "enum() rejects documentation for a constant that does not exist",
  function()
    local api = fresh_env()
    local ok, err = pcall(api.enum, "things.State", {
      backing = "WIDGET_STATE",
      values = {
        OFF = "off.",
        ON = "on.",
        BROKEN = "broken.",
        IMAGINARY = "not a real constant.",
      },
    })
    assert(not ok, "documentation for a nonexistent constant was accepted")
    assert(
      tostring(err):find("IMAGINARY"),
      "the error should name the constant: " .. tostring(err)
    )
  end
)

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

  -- C hands them over in hash order.
  assert(#e.values == 3)
  assert(e.values[1].name == "OFF" and e.values[1].value == 0)
  assert(e.values[2].name == "ON" and e.values[2].value == 1)
  assert(e.values[3].name == "BROKEN" and e.values[3].value == 7)
  assert(e.values[3].description == "broken.", "prose missing from describe()")
end)

test("to_json round-trips the surface", function()
  local api = fresh_env()
  api.module("things", {})
  api.define(
    "things.count",
    { impl = function() end, description = 'has "quotes" and \\ slashes' }
  )
  local json = api.to_json()
  assert(json:find('"things.count"'), "path missing from json")
  assert(json:find('\\"quotes\\"'), "quotes not escaped")
end)

test(
  "a module property reads through its getter and writes through its setter",
  function()
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
      type = "table",
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
    assert(
      not ok and tostring(err):find("read-only", 1, true),
      "a getter-only property must not be writable"
    )

    -- The registry owns __newindex, so an undeclared name cannot be created.
    ok = pcall(function()
      trx.things.undeclared = 1
    end)
    assert(not ok, "writing an undeclared property must raise")
    assert(
      trx.things.undeclared == nil,
      "reading an undeclared property must be nil"
    )
  end
)

test(
  "anything declared can be deprecated, and says so in describe()",
  function()
    local api = fresh_env()
    api.module("things", {})
    api.define("things.old", {
      deprecated = "Use `trx.things.new` instead.",
      description = "Old.",
      impl = function() end,
    })
    api.property("things.air", {
      deprecated = true,
      type = "integer",
      description = "Air.",
      get = function() end,
    })
    api.type("things.Widget", {
      backing = "WIDGET",
      description = "A widget.",
      fields = {
        shown = {
          from = "shown",
          type = "integer",
          deprecated = "Read `trx.things.air` instead.",
          description = "Shown.",
        },
      },
    })

    -- The surface carries the registry's own declarations as well, so each entry
    -- is looked up by the path it was declared at.
    local function at(list, path)
      for _, entry in ipairs(list) do
        if (entry.path or entry.name) == path then
          return entry
        end
      end
    end

    local out = api.describe()
    assert(
      at(out.functions, "things.old").deprecated
        == "Use `trx.things.new` instead."
    )
    assert(at(out.properties, "things.air").deprecated == true)
    assert(
      at(out.types, "things.Widget").fields[1].deprecated
        == "Read `trx.things.air` instead.",
      "a member of a type carries it too"
    )
    assert(
      at(out.modules, "things").deprecated == nil,
      "nothing else is marked"
    )

    -- A deprecated declaration goes on working: it is the docs that change, not
    -- what a script can call.
    assert(type(trx.things.old) == "function")
  end
)

test("deprecated has to say what it is", function()
  local api = fresh_env()
  api.module("things", {})
  local ok = pcall(api.define, "things.old", {
    deprecated = 1,
    description = "Old.",
    impl = function() end,
  })
  assert(
    not ok,
    "a deprecation that is neither true nor words must be refused"
  )
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
  api.property(
    "things.pos",
    { type = "table", description = "Where.", get = function() end }
  )

  local out = api.describe()
  assert(#out.properties == 2, "properties missing from describe()")
  assert(out.properties[1].path == "things.air")
  assert(
    out.properties[1].writable == true,
    "a property with a setter is writable"
  )
  assert(
    out.properties[2].writable == false,
    "a getter-only property is read-only"
  )
end)

test(
  "a namespace groups members one level down, and can itself be callable",
  function()
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
    assert(
      logged[1] == "a" and logged[2] == "b",
      "the call did not reach info()"
    )
  end
)

test("seal audits inside a namespace", function()
  local api = fresh_env()
  api.module("console", {})
  api.namespace("console.log", { description = "Logging." })
  api.define(
    "console.log.info",
    { description = "Info.", impl = function() end }
  )
  api.seal()

  local api2 = fresh_env()
  api2.module("console", {})
  api2.namespace("console.log", { description = "Logging." })
  rawset(trx.console.log, "sneaky", function() end)
  local ok, err = pcall(api2.seal)
  assert(not ok, "an undeclared member inside a namespace must fail the seal")
  assert(
    tostring(err):find("trx.console.log.sneaky", 1, true),
    "the audit must name it: " .. tostring(err)
  )
end)

test("a path deeper than a namespace is rejected", function()
  local api = fresh_env()
  api.module("console", {})
  assert(
    not pcall(api.define, "console.log.deep.deeper", { impl = function() end }),
    "3 levels deep"
  )
end)

test("two constants that fold onto one name fail", function()
  local api = fresh_env()
  api.module("things", {})

  -- `on` reads as ON, and ON is already a constant.
  local ok, err = pcall(api.enum, "things.State", {
    backing = "COLLIDING_STATE",
    values = { ON = "on." },
  })
  assert(not ok, "a collision must not be accepted")
  assert(
    tostring(err):find("two constants", 1, true),
    "the error must say why: " .. tostring(err)
  )
end)

test(
  "an enum answers to a name in any case, and cannot be written to",
  function()
    local api = fresh_env()
    api.module("things", {})

    local e = api.enum("things.State", {
      backing = "WIDGET_STATE",
      values = { OFF = "off.", ON = "on.", BROKEN = "broken." },
    })

    assert(e.ON == 1)
    assert(e.on == 1, "an enum must answer to a lower case name")
    assert(e.On == 1, "any case")
    assert(e.invalid == nil, "a name the enum does not have is still nil")

    local names = {}
    for name in pairs(e) do
      names[#names + 1] = name
    end
    table.sort(names)
    assert(
      table.concat(names, ",") == "BROKEN,OFF,ON",
      "pairs() must yield the canonical spelling only"
    )

    local ok = pcall(function()
      e.ON = 7
    end)
    assert(not ok, "an enum must not be writable")
    assert(e.ON == 1, "and the write must not have landed")
  end
)

-- A catalog reports what it holds, so its constants are whatever it names when
-- the enum is declared, and a name minted after that still resolves.
test("a catalog enum reads its constants from the catalog", function()
  local api = fresh_env()
  api.module("things", {})

  local e = api.enum("things.Kind", {
    backing = "WIDGET_STATE",
    context = FAKE_CONTEXT,
    bulk = true,
  })

  assert(e.OFF == 0, "a catalog name reads as a constant")
  assert(e.ON == 1)
  assert(e.on == 1, "in any case")

  local names = {}
  for name in pairs(e) do
    names[#names + 1] = name
  end
  table.sort(names)
  assert(
    table.concat(names, ",") == "OFF,ON",
    "and the constant list is what the catalog held"
  )

  assert(
    e["oil_drum"] == 9,
    "a name minted after the enum was declared must resolve"
  )
  assert(e["OIL_DRUM"] == 9, "in any case")
  assert(e.WOMBAT == nil, "and a name nothing holds is still nil")
end)

test(
  "a bulk enum is described as a whole, not a constant at a time",
  function()
    local api = fresh_env()
    api.module("catalog", {})

    local e = api.enum("catalog.objects", {
      backing = "WIDGET_STATE",
      bulk = true,
      description = "Every object id, by name.",
    })
    assert(e.ON == 1, "the constants are still there")
    assert(e.on == 1)

    local entry = api.describe().enums[1]
    assert(entry.bulk == true)
    assert(entry.count == 3, "a bulk enum reports how many constants it has")
    assert(#entry.values == 0, "and not what they are")
  end
)

-- A type written in Lua: the declaration hands back the class a value carries
-- as its metatable, and every method a script can call is one declared here.
local function widget_class(api, spec)
  return api.type("things.Widget", spec)
end

test("type() written in Lua binds its methods to the class", function()
  local api, exposed = fresh_env()
  local Widget = widget_class(api, {
    description = "A widget.",
    methods = {
      poke = {
        description = "...",
        impl = function(self)
          return self.name .. " poked"
        end,
      },
    },
  })

  local widget = setmetatable({ name = "hinge" }, Widget)
  assert(widget:poke() == "hinge poked")
  assert(
    exposed.methods.poke == nil,
    "a type with no backing must ask C for nothing"
  )
end)

test("a Lua type's methods must carry an impl", function()
  local api = fresh_env()
  assert(not pcall(widget_class, api, { methods = { poke = {} } }))
end)

test("a derived Lua type inherits methods and operators", function()
  local api = fresh_env()
  local Widget = api.type("things.Widget", {
    operators = {
      band = {
        description = "...",
        impl = function(a, b)
          return a.name .. "+" .. b.name
        end,
      },
    },
    methods = {
      poke = {
        description = "...",
        impl = function(self)
          return self.name
        end,
      },
    },
  })
  local Lever = api.type("things.Lever", {
    extends = "things.Widget",
    methods = {
      pull = {
        description = "...",
        impl = function(self)
          return self.name .. " pulled"
        end,
      },
    },
  })

  local lever = setmetatable({ name = "lever" }, Lever)
  assert(lever:pull() == "lever pulled")
  assert(lever:poke() == "lever", "an inherited method is unreachable")
  -- Lua looks a metamethod up raw, so the derived class needs its own copy.
  assert(
    (lever & setmetatable({ name = "other" }, Lever)) == "lever+other",
    "an inherited operator is unreachable"
  )
  assert(getmetatable(Widget) == nil, "the base class gained a metatable")
end)

test("a derived Lua type inherits its parent's fields", function()
  local api = fresh_env()
  api.type("things.Widget", {
    fields = {
      name = {
        type = "string",
        description = "...",
        get = function(self)
          return rawget(self, "held")
        end,
        set = function(self, value)
          rawset(self, "held", value)
        end,
      },
    },
  })
  local Lever = api.type("things.Lever", {
    extends = "things.Widget",
    methods = {
      pull = {
        description = "...",
        impl = function(self)
          return self.name .. " pulled"
        end,
      },
    },
  })

  -- The accessors sit in the parent's __index closure, so the metatable chain
  -- does not reach one and the derived type has to take them over.
  local lever = setmetatable({ held = "lever" }, Lever)
  assert(lever.name == "lever", "an inherited field is unreadable")
  lever.name = "pulled lever"
  assert(lever.name == "pulled lever", "an inherited field is unwritable")
  assert(lever:pull() == "pulled lever pulled")
end)

test("extending a type nobody declared raises", function()
  local api = fresh_env()
  assert(not pcall(api.type, "things.Lever", { extends = "things.Widget" }))
end)

test("a Lua type checks its own values, derived ones included", function()
  local api = fresh_env()
  local Widget = api.type("things.Widget", {
    methods = {
      poke = {
        description = "...",
        impl = function()
          return true
        end,
      },
    },
  })
  local Lever = api.type("things.Lever", { extends = "things.Widget" })
  api.define("things.press", {
    params = { { name = "widget", type = "things.Widget" } },
    impl = function()
      return true
    end,
  })

  api.strict(true)
  assert(pcall(trx.things.press, setmetatable({}, Widget)))
  assert(
    pcall(trx.things.press, setmetatable({}, Lever)),
    "a derived value must satisfy the type it extends"
  )
  assert(not pcall(trx.things.press, {}), "a plain table passed as a Widget")
  assert(not pcall(trx.things.press, 7))

  -- Strict mode rebinds a Lua type's methods the same way it rebinds a
  -- module's functions, and the handle it checks first is the value itself.
  assert(pcall(function()
    return setmetatable({}, Widget):poke()
  end))
  assert(not pcall(Widget.poke, {}), "self went unchecked")
  api.strict(false)
end)

test("describe() marks which types are handles", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.type("things.Widget", { backing = "WIDGET", description = "..." })
  api.type("things.Lever", {
    description = "...",
    operators = {
      bnot = {
        description = "Everything it does not match.",
        impl = function() end,
      },
    },
  })

  local by_path = {}
  for _, entry in ipairs(api.describe().types) do
    by_path[entry.path] = entry
  end
  assert(by_path["things.Widget"].handle == true)
  assert(
    by_path["things.Lever"].handle == false,
    "a type written in Lua is not a handle"
  )
  assert(#by_path["things.Lever"].operators == 1, "operators missing")
  assert(by_path["things.Lever"].operators[1].name == "bnot")
end)

test("seal blocks further declarations", function()
  local api = fresh_env()
  api.seal()

  assert(
    not pcall(api.type, "things.Widget", { backing = "WIDGET" }),
    "type() after seal"
  )
  assert(
    not pcall(api.define, "things.evil", { impl = function() end }),
    "define() after seal"
  )
  assert(
    not pcall(
      api.enum,
      "things.State",
      { backing = "WIDGET_STATE", values = {} }
    ),
    "enum() after seal"
  )
  assert(
    not pcall(api.property, "things.air", { get = function() end }),
    "property() after seal"
  )
end)

-- Several bridges read lua_gettop() to tell an omitted argument from a nil one,
-- so the wrapper must not pass one the caller did not.
test(
  "strict mode drops an optional argument that carries no default",
  function()
    local api = fresh_env()
    api.define("things.flip", {
      params = {
        { name = "id", type = "integer" },
        { name = "timer", type = "integer", optional = true },
      },
      impl = function(...)
        return select("#", ...)
      end,
    })

    api.strict(true)
    assert(
      trx.things.flip(1) == 1,
      "an omitted optional must not arrive as nil"
    )
    assert(trx.things.flip(1, 5) == 2, "one that was passed must arrive")
  end
)

test("strict mode substitutes a default and still checks it", function()
  local api = fresh_env()
  api.define("things.track", {
    params = {
      { name = "track", type = "integer", optional = true, default = 3 },
    },
    impl = function(track)
      return track
    end,
  })

  api.strict(true)
  assert(trx.things.track() == 3, "the default stands in for the argument")
  assert(not pcall(trx.things.track, "nope"))
end)

test("strict mode checks a namespace's call and a property's write", function()
  local api = fresh_env()
  local logged, air
  api.module("things", {})
  api.namespace("things.log", {
    description = "Logging.",
    params = { { name = "message", type = "string" } },
    call = function(message)
      logged = message
    end,
  })
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

  api.strict(true)
  trx.things.log("hello")
  assert(logged == "hello")
  assert(not pcall(trx.things.log, 42), "calling the group must be checked")

  trx.things.air = 5
  assert(air == 5)
  assert(not pcall(function()
    trx.things.air = "not an integer"
  end), "writing a property must be checked")
end)

test("seal refuses a parameter nothing can check", function()
  local api = fresh_env()
  api.define("things.bad", {
    params = { { name = "a", type = "intger" } },
    impl = function() end,
  })

  local ok, err = pcall(api.seal)
  assert(not ok, "a misspelled type must fail the seal")
  assert(
    tostring(err):find("intger", 1, true),
    "the audit must name it: " .. tostring(err)
  )
end)

test("seal refuses a default of the wrong type", function()
  local api = fresh_env()
  api.define("things.bad", {
    params = {
      {
        name = "a",
        type = "integer",
        optional = true,
        default = "trx.things.A",
      },
    },
    impl = function() end,
  })

  assert(
    not pcall(api.seal),
    "a default that is not of the parameter's type must fail the seal"
  )
end)

test("seal refuses a required parameter behind an optional one", function()
  local api = fresh_env()
  api.define("things.bad", {
    params = {
      { name = "a", type = "integer", optional = true },
      { name = "b", type = "integer" },
    },
    impl = function() end,
  })

  assert(not pcall(api.seal), "a call cannot reach b without passing a")
end)

test(
  "a container is declared, not hand-rolled onto the module table",
  function()
    local api = fresh_env()
    local things = { "first", "second" }
    api.module("things", {})
    api.container("things", {
      description = "Indexing.",
      key = { type = "integer", description = "1-based." },
      value = { type = "string" },
      get = function(key)
        return things[key]
      end,
      count = function()
        return #things
      end,
    })

    assert(trx.things[1] == "first")
    assert(#trx.things == 2)

    -- A property declared afterwards must not take the metatable with it.
    api.property("things.best", {
      type = "string",
      description = "Best.",
      get = function()
        return things[1]
      end,
    })
    assert(
      trx.things[2] == "second",
      "declaring a property must not drop the indexing"
    )
    assert(trx.things.best == "first")
  end
)

test("a Lua type's fields read and write through its accessors", function()
  local api = fresh_env()
  local state = {}
  local Widget = api.type("things.Widget", {
    fields = {
      name = {
        type = "string",
        description = "...",
        get = function(self)
          return state[self].name
        end,
        set = function(self, value)
          state[self].name = value
        end,
      },
      size = {
        type = "integer",
        description = "...",
        get = function(self)
          return #state[self].name
        end,
      },
    },
    methods = {
      poke = {
        description = "...",
        impl = function(self)
          return self.name .. "!"
        end,
      },
    },
  })

  local widget = setmetatable({}, Widget)
  state[widget] = { name = "hinge" }

  assert(widget.name == "hinge")
  assert(widget.size == 5, "a field with no set is still readable")
  assert(widget:poke() == "hinge!", "a method must still be reachable")

  widget.name = "lever"
  assert(widget.name == "lever")

  -- Declared or absent: what the type does not name is neither readable nor
  -- writable, so a value cannot grow a member the docs never see.
  assert(widget.colour == nil)
  local ok, err = pcall(function()
    widget.size = 3
  end)
  assert(not ok and tostring(err):find("read%-only"), tostring(err))
  assert(not pcall(function()
    widget.colour = "red"
  end))

  local entry = api.describe().types[1]
  local by_name = {}
  for _, field in ipairs(entry.fields) do
    by_name[field.name] = field
  end
  assert(by_name.name.writable == true)
  assert(by_name.size.writable == false, "a field with no set is read-only")
end)

test("a Lua type's field with no accessors is an entry it carries", function()
  local api = fresh_env()
  local Box = api.type("things.Box", {
    description = "...",
    fields = {
      min_x = { type = "integer", description = "..." },
      max_x = { type = "integer", description = "..." },
    },
    methods = {
      width = {
        description = "...",
        impl = function(self)
          return self.max_x - self.min_x
        end,
      },
    },
  })

  local box = setmetatable({ min_x = 1, max_x = 5 }, Box)
  assert(box.min_x == 1)
  assert(box:width() == 4, "a method must still be reachable")

  local entry = api.describe().types[1]
  for _, field in ipairs(entry.fields) do
    assert(
      field.writable == nil,
      "a plain table the caller owns has no writability to report"
    )
  end
end)

test("a Lua type's field with a set needs a get as well", function()
  local api = fresh_env()
  assert(not pcall(api.type, "things.Widget", {
    fields = { name = { type = "string", set = function() end } },
  }))
end)

test("a type is named by the path it was declared under", function()
  local api = fresh_env()
  local Widget = api.type("things.Widget", {})
  api.define("things.press", {
    params = { { name = "widget", type = "things.Widget" } },
    impl = function()
      return true
    end,
  })

  api.strict(true)
  assert(pcall(trx.things.press, setmetatable({}, Widget)))
  assert(not pcall(trx.things.press, {}))
  api.strict(false)
end)

-- The path is the only name a type has. A bare one used to answer as well,
-- which meant a declaration could name a type the declaration never chose and
-- go on meaning it until another module claimed the name.
test("a bare type name names nothing", function()
  local api = fresh_env()
  api.type("things.Widget", {})
  api.define("things.press", {
    params = { { name = "thing", type = "Widget" } },
    impl = function() end,
  })
  assert(
    not pcall(api.seal),
    "a type nothing declares must be reported, not waved through"
  )
end)

-- Two modules may declare a type of the same name - music.Stream and
-- sound.Stream are one - and each is named by its own path.
test("two modules may name a type the same", function()
  local api = fresh_env()
  local Widget = api.type("things.Widget", {})
  local Gadget = api.type("others.Widget", {})
  api.define("things.press", {
    params = { { name = "thing", type = "things.Widget" } },
    impl = function()
      return true
    end,
  })

  api.strict(true)
  assert(pcall(trx.things.press, setmetatable({}, Widget)))
  assert(
    not pcall(trx.things.press, setmetatable({}, Gadget)),
    "the path must name one type and not the other"
  )
  api.strict(false)
end)

-- The registry declares functions of its own, so a test finds its own by path.
local function declared(api, path)
  for _, fn in ipairs(api.describe().functions) do
    if fn.path == path then
      return fn
    end
  end
  return nil
end

test("a number is written once and named from wherever it is meant", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.number("things.Num", {
    base = 0,
    description = "A thing number.",
  })
  api.define("things.get", {
    params = {
      { name = "num", type = "things.Num" },
      {
        name = "callback",
        type = "function",
        params = { { name = "num", type = "things.Num" } },
      },
    },
    returns = { type = "things.Num" },
    impl = function() end,
  })

  local dumped = api.describe()
  assert(
    #dumped.numbers == 1,
    "the number must reach the docs to be linked to"
  )
  assert(dumped.numbers[1].path == "things.Num")
  assert(dumped.numbers[1].base == 0)

  -- What a declaration holds is written out as the path it names, so what the
  -- number says is read from one place.
  local fn = declared(api, "things.get")
  assert(fn.params[1].type == "things.Num")
  assert(fn.params[1].description == nil, "the description is not copied")
  assert(
    fn.params[2].params[1].type == "things.Num",
    "a callback's own arguments name it too"
  )
  assert(fn.returns.type == "things.Num")
end)

test("a declaration keeps what it adds of its own", function()
  local api = fresh_env()
  api.number("things.Num", { base = 0, description = "A number." })
  api.define("things.get", {
    params = {
      {
        name = "num",
        type = "things.Num",
        description = "The one that broke.",
      },
    },
    impl = function() end,
  })

  local param = declared(api, "things.get").params[1]
  assert(param.type == "things.Num")
  assert(param.description == "The one that broke.")
end)

test("an enum is a type like any other", function()
  local api = fresh_env()
  api.enum("things.State", {
    backing = "WIDGET_STATE",
    description = "A state.",
    values = { OFF = "off.", ON = "on.", BROKEN = "broken." },
  })
  api.define("things.get", {
    params = { { name = "state", type = "things.State" } },
    impl = function() end,
  })
  assert(declared(api, "things.get").params[1].type == "things.State")
end)

-- A unit is what a value is measured in, and it answers the same question a
-- number does: what this is, said once, wherever one turns up.
test("a unit is written once and named from wherever it is meant", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.unit("things.Angle", {
    description = "An angle in the engine's own units.",
    spellings = { "engine's own units" },
  })
  api.unit("things.Seconds", { type = "number", description = "Seconds." })
  api.define("things.turn", {
    params = { { name = "angle", type = "things.Angle" } },
    returns = { type = "things.Seconds" },
    impl = function() end,
  })

  local dumped = api.describe()
  assert(#dumped.units == 2, "the units must reach the docs to be linked to")
  assert(dumped.units[1].path == "things.Angle")
  assert(dumped.units[1].type == "integer", "a unit is whole unless it says")
  assert(dumped.units[1].spellings[1] == "engine's own units")
  assert(dumped.units[2].type == "number")

  local fn = declared(api, "things.turn")
  assert(fn.params[1].type == "things.Angle")
  assert(fn.returns.type == "things.Seconds")
end)

test("a unit measures a whole or a real number, and nothing else", function()
  local api = fresh_env()
  assert(
    not pcall(
      api.unit,
      "things.Angle",
      { type = "string", description = "Not a measurement." }
    )
  )
end)

test("a unit cannot be written twice", function()
  local api = fresh_env()
  api.unit("things.Angle", { description = "An angle." })
  assert(
    not pcall(api.unit, "things.Angle", { description = "Something else." })
  )
end)

-- Strict mode checks a unit as it checks anything else: what a value of it is
-- is what the unit says it is.
test("a unit is checked by what it measures", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.unit("things.Seconds", { type = "number", description = "Seconds." })
  api.unit("things.Angle", { description = "An angle." })
  api.define("things.wait", {
    params = {
      { name = "time", type = "things.Seconds" },
      { name = "angle", type = "things.Angle" },
    },
    impl = function() end,
  })

  api.strict(true)
  assert(pcall(trx.things.wait, 1.5, 90), "a real number and a whole one")
  assert(not pcall(trx.things.wait, 1.5, 1.5), "an angle is whole")
  api.strict(false)
end)

-- Partial is for a suite that stands up a few modules, where a name the rest
-- of the surface declares is expected to be missing. It used to reach for the
-- checker it had just found nothing for.
test("a partial seal tolerates a default it cannot check", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.define("things.get", {
    description = "...",
    params = {
      {
        name = "num",
        type = "elsewhere.Num",
        optional = true,
        default = 0,
        description = "...",
      },
    },
    impl = function() end,
  })

  assert(
    pcall(api.seal, { partial = true }),
    "a partial seal must tolerate it"
  )
end)

-- describe() is read twice by the tests and once by the dump, and a projection
-- that handed out its own registry entry would rewrite it the first time.
test("describe() gives the same answer twice", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.namespace("things.log", {
    description = [[
      Indented, so the dedent has something to take off.
    ]],
    params = { { name = "message", type = "any", description = "..." } },
    call = function() end,
  })

  local first = api.to_json()
  assert(api.to_json() == first, "describe() rewrote what it read")
end)

-- A table a script writes out is checked by what it holds. A key it does not
-- name reads as nothing at all otherwise, whether checking is on or off.
test("strict mode checks the keys a table declares", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.type("things.Options", {
    record = true,
    description = "How to do it.",
    fields = {
      mode = { type = "integer", optional = true, description = "." },
    },
  })
  api.define("things.play", {
    description = "...",
    params = {
      { name = "opts", type = "things.Options", optional = true },
      {
        name = "inline",
        type = "table",
        optional = true,
        description = ".",
        fields = { { name = "pos", type = "integer", description = "." } },
      },
    },
    impl = function() end,
  })

  api.strict(true)
  assert(pcall(trx.things.play, { mode = 1 }), "a plain table a script wrote")
  assert(not pcall(trx.things.play, { moed = 1 }), "a key nothing names")
  assert(not pcall(trx.things.play, { mode = "x" }), "a key of the wrong type")
  assert(pcall(trx.things.play, nil, { pos = 1 }), "keys declared inline")
  assert(not pcall(trx.things.play, nil, {}), "one of them missing")
  api.strict(false)
end)

-- A declaration that holds several of something is checked as the list it is.
-- Checking the element type against the list itself rejects every valid call.
test("strict mode checks a list by what it holds", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.define("things.collapse", {
    description = "...",
    params = { { name = "numbers", type = "integer", list = true } },
    impl = function() end,
  })
  api.define("things.match", {
    description = "...",
    params = {
      {
        name = "sources",
        type = "table",
        list = true,
        description = ".",
        fields = { { name = "key", type = "string", description = "." } },
      },
    },
    impl = function() end,
  })

  api.strict(true)
  assert(pcall(trx.things.collapse, { 1, 2, 3 }), "a list of what it holds")
  assert(pcall(trx.things.collapse, {}), "an empty one")
  assert(not pcall(trx.things.collapse, 1), "the element on its own")
  assert(
    not pcall(trx.things.collapse, { 1, "x" }),
    "an entry of the wrong type"
  )
  assert(pcall(trx.things.match, { { key = "a" } }), "keys declared inline")
  assert(not pcall(trx.things.match, { { kye = "a" } }), "a key nothing names")
  api.strict(false)
end)

test("a number cannot be written twice", function()
  local api = fresh_env()
  api.number("things.Num", { base = 0, description = "A thing number." })
  assert(
    not pcall(api.number, "things.Num", { description = "Something else." })
  )
end)

-- A collection declares what its key is, and the reference says so, so strict
-- mode holds a script to it rather than handing C a number nothing answers to.
test("strict mode checks what a collection is indexed with", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.number("things.Num", { base = 0, description = "Where it sits." })
  api.container("things", {
    description = "Indexing.",
    key = { type = "things.Num" },
    value = { type = "string", nullable = true },
    get = function(i)
      return "held " .. i
    end,
    count = function()
      return 2
    end,
  })

  assert(trx.things[1.5] == "held 1.5", "unchecked while checking is off")

  api.strict(true)
  assert(trx.things[1] == "held 1", "a key of the declared type")
  local ok, err = pcall(function()
    return trx.things[1.5]
  end)
  assert(not ok, "a fractional key must be refused")
  assert(
    tostring(err):find("things[1.5]", 1, true),
    "the message must name what was written: " .. tostring(err)
  )
  -- A key of a kind the collection never accepts is left to the rest of the
  -- metatable, so it is nil rather than an error.
  assert(trx.things.nonsense == nil, "a string key is not indexing")
  api.strict(false)
end)

test("a container walks from the index it counts from", function()
  local api = fresh_env()
  local zero, one = { [0] = "a", [1] = "b" }, { "a", "b" }

  api.module("counted", {})
  api.number("counted.Num", { base = 0, description = "Where it sits." })
  api.container("counted", {
    description = "Indexing.",
    key = { type = "counted.Num" },
    value = { type = "string" },
    get = function(key)
      return zero[key]
    end,
    count = function()
      return 2
    end,
  })

  api.module("listed", {})
  api.number("listed.Num", { base = 1, description = "Where it sits." })
  api.container("listed", {
    description = "Indexing.",
    key = { type = "listed.Num" },
    value = { type = "string" },
    get = function(key)
      return one[key]
    end,
    count = function()
      return #one
    end,
  })

  local function walked(module)
    local keys = {}
    for key in pairs(module) do
      keys[#keys + 1] = key
    end
    return table.concat(keys, ",")
  end

  assert(walked(trx.counted) == "0,1", walked(trx.counted))
  assert(walked(trx.listed) == "1,2", walked(trx.listed))
end)

-- A sparse collection runs its keys further than it has entries: the samples a
-- level carries are a hundred spread over twice as many numbers.
test("a sparse container walks its keys and skips the gaps", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.number("things.Num", { base = 0, description = "Where it sits." })
  local held = { [0] = "a", [3] = "b", [4] = "c" }
  api.container("things", {
    description = "Indexing.",
    key = { type = "things.Num" },
    value = { type = "string", nullable = true },
    get = function(i)
      return held[i]
    end,
    count = function()
      return 3
    end,
    limit = function()
      return 5
    end,
  })

  assert(#trx.things == 3, "# is how many there are")
  local seen = {}
  for i, value in pairs(trx.things) do
    seen[#seen + 1] = i .. "=" .. value
  end
  assert(
    table.concat(seen, ",") == "0=a,3=b,4=c",
    "the gaps must not come out as nil: " .. table.concat(seen, ",")
  )
end)

-- A module may hand out a collection under a name of its own as well as being
-- indexed itself, and the table that collection sits on is the registry's in
-- the same way a module's is.
test("a collection a module hands out is indexed on its own table", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  local held = { [1] = "a", [2] = "b" }
  local samples = api.container("things.samples", {
    description = "Indexing.",
    key = { type = "integer", description = "A key." },
    value = { type = "string" },
    get = function(i)
      return held[i]
    end,
    count = function()
      return 2
    end,
  })

  assert(samples == trx.things.samples, "the table it is indexed on")
  assert(trx.things.samples[2] == "b")
  assert(#trx.things.samples == 2)

  -- The collection and anything else declared on it answer off one entry, so
  -- the property does not arrive with a metatable of its own and take the
  -- indexing with it.
  api.property("things.samples.total", {
    type = "integer",
    description = "...",
    get = function()
      return 2
    end,
  })
  assert(trx.things.samples.total == 2, "the property must answer")
  assert(trx.things.samples[1] == "a", "and the indexing must survive it")
end)

-- A namespace declares the table it stands for, and would put it where the
-- collection is read through. Nothing about the surface afterwards says the
-- indexing went: it reads as nil, and the audit sees only what a table holds.
test(
  "a namespace cannot replace the table a collection is read through",
  function()
    local api = fresh_env()
    api.module("things", { description = "..." })
    api.container("things.samples", {
      description = "Indexing.",
      key = { type = "integer", description = "A key." },
      value = { type = "string" },
      get = function()
        return "a"
      end,
    })

    local ok, err =
      pcall(api.namespace, "things.samples", { description = "Samples." })
    assert(not ok, "the collection already stands there")
    assert(
      tostring(err):find("already stands for a table", 1, true),
      "the message must say what stands there: " .. tostring(err)
    )
    assert(trx.things.samples[1] == "a", "the indexing must survive it")
  end
)

-- The table a collection is read through holds what was declared inside it,
-- the way a namespace's does, so the audit has to read it as well.
test("seal audits the table a collection is read through", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.container("things.samples", {
    description = "Indexing.",
    key = { type = "integer", description = "A key." },
    value = { type = "string" },
    get = function() end,
  })
  rawset(trx.things.samples, "sneaky", function() end)

  local ok, err = pcall(api.seal)
  assert(not ok, "an undeclared member on the collection must fail the seal")
  assert(
    tostring(err):find("trx.things.samples.sneaky", 1, true),
    "the audit must name it: " .. tostring(err)
  )
end)

test("a collection is declared as a module or a member of one", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  local ok, err = pcall(api.container, "things.a.b", {
    description = "Indexing.",
    key = { type = "integer", description = "A key." },
    value = { type = "string" },
    get = function() end,
  })
  assert(not ok, "three segments deep")
  -- The path the message names is the one that was written, not one with a
  -- segment added to borrow another declaration's parser.
  assert(
    tostring(err):find("got: things.a.b", 1, true),
    "the message must name the path as written: " .. tostring(err)
  )
end)

-- Several collections on one module tie on the module they sit under, and the
-- dump is committed and diffed, so the member has to settle the order.
test("the containers of a module come out in a fixed order", function()
  local api = fresh_env()
  local function collection(path)
    api.container(path, {
      description = "Indexing.",
      key = { type = "integer", description = "A key." },
      value = { type = "string" },
      get = function() end,
    })
  end
  api.module("things", { description = "..." })
  api.module("others", { description = "..." })
  collection("things.tracks")
  collection("others")
  collection("things.samples")

  local seen = {}
  for _, one in ipairs(api.describe().containers) do
    seen[#seen + 1] = one.module .. (one.member and "." .. one.member or "")
  end
  assert(
    table.concat(seen, ",") == "others,things.samples,things.tracks",
    "the order must not follow the declarations: " .. table.concat(seen, ",")
  )
end)

test("a limit needs a count beside it", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  assert(not pcall(api.container, "things", {
    description = "Indexing.",
    key = { type = "integer", description = "A key." },
    value = { type = "string" },
    get = function() end,
    limit = function()
      return 5
    end,
  }))
end)

test("a container key cannot say where it counts from either", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  assert(not pcall(api.container, "things", {
    description = "Indexing.",
    key = { type = "integer", base = 1, description = "A key." },
    value = { type = "string" },
    get = function() end,
  }))
end)

-- Where a collection counts from is the key's to say, so a container that says
-- it itself is a declaration left behind by the move.
test("a container cannot say where it counts from", function()
  local api = fresh_env()
  api.module("things", {})
  assert(not pcall(api.container, "things", {
    description = "Indexing.",
    base = 1,
    key = { type = "integer", description = "Where it sits." },
    value = { type = "string" },
    get = function() end,
  }))
end)

-- A number need not count from anywhere - a sample number is an identifier and
-- nothing more - and a key that names one goes on to the next.
test("a key counts from the first of its numbers that says", function()
  local api = fresh_env()
  api.module("things", {})
  api.number("things.Name", { description = "What it is called." })
  api.number("things.Num", { base = 1, description = "Where it sits." })
  api.container("things", {
    description = "Indexing.",
    key = { type = { "things.Name", "things.Num" }, description = "Either." },
    value = { type = "string" },
    get = function(key)
      return tostring(key)
    end,
    count = function()
      return 2
    end,
  })

  local keys = {}
  for key in pairs(trx.things) do
    keys[#keys + 1] = key
  end
  assert(table.concat(keys, ",") == "1,2", table.concat(keys, ","))
end)

test("a declaration naming several types is checked against each", function()
  local api = fresh_env()
  api.number("things.Num", { base = 0, description = "A thing number." })
  api.define("things.get", {
    params = { { name = "which", type = { "things.Num", "string" } } },
    impl = function()
      return true
    end,
  })

  api.strict(true)
  assert(pcall(trx.things.get, 3))
  assert(pcall(trx.things.get, "hammer"))
  assert(
    not pcall(trx.things.get, {}),
    "a type the declaration does not name must be refused"
  )
  api.strict(false)
end)

test("a property naming several types is checked on write", function()
  local api = fresh_env()
  local held
  api.property("things.size", {
    type = { "integer", "string" },
    description = "How big.",
    get = function()
      return held
    end,
    set = function(value)
      held = value
    end,
  })

  api.strict(true)
  trx.things.size = "large"
  assert(held == "large")
  assert(not pcall(function()
    trx.things.size = {}
  end))
  api.strict(false)
end)

test("seal names the several types a declaration accepts", function()
  local api = fresh_env()
  api.define("things.get", {
    params = { { name = "which", type = { "things.Gone", "string" } } },
    impl = function() end,
  })

  local ok, err = pcall(api.seal)
  assert(not ok, "a type nothing declares must be reported")
  assert(
    err:find("things.Gone or string", 1, true) ~= nil,
    "the message must name what the declaration wrote: " .. tostring(err)
  )
end)

test("a number cannot take the path a type holds", function()
  local api = fresh_env()
  api.type("things.Widget", {})
  assert(
    not pcall(
      api.number,
      "things.Widget",
      { base = 0, description = "A widget number." }
    )
  )
end)

test("a number belongs to a module", function()
  local api = fresh_env()
  assert(not pcall(api.number, "nonsense", { description = "A number." }))
end)

-- The class is what a module gives a value it hands out, and a module that
-- hands out another's values has no other way to reach it.
test("class() hands back the class a type was declared with", function()
  local api = fresh_env()
  local Widget = api.type("things.Widget", {
    methods = { press = { impl = function() end } },
  })

  assert(api.class("things.Widget") == Widget)
  assert(not pcall(api.class, "things.Gadget"))
end)

-- Which of the two a type is, the declaration says. Read off the shape, a type
-- the registry hands out but that carries nothing except keys - math.Box - would
-- be taken for a table a script writes, and its class would go on a value
-- nothing checks by it.
test("a type says whether it is a record or a value with a class", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  local Box = api.type("things.Box", {
    description = "...",
    fields = {
      min_x = { type = "integer", description = "..." },
      max_x = { type = "integer", description = "..." },
    },
  })
  api.type("things.Options", {
    record = true,
    description = "...",
    fields = { mode = { type = "integer", description = "." } },
  })
  api.define("things.fit", {
    description = "...",
    params = {
      { name = "box", type = "things.Box" },
      { name = "opts", type = "things.Options" },
    },
    impl = function() end,
  })

  api.strict(true)
  local written = { min_x = 1, max_x = 5 }
  assert(
    not pcall(trx.things.fit, written, { mode = 1 }),
    "a box the registry did not hand out"
  )
  assert(
    pcall(trx.things.fit, setmetatable(written, Box), { mode = 1 }),
    "one that carries the class"
  )
  api.strict(false)

  -- A record is a plain table, so there is no class to put on one.
  assert(api.class("things.Box") == Box)
  assert(
    not pcall(api.class, "things.Options"),
    "a record must not hand out a class"
  )
end)

test("a record is checked by the keys it names", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  assert(
    not pcall(api.type, "things.Empty", { record = true, description = "." }),
    "a record with no keys has nothing to be checked by"
  )
  assert(not pcall(api.type, "things.Method", {
    record = true,
    description = ".",
    fields = { mode = { type = "integer", description = "." } },
    methods = { poke = { impl = function() end } },
  }), "a record is a plain table, so it has no methods")
  assert(not pcall(api.type, "things.Accessor", {
    record = true,
    description = ".",
    fields = {
      mode = {
        type = "integer",
        description = ".",
        get = function()
          return 1
        end,
      },
    },
  }), "a record has nowhere to keep an accessor")
end)

test("a type declares only what its own shape can hold", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  local computed = { derived = { description = ".", impl = function() end } }
  local operators = { band = { description = ".", impl = function() end } }

  assert(not pcall(api.type, "things.Class", { extensions = computed }))
  assert(not pcall(api.type, "things.Handle", {
    backing = "WIDGET",
    operators = operators,
  }))
  assert(not pcall(api.type, "things.Record", {
    record = true,
    description = ".",
    fields = { mode = { type = "integer", description = "." } },
    extensions = computed,
  }))
end)

test("strict mode says what an argument had to be", function()
  local api = fresh_env()
  api.define("things.spawn", {
    params = { { name = "id", type = "integer" } },
    impl = function() end,
  })

  api.strict(true)
  local ok, why = pcall(trx.things.spawn, "wolf")
  assert(not ok)
  assert(
    why:find("expected integer", 1, true) ~= nil,
    "the message names the parameter but not what it wanted: " .. why
  )
end)

test("seal refuses a collection key nothing can check", function()
  local api = fresh_env()
  api.module("things", { description = "..." })
  api.container("things", {
    description = ".",
    key = { type = "things.Nothing", description = "." },
    value = { type = "integer", description = "." },
    get = function() end,
  })

  local ok, why = pcall(api.seal)
  assert(not ok, "strict mode would wave every key through")
  assert(why:find("things.Nothing", 1, true) ~= nil, why)
end)

-- A suite that registers nothing prints "0 failed" and exits clean, which reads
-- exactly like a suite that passed.
assert(passed + failures > 0, "the suite registered no tests")

print(
  ("\n%d passed, %d failed, %d total"):format(
    passed,
    failures,
    passed + failures
  )
)
os.exit(failures == 0 and 0 or 1)
