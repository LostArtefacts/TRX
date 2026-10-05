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

local FAKE_CONTEXT = 3

-- Matches the position an error points at when it blames the caller in this
-- file, rather than a frame inside the API.
local HERE = "tests/lua/trx/api%.lua:%d+: "

local function empty_signatures()
  return {
    functions = {},
    calls = {},
    methods = {},
    types = {},
    records = {},
    properties = {},
    containers = {},
    members = {},
    writable = {},
    constants = {},
  }
end

-- Stands up api.lua and helpers.lua over a stubbed C bridge. `signatures`
-- overrides the groups of trx.internal.signatures it names; the rest stay empty.
local function fresh_env(signatures)
  local sigs = empty_signatures()
  for group, entries in pairs(signatures or {}) do
    sigs[group] = entries
  end

  local env = {
    signatures = sigs,
    types = {},
    entrypoints = {},
    catalog = {
      { name = "off", value = 0 },
      { name = "on", value = 1 },
    },
  }

  -- C methods by backing type. Each reports its name and what it was handed.
  local c_methods = {
    WIDGET = {
      poke = function(self, ...)
        return "c poke", self, ...
      end,
      c_spin = function(self)
        return "c spin", self
      end,
      describe = function()
        return "c describe"
      end,
    },
  }

  local function type_of(backing)
    env.types[backing] = env.types[backing]
      or { fields = {}, methods = {}, computed = {} }
    return env.types[backing]
  end

  -- A handle as LUA_Struct_Register makes one: its metatable reads as the C
  -- type name, fields read and write the C members, and a method is either a
  -- C method name or a Lua function.
  function env.new_handle(backing, data)
    local t = type_of(backing)
    return setmetatable({}, {
      __metatable = backing,
      __index = function(self, key)
        local field = t.fields[key]
        if field ~= nil then
          return data[field.from]
        end
        local computed = t.computed[key]
        if computed ~= nil then
          return computed(self)
        end
        local method = t.methods[key]
        if type(method) == "string" then
          return c_methods[backing][method]
        end
        return method
      end,
      __newindex = function(_, key, value)
        local field = t.fields[key]
        if field == nil then
          error(("%s has no member '%s'"):format(backing, key), 2)
        end
        if not field.writable then
          error(("%s.%s is read-only"):format(backing, key), 2)
        end
        data[field.from] = value
      end,
    })
  end

  _G.trxc = {
    struct = {
      expose_field = function(backing, public, from, writable)
        type_of(backing).fields[public] = { from = from, writable = writable }
      end,
      expose_method = function(backing, public, impl)
        type_of(backing).methods[public] = impl
      end,
      expose_computed = function(backing, public, fn)
        type_of(backing).computed[public] = fn
      end,
      method = function(backing, name)
        local fn = (c_methods[backing] or {})[name]
        if fn == nil then
          error(("%s has no method '%s'"):format(backing, name))
        end
        return fn
      end,
    },
    enum = {
      values = function(backing)
        if backing == "COLLIDING_STATE" then
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
    catalog = {
      values = function(context)
        assert(
          context == FAKE_CONTEXT,
          "unknown context: " .. tostring(context)
        )
        local out = {}
        for i, constant in ipairs(env.catalog) do
          out[i] = constant
        end
        return out
      end,
      from_key = function(context, key)
        assert(
          context == FAKE_CONTEXT,
          "unknown context: " .. tostring(context)
        )
        for _, constant in ipairs(env.catalog) do
          if constant.name == key then
            return constant.value
          end
        end
        return nil
      end,
    },
    api = {
      set_entrypoint = function(name, fn)
        env.entrypoints[name] = fn
      end,
    },
  }
  _G.trx = {}

  local loaded = {}
  _G.require = function(name)
    if name == "trx.internal.signatures" then
      return sigs
    end
    if loaded[name] == nil then
      local file = name:match("^trx%.(.+)$")
      assert(file ~= nil, "unexpected require: " .. name)
      loaded[name] =
        dofile(ROOT .. "src/lua/trx/" .. file:gsub("%.", "/") .. ".lua")
    end
    return loaded[name]
  end

  dofile(ROOT .. "src/lua/trx/api.lua")

  env.api = trx.api
  env.h = require("trx.internal.helpers")
  return env
end

local function sorted_keys(tbl)
  local names = {}
  for name in pairs(tbl) do
    names[#names + 1] = name
  end
  table.sort(names)
  return table.concat(names, ",")
end

local function raises(fn, pattern, ...)
  local ok, err = pcall(fn, ...)
  assert(not ok, "expected an error matching " .. pattern)
  assert(
    tostring(err):find(pattern) ~= nil,
    ("error %q does not match %q"):format(tostring(err), pattern)
  )
  return err
end

local STATE_DOCS = { OFF = "off.", ON = "on.", BROKEN = "broken." }

-- Enums

test("an enum reflects its constants out of C", function()
  local env = fresh_env()
  local State = env.h.enum("things.State", "WIDGET_STATE", STATE_DOCS)

  assert(State.OFF == 0)
  assert(State.ON == 1)
  assert(State.BROKEN == 7, "the value is C's, gaps and all")
end)

test(
  "an enum answers to a name in any case, and cannot be written to",
  function()
    local env = fresh_env()
    local State = env.h.enum("things.State", "WIDGET_STATE", STATE_DOCS)

    assert(State.on == 1)
    assert(State.On == 1)
    assert(State.invalid == nil)
    assert(State[1] == nil, "a key that is not a name is nil")
    assert(
      sorted_keys(State) == "BROKEN,OFF,ON",
      "pairs() yields the canonical spelling only"
    )

    raises(function()
      State.ON = 7
    end, "trx%.things%.State%.ON: an enum cannot be written to")
    assert(State.ON == 1)
  end
)

test("an enum rejects a constant nobody documented", function()
  local env = fresh_env()
  raises(
    env.h.enum,
    "^helpers%.enum: things%.State%.BROKEN is not documented$",
    "things.State",
    "WIDGET_STATE",
    { OFF = "off.", ON = "on." }
  )
end)

test("an enum rejects docs for a constant that does not exist", function()
  local env = fresh_env()
  raises(
    env.h.enum,
    "things%.State%.IMAGINARY is not a constant of WIDGET_STATE",
    "things.State",
    "WIDGET_STATE",
    { OFF = "", ON = "", BROKEN = "", IMAGINARY = "" }
  )
end)

test("an enum rejects two constants that fold onto one name", function()
  local env = fresh_env()
  raises(
    env.h.enum,
    "things%.State%.ON is the name of two constants of COLLIDING_STATE",
    "things.State",
    "COLLIDING_STATE"
  )
end)

test("an enum rejects docs that are not a table", function()
  local env = fresh_env()
  raises(
    env.h.enum,
    "docs must be a table",
    "things.State",
    "WIDGET_STATE",
    "docs"
  )
end)

test("a bulk enum needs no docs and still holds every constant", function()
  local env = fresh_env()
  local State = env.h.enum("catalog.states", "WIDGET_STATE")
  assert(State.ON == 1 and State.broken == 7)
  assert(sorted_keys(State) == "BROKEN,OFF,ON")
end)

test("a catalog enum reads its constants from the catalog", function()
  local env = fresh_env()
  local Kind = env.h.catalog("catalog.kinds", FAKE_CONTEXT)

  assert(Kind.OFF == 0 and Kind.on == 1)
  assert(sorted_keys(Kind) == "OFF,ON")
  assert(Kind.OIL_DRUM == nil)

  env.catalog[#env.catalog + 1] = { name = "oil_drum", value = 9 }
  assert(Kind.oil_drum == 9, "a name minted later is asked of the catalog")
  assert(Kind.OIL_DRUM == 9, "in any case")
  assert(Kind.WOMBAT == nil)
  assert(sorted_keys(Kind) == "OFF,OIL_DRUM,ON", "pairs() reads it again")

  raises(function()
    Kind.OFF = 3
  end, "an enum cannot be written to")
end)

-- Handles

test("a handle exposes its fields, read-only unless listed", function()
  local env = fresh_env()
  env.h.handle("things.Widget", "WIDGET", {
    fields = { shown = "visible", locked = "lock" },
    writable = { "shown" },
  })

  local fields = env.types.WIDGET.fields
  assert(fields.shown.from == "visible" and fields.shown.writable == true)
  assert(fields.locked.from == "lock" and fields.locked.writable == false)

  local data = { visible = 1, lock = 2 }
  local widget = env.new_handle("WIDGET", data)
  assert(widget.shown == 1 and widget.locked == 2)
  widget.shown = 5
  assert(data.visible == 5)
  raises(function()
    widget.locked = 3
  end, "read%-only")

  local reflected = env.h.reflected().fields["things.Widget"]
  assert(reflected.shown == true and reflected.locked == false)
end)

test("a handle rejects a writable name that is not a field", function()
  local env = fresh_env()
  raises(
    env.h.handle,
    "^helpers%.handle: things%.Widget%.ghost is not a field$",
    "things.Widget",
    "WIDGET",
    { fields = { shown = "visible" }, writable = { "ghost" } }
  )
end)

test("a handle exposes its extensions as computed members", function()
  local env = fresh_env()
  env.h.handle("things.Widget", "WIDGET", {
    fields = { shown = "visible" },
    extensions = {
      twice = function(self)
        return self.shown * 2
      end,
      half = function(self)
        return self.shown / 2
      end,
    },
  })

  local widget = env.new_handle("WIDGET", { visible = 4 })
  assert(widget.twice == 8 and widget.half == 2)

  raises(
    env.h.handle,
    "things%.Gadget%.bad: an extension is a function",
    "things.Gadget",
    "WIDGET",
    { extensions = { bad = 1 } }
  )
end)

test("a handle method binds C where C has one of that name", function()
  local env = fresh_env()
  local Widget = env.h.handle("things.Widget", "WIDGET", {
    methods = { spin = "c_spin" },
    lua = { "describe" },
  })
  function Widget:poke() end
  function Widget:spin() end
  function Widget:describe()
    return "lua describe"
  end
  function Widget:shout()
    return "lua shout"
  end

  local methods = env.types.WIDGET.methods
  assert(methods.poke == "poke", "a C method name binds C")
  assert(methods.spin == "c_spin", "`methods` renames to a C method")
  assert(type(methods.describe) == "function", "`lua` keeps the Lua body")
  assert(type(methods.shout) == "function", "no C method keeps the Lua body")
  assert(rawget(Widget, "poke") ~= nil, "the declaration keeps its body")

  local widget = env.new_handle("WIDGET", {})
  assert(widget:poke() == "c poke")
  assert(widget:spin() == "c spin")
  assert(widget:describe() == "lua describe")
  assert(widget:shout() == "lua shout")
  assert(env.h.handles()["things.Widget"].methods.spin == "c_spin")
end)

test("a handle rejects a rename to a C method that is not there", function()
  local env = fresh_env()
  local Widget = env.h.handle("things.Widget", "WIDGET", {
    methods = { spin = "c_missing" },
  })
  raises(function()
    function Widget:spin() end
  end, "things%.Widget%.spin: C has no method 'c_missing'")
  raises(function()
    Widget.bad = 1
  end, "things%.Widget%.bad: a method is a function")
end)

-- Lua classes

test("a class binds the methods assigned to it", function()
  local env = fresh_env()
  local Widget = env.h.class("things.Widget")
  function Widget:poke()
    return self.name .. " poked"
  end

  local widget = setmetatable({ name = "hinge" }, Widget)
  assert(widget:poke() == "hinge poked")
  assert(env.h.class_of("things.Widget") == Widget)
  assert(env.h.classes()["things.Widget"].class == Widget)
  raises(env.h.class_of, "'things%.Gadget' is not declared", "things.Gadget")
end)

test("a derived class inherits methods and operators", function()
  local env = fresh_env()
  local Widget = env.h.class("things.Widget", {
    operators = {
      band = function(a, b)
        return a.name .. "+" .. b.name
      end,
      tostring = function(self)
        return "widget " .. self.name
      end,
    },
  })
  function Widget:poke()
    return self.name
  end
  local Lever = env.h.class("things.Lever", { extends = "things.Widget" })
  function Lever:pull()
    return self.name .. " pulled"
  end

  local lever = setmetatable({ name = "lever" }, Lever)
  assert(lever:pull() == "lever pulled")
  assert(lever:poke() == "lever")
  assert((lever & setmetatable({ name = "other" }, Lever)) == "lever+other")
  assert(tostring(lever) == "widget lever")
  assert(getmetatable(Widget) == nil, "the base class gained a metatable")
end)

test("a class reads and writes fields through its accessors", function()
  local env = fresh_env()
  local state = {}
  local Widget = env.h.class("things.Widget", {
    fields = {
      name = {
        get = function(self)
          return state[self].name
        end,
        set = function(self, value)
          state[self].name = value
        end,
      },
      size = {
        get = function(self)
          return #state[self].name
        end,
      },
    },
    operators = {
      tostring = function(self)
        return self.name
      end,
    },
  })
  function Widget:poke()
    return self.name .. "!"
  end

  local widget = setmetatable({}, Widget)
  state[widget] = { name = "hinge" }
  assert(widget.name == "hinge" and widget.size == 5)
  assert(widget:poke() == "hinge!", "a method is still reachable")
  assert(tostring(widget) == "hinge")
  assert(widget.__tostring == nil, "a metamethod is no member of the type")
  assert(widget.colour == nil)

  widget.name = "lever"
  assert(widget.name == "lever")
  raises(function()
    widget.size = 3
  end, HERE .. "things%.Widget%.size is read%-only$")
  raises(function()
    widget.colour = "red"
  end, HERE .. "things%.Widget has no member 'colour'$")

  local fields = env.h.reflected().class_fields["things.Widget"]
  assert(fields.name == true and fields.size == false)
end)

test("a derived class inherits its parent's accessor fields", function()
  local env = fresh_env()
  env.h.class("things.Widget", {
    fields = {
      name = {
        get = function(self)
          return rawget(self, "held")
        end,
        set = function(self, value)
          rawset(self, "held", value)
        end,
      },
    },
  })
  local Lever = env.h.class("things.Lever", { extends = "things.Widget" })
  function Lever:pull()
    return self.name .. " pulled"
  end

  local lever = setmetatable({ held = "lever" }, Lever)
  assert(lever.name == "lever")
  lever.name = "pulled lever"
  assert(lever:pull() == "pulled lever pulled")
  raises(function()
    lever.colour = "red"
  end, "things%.Lever has no member 'colour'")
end)

test("a class rejects what it cannot install", function()
  local env = fresh_env()
  raises(
    env.h.class,
    "things%.Lever extends an undeclared type",
    "things.Lever",
    { extends = "things.Widget" }
  )
  raises(
    env.h.class,
    "things%.Widget: operator 'eq' is a function",
    "things.Widget",
    { operators = { eq = 1 } }
  )
  raises(
    env.h.class,
    "things%.Widget%.name: get must be a function",
    "things.Widget",
    { fields = { name = { set = function() end } } }
  )
end)

-- Tables with computed members

test("a property reads through get and writes through set", function()
  local env = fresh_env()
  local things = env.h.module("things")
  local air = 100
  env.h.properties(things, "things", {
    air = {
      get = function()
        return air
      end,
      set = function(value)
        air = value
      end,
    },
    depth = {
      get = function()
        return 3
      end,
    },
  })

  assert(trx.things == things)
  assert(things.air == 100 and things.depth == 3)
  things.air = 50
  assert(air == 50)
  raises(function()
    things.depth = 4
  end, HERE .. "trx%.things%.depth is read%-only$")

  local reflected = env.h.reflected().properties
  assert(reflected["things.air"] == true)
  assert(reflected["things.depth"] == false)
end)

test("a property rejects accessors that are not functions", function()
  local env = fresh_env()
  raises(
    env.h.properties,
    "things%.air: get must be a function",
    {},
    "things",
    { air = {} }
  )
  raises(
    env.h.properties,
    "things%.air: set must be a function",
    {},
    "things",
    { air = { get = function() end, set = 1 } }
  )
end)

test("a container walks from the index it counts from", function()
  local env = fresh_env()
  local held = { [0] = "a", "b", "c" }
  local zero = env.h.container("things.zero", {
    base = 0,
    get = function(i)
      return held[i]
    end,
    count = function()
      return 3
    end,
  })
  local one = env.h.container("things.one", {
    base = 1,
    get = function(i)
      return held[i]
    end,
    count = function()
      return 2
    end,
  })

  assert(#zero == 3 and #one == 2)
  assert(zero[0] == "a" and zero[2] == "c")
  local walked = {}
  for i, value in pairs(zero) do
    walked[#walked + 1] = i .. "=" .. value
  end
  assert(table.concat(walked, ",") == "0=a,1=b,2=c")
  walked = {}
  for i, value in pairs(one) do
    walked[#walked + 1] = i .. "=" .. value
  end
  assert(table.concat(walked, ",") == "1=b,2=c")
  assert(zero.name == nil, "a name does not reach a collection by number")
end)

test("a sparse container walks up to its limit and skips the gaps", function()
  local env = fresh_env()
  local held = { [1] = "a", [4] = "d", [6] = "f" }
  local sparse = env.h.container("things.sparse", {
    base = 1,
    get = function(i)
      return held[i]
    end,
    count = function()
      return 3
    end,
    limit = function()
      return 6
    end,
  })

  assert(#sparse == 3, "# says how many there are")
  local walked = {}
  for i in pairs(sparse) do
    walked[#walked + 1] = i
  end
  assert(table.concat(walked, ",") == "1,4,6", table.concat(walked, ","))
end)

test("a container keyed by name hands the name to get", function()
  local env = fresh_env()
  local things = env.h.module("things")
  local function get(key)
    return "got " .. tostring(key)
  end
  local tbl =
    env.h.container("things", { base = 0, by_name = true, get = get }, things)

  assert(tbl == things, "the collection is the table it was given")
  assert(things.wolf == "got wolf")
  assert(things[3] == "got 3")
  assert(getmetatable(things).__len == nil, "no count means no #")
end)

test("a container rejects a spec it cannot serve", function()
  local env = fresh_env()
  raises(env.h.container, "things: get must be a function", "things", {
    base = 0,
  })
  raises(
    env.h.container,
    "things: base counts from 0 or from 1",
    "things",
    { base = 2, get = function() end }
  )
  raises(
    env.h.container,
    "things: a limit needs a count beside it",
    "things",
    { base = 0, get = function() end, limit = function() end }
  )
end)

local function widget_handle(env)
  env.h.handle("things.Widget", "WIDGET", {
    fields = { shown = "visible", locked = "lock" },
    writable = { "shown" },
  })
  local data = { visible = 1, lock = 2 }
  local handle = env.new_handle("WIDGET", data)
  env.types.WIDGET.methods.poke = "poke"
  return handle, data
end

test("a mirroring module shows the fields of the handle", function()
  local env = fresh_env()
  local handle, data = widget_handle(env)
  local things = env.h.module("things")
  env.h.mirror(things, "things", function()
    return handle
  end, "things.Widget")

  assert(things.shown == 1)
  data.visible = false
  assert(things.shown == false, "a false field reads as false")
  assert(things.poke == nil, "a method stays on the handle")

  things.shown = 7
  assert(data.visible == 7, "a write reaches the handle")
  raises(function()
    things.locked = 3
  end, "read%-only")
  raises(function()
    things.missing = 3
  end, HERE .. "Cannot set field 'missing' on trx%.things$")
end)

test("a module mirrors only a declared handle", function()
  local env = fresh_env()
  local things = env.h.module("things")
  raises(function()
    env.h.mirror(things, "things", function() end, "things.Nothing")
  end, "things%.Nothing is no handle")
end)

test("a mirroring module with no handle refuses writes", function()
  local env = fresh_env()
  widget_handle(env)
  local things = env.h.module("things")
  env.h.mirror(things, "things", function()
    return nil
  end, "things.Widget")

  assert(things.shown == nil)
  raises(function()
    things.shown = 3
  end, HERE .. "Cannot set field 'shown' on trx%.things$")
end)

test("a mirroring module that is also a collection serves both", function()
  local env = fresh_env()
  local handle = widget_handle(env)
  local things = env.h.module("things")
  env.h.mirror(things, "things", function()
    return handle
  end, "things.Widget")
  env.h.container("things", {
    base = 1,
    get = function(i)
      return "entry " .. i
    end,
    count = function()
      return 2
    end,
  }, things)

  assert(things[2] == "entry 2" and #things == 2)
  assert(things.shown == 1, "a name still reaches the handle")
end)

test("a namespace groups members and can itself be called", function()
  local env = fresh_env()
  local plain = env.h.namespace("things.signals")
  assert(getmetatable(plain) == nil, "a plain namespace is a plain table")

  local logged
  local log = env.h.namespace("things.log", function(message)
    logged = message
    return "logged"
  end)
  assert(log("hello") == "logged" and logged == "hello")
  assert(env.h.callables()["things.log"] ~= nil)
  assert(env.h.callables()["things.signals"] == nil)
end)

test("const returns its value and records it", function()
  local env = fresh_env()
  assert(env.h.const("things.LIMIT", 5) == 5)
  assert(env.h.reflected().constants["things.LIMIT"] == 5)
end)

-- The declared-member rule

test("a module table takes only the members its signatures declare", function()
  local env = fresh_env({ members = { things = { "helper" } } })
  local things = env.h.module("things")
  env.h.properties(things, "things", {
    air = {
      get = function()
        return 1
      end,
    },
  })

  local helper = function() end
  things.helper = helper
  assert(rawget(things, "helper") == helper)
  raises(function()
    things.stray = 1
  end, HERE .. "Cannot set field 'stray' on trx%.things$")
  assert(rawget(things, "stray") == nil)
end)

test("a declared member of a mirroring module stays on the module", function()
  local env = fresh_env({ members = { things = { "helper" } } })
  local handle, data = widget_handle(env)
  local things = env.h.module("things")
  env.h.mirror(things, "things", function()
    return handle
  end, "things.Widget")

  things.helper = 1
  assert(
    rawget(things, "helper") == 1,
    "a declared member stays on the module"
  )
  things.shown = 9
  assert(rawget(things, "shown") == nil and data.visible == 9)
end)

-- Strict mode

test("strict mode is off until something turns it on", function()
  local env = fresh_env()
  assert(env.api.is_strict() == false)
  env.api.strict(true)
  assert(env.api.is_strict() == true)
  env.api.strict(false)
  assert(env.api.is_strict() == false)
end)

local POS = {
  { name = "x", type = "integer" },
  { name = "y", type = "integer" },
  { name = "z", type = "integer", optional = true },
}

test("strict mode rejects bad arguments and accepts good ones", function()
  local env = fresh_env({
    records = { ["things.Pos"] = POS },
    functions = {
      ["things.spawn"] = {
        { name = "id", type = "integer" },
        { name = "pos", type = "things.Pos" },
      },
    },
  })
  local things = env.h.module("things")
  local raw = function(id, pos)
    return id + pos.x
  end
  things.spawn = raw

  env.api.strict(true)
  assert(things.spawn ~= raw, "strict mode wraps the function")
  assert(things.spawn(1, { x = 2, y = 3 }) == 3)
  raises(function()
    things.spawn("wolf", { x = 1, y = 1 })
  end, HERE .. "things%.spawn: invalid argument 'id' %- expected integer$")
  raises(
    things.spawn,
    "^things%.spawn: invalid argument 'pos' %- not a table$",
    1,
    "here"
  )
  raises(
    things.spawn,
    "invalid argument 'pos' %- no such key 'w'",
    1,
    { x = 1, y = 1, w = 1 }
  )
  raises(
    things.spawn,
    "invalid argument 'pos' %- 'y' is missing",
    1,
    { x = 1 }
  )
  raises(
    things.spawn,
    "invalid argument 'pos' %- 'x': expected integer",
    1,
    { x = 1.5, y = 1 }
  )

  env.api.strict(false)
  assert(things.spawn == raw, "turning strict off restores the raw function")
end)

test("strict mode leaves a function that takes nothing unwrapped", function()
  local env = fresh_env({ functions = { ["things.count"] = {} } })
  local things = env.h.module("things")
  local raw = function()
    return 3
  end
  things.count = raw
  env.api.strict(true)
  assert(things.count == raw)
end)

test("strict mode checks inline fields, lists and several types", function()
  local env = fresh_env({
    types = { ["things.Num"] = "integer" },
    functions = {
      ["things.play"] = {
        {
          name = "opts",
          fields = { { name = "loud", type = "boolean", optional = true } },
        },
      },
      ["things.pick"] = { { name = "ids", type = "integer", list = true } },
      ["things.find"] = {
        { name = "key", type = { "things.Num", "string" } },
      },
      ["things.log"] = {
        { name = "level", type = "string" },
        { name = "..." },
      },
    },
  })
  local things = env.h.module("things")
  things.play = function() end
  things.pick = function() end
  things.find = function() end
  things.log = function(...)
    return select("#", ...)
  end
  env.api.strict(true)

  things.play({ loud = true })
  things.play({})
  raises(things.play, "'opts' %- 'loud': expected boolean", { loud = 1 })
  raises(things.play, "'opts' %- not a table", nil)

  things.pick({ 1, 2 })
  raises(
    things.pick,
    "^things%.pick: invalid argument 'ids' %- entry 2: expected integer$",
    { 1, "two" }
  )
  raises(things.pick, "'ids' %- not a list", 1)

  things.find(3)
  things.find("wolf")
  raises(
    things.find,
    "^things%.find: invalid argument 'key' %- expected things%.Num or string$",
    1.5
  )

  assert(things.log("info", 1, 2, 3) == 4, "the variadic tail goes through")
  raises(things.log, "invalid argument 'level'", 1)
end)

test("strict mode drops an optional argument that has no default", function()
  local env = fresh_env({
    functions = {
      ["things.flip"] = {
        { name = "id", type = "integer" },
        { name = "timer", type = "integer", optional = true },
      },
    },
  })
  local things = env.h.module("things")
  things.flip = function(...)
    return select("#", ...)
  end
  env.api.strict(true)

  assert(things.flip(1) == 1, "an omitted optional must not arrive as nil")
  assert(things.flip(1, 5) == 2)
  raises(things.flip, "invalid argument 'timer'", 1, "soon")
end)

test("strict mode substitutes a default and still checks it", function()
  local env = fresh_env({
    functions = {
      ["things.track"] = {
        { name = "track", type = "integer", optional = true, default = 3 },
        { name = "loop", type = "boolean", optional = true, default = false },
      },
    },
  })
  local things = env.h.module("things")
  things.track = function(...)
    return select("#", ...), ...
  end
  env.api.strict(true)

  local n, track, loop = things.track()
  assert(n == 2 and track == 3 and loop == false, "both defaults stand in")
  n, track, loop = things.track(5, true)
  assert(n == 2 and track == 5 and loop == true)
  raises(things.track, "invalid argument 'track' %- expected integer", "nope")
  raises(things.track, "invalid argument 'loop' %- expected boolean", 1, 2)
end)

test("strict mode reads a default given as a function", function()
  local env = fresh_env({
    functions = {
      ["things.set_state"] = {
        {
          name = "state",
          type = "things.State",
          optional = true,
          default = function()
            return trx.things.State.ON
          end,
        },
      },
    },
    types = { ["things.State"] = "integer" },
  })
  local things = env.h.module("things")
  things.State = env.h.enum("things.State", "WIDGET_STATE", STATE_DOCS)
  things.set_state = function(state)
    return state
  end
  env.api.strict(true)

  assert(things.set_state() == 1)
  assert(things.set_state(things.State.BROKEN) == 7)
  raises(things.set_state, "expected things%.State", "ON")
end)

test("strict mode keeps a default of false given as a function", function()
  local env = fresh_env({
    functions = {
      ["things.toggle"] = {
        {
          name = "on",
          type = "boolean",
          optional = true,
          default = function()
            return false
          end,
        },
      },
    },
  })
  local things = env.h.module("things")
  local seen = "unset"
  things.toggle = function(on)
    seen = on
  end
  env.api.strict(true)

  things.toggle()
  assert(seen == false, "the false default was dropped")
end)

test("strict mode passes by a declared function nothing defines", function()
  local env = fresh_env({
    functions = {
      ["things.ghost"] = { { name = "n", type = "integer" } },
    },
  })
  env.h.module("things")
  env.api.strict(true)
  env.api.strict(false)

  assert(rawget(trx.things, "ghost") == nil, "a wrapper around nil was bound")
end)

test("strict mode checks a namespace's call", function()
  local env = fresh_env({
    calls = { ["things.log"] = { { name = "message", type = "string" } } },
  })
  local things = env.h.module("things")
  local logged
  local raw = function(message)
    logged = message
  end
  things.log = env.h.namespace("things.log", raw)
  env.api.strict(true)

  things.log("hello")
  assert(logged == "hello")
  raises(
    things.log,
    "^things%.log: invalid argument 'message' %- expected string$",
    42
  )

  env.api.strict(false)
  things.log(42)
  assert(logged == 42)
  assert(env.h.callables()["things.log"].call == raw)
end)

test("strict mode blames the caller of a namespace's call", function()
  local env = fresh_env({
    calls = { ["things.log"] = { { name = "message", type = "string" } } },
  })
  local things = env.h.module("things")
  things.log = env.h.namespace("things.log", function() end)
  env.api.strict(true)

  raises(function()
    things.log(42)
  end, HERE .. "things%.log: invalid argument")
end)

test("strict mode checks a property's write", function()
  local env = fresh_env({
    properties = { ["things.air"] = { type = "integer" } },
  })
  local things = env.h.module("things")
  local air = 0
  env.h.properties(things, "things", {
    air = {
      get = function()
        return air
      end,
      set = function(value)
        air = value
      end,
    },
  })
  env.api.strict(true)

  things.air = 5
  assert(air == 5)
  raises(function()
    things.air = "lots"
  end, HERE .. "trx%.things%.air: expected integer$")
  assert(air == 5)

  env.api.strict(false)
  things.air = "lots"
  assert(air == "lots")
end)

test("strict mode lets a nullable property take nil", function()
  local env = fresh_env({
    properties = { ["things.air"] = { type = "integer", optional = true } },
  })
  local things = env.h.module("things")
  local air = 0
  env.h.properties(things, "things", {
    air = {
      get = function()
        return air
      end,
      set = function(value)
        air = value
      end,
    },
  })
  env.api.strict(true)

  things.air = nil
  assert(air == nil)
  raises(function()
    things.air = "lots"
  end, HERE .. "trx%.things%.air: expected integer$")
end)

test("strict mode checks what a collection is indexed with", function()
  local env = fresh_env({
    types = { ["things.Num"] = "integer" },
    containers = { ["things"] = { type = { "things.Num", "string" } } },
  })
  local things = env.h.module("things")
  env.h.container("things", {
    base = 0,
    by_name = true,
    get = function(key)
      return key
    end,
  }, things)
  env.api.strict(true)

  assert(things[2] == 2 and things.wolf == "wolf")
  raises(function()
    return things[1.5]
  end, HERE .. "trx%.things%[1%.5%]: expected things%.Num or string$")

  env.api.strict(false)
  assert(things[1.5] == 1.5)
end)

test("strict mode checks the methods of a handle", function()
  local env = fresh_env({
    methods = {
      ["things.Widget.poke"] = { { name = "force", type = "integer" } },
      ["things.Widget.shout"] = { { name = "text", type = "string" } },
    },
  })
  local Widget = env.h.handle("things.Widget", "WIDGET", {})
  function Widget:poke() end
  function Widget:shout(text)
    return text .. "!"
  end
  local widget = env.new_handle("WIDGET", {})
  env.api.strict(true)

  local said, self, force = widget:poke(3)
  assert(said == "c poke" and self == widget and force == 3)
  assert(widget:shout("hey") == "hey!")
  raises(
    function()
      widget:poke("hard")
    end,
    HERE .. "things%.Widget%.poke: invalid argument 'force' %- expected int"
  )
  local dot = "things%.Widget%.shout: invalid argument 'self' %- "
  raises(function()
    widget.shout("hey")
  end, HERE .. dot .. "expected things%.Widget$")

  env.api.strict(false)
  assert(env.types.WIDGET.methods.poke == "poke", "the C method name is back")
  assert(env.types.WIDGET.methods.shout == rawget(Widget, "shout"))
end)

test("strict mode checks the methods of a class", function()
  local env = fresh_env({
    methods = {
      ["things.Widget.poke"] = { { name = "force", type = "integer" } },
    },
  })
  local Widget = env.h.class("things.Widget")
  local raw = function(self, force)
    return self.name .. force
  end
  Widget.poke = raw
  local Lever = env.h.class("things.Lever", { extends = "things.Widget" })
  env.api.strict(true)

  local widget = setmetatable({ name = "w" }, Widget)
  assert(widget:poke(1) == "w1")
  assert(
    setmetatable({ name = "l" }, Lever):poke(2) == "l2",
    "a derived value counts as the class it extends"
  )
  local dot = "things%.Widget%.poke: invalid argument 'self' %- "
  raises(function()
    widget.poke(1)
  end, HERE .. dot .. "expected things%.Widget$")
  raises(function()
    widget:poke("x")
  end, "invalid argument 'force'")
  raises(function()
    Widget.poke({ name = "plain" }, 1)
  end, "invalid argument 'self'")

  env.api.strict(false)
  assert(rawget(Widget, "poke") == raw)
end)

test("strict mode checks a handle passed as an argument", function()
  local env = fresh_env({
    functions = {
      ["things.use"] = { { name = "widget", type = "things.Widget" } },
    },
  })
  env.h.handle("things.Widget", "WIDGET", {})
  local things = env.h.module("things")
  things.use = function() end
  env.api.strict(true)

  things.use(env.new_handle("WIDGET", {}))
  raises(things.use, "expected things%.Widget", {})
end)

test("strict mode waves through a type nobody declares", function()
  local env = fresh_env({
    functions = {
      ["things.use"] = { { name = "a", type = "things.Nothing" } },
    },
  })
  local things = env.h.module("things")
  things.use = function(a)
    return a
  end
  env.api.strict(true)
  assert(things.use("anything") == "anything")
end)

-- The seal

test("the seal reports members nobody annotated", function()
  local env = fresh_env({
    members = {
      things = { "signals", "spawn" },
      ["things.signals"] = { "ready" },
    },
  })
  local things = env.h.module("things")
  things.spawn = function() end
  things.stray = function() end
  things.signals = { ready = 1, extra = 2 }

  raises(
    env.entrypoints.seal,
    ": trx%.things%.signals%.extra, trx%.things%.stray: reachable from scripts"
  )
end)

test("the seal reports members annotated but not defined", function()
  local env = fresh_env({ members = { things = { "spawn", "ghost" } } })
  env.h.module("things").spawn = function() end

  raises(env.entrypoints.seal, "trx%.things%.ghost: annotated but not defined")
end)

test("a partial seal tolerates a member another module adds", function()
  local env = fresh_env({ members = { things = { "spawn", "ghost" } } })
  env.h.module("things").spawn = function() end
  env.entrypoints.seal({ partial = true })
end)

test("the seal holds a property to what its annotation says", function()
  local env = fresh_env({ writable = { things = { level = false } } })
  local things = env.h.module("things")
  env.h.properties(things, "things", {
    level = {
      get = function()
        return 1
      end,
      set = function() end,
    },
  })

  raises(env.entrypoints.seal, "trx%.things%.level takes a write")
end)

test("the seal reports a property annotated but not built", function()
  local env = fresh_env({ writable = { things = { speed = true } } })
  env.h.module("things")

  raises(
    env.entrypoints.seal,
    "trx%.things%.speed is annotated, but the module does not build it"
  )
end)

test("the seal reports a property that takes an unannotated write", function()
  local env = fresh_env()
  local things = env.h.module("things")
  env.h.properties(things, "things", {
    level = {
      get = function()
        return 1
      end,
      set = function() end,
    },
  })

  raises(
    env.entrypoints.seal,
    "trx%.things%.level takes a write, but it is not annotated"
  )
end)

test("the seal passes a class field held as plain data", function()
  local env = fresh_env({ writable = { ["things.Widget"] = { name = true } } })
  env.h.class("things.Widget")
  env.entrypoints.seal()
end)

test("the seal holds a constant to the value its annotation prints", function()
  local env = fresh_env({
    members = { things = { "MAX" } },
    constants = { ["things.MAX"] = 8 },
  })
  env.h.module("things").MAX = env.h.const("things.MAX", 9)

  raises(
    env.entrypoints.seal,
    "trx%.things%.MAX is 9, but the annotations say 8"
  )
end)

test("the seal passes a surface that is all declared", function()
  local env = fresh_env({ members = { things = { "spawn" } } })
  env.h.module("things").spawn = function() end
  env.entrypoints.seal()
end)

test("the checking layer is never on trx", function()
  fresh_env()
  assert(rawget(trx, "check") == nil)
  assert(rawget(trx, "helpers") == nil)
end)

test("the seal reports a type nothing can check", function()
  local env = fresh_env({
    functions = { ["things.bad"] = { { name = "a", type = "intger" } } },
    methods = {
      ["things.Widget.poke"] = { { name = "b", type = "things.Nothing" } },
    },
    properties = { ["things.air"] = { type = "things.Air" } },
    containers = { ["things"] = { type = "things.Num" } },
  })

  local err = raises(
    env.entrypoints.seal,
    "things%.bad: 'a' has an unknown type 'intger'"
  )
  for _, line in ipairs({
    "things.Widget.poke: 'b' has an unknown type 'things.Nothing'",
    "things.air: the property has an unknown type 'things.Air'",
    "things: the key has an unknown type 'things.Num'",
  }) do
    assert(err:find(line, 1, true), err)
  end
end)

test("the seal reports a default of the wrong type", function()
  local env = fresh_env({
    functions = {
      ["things.bad"] = {
        {
          name = "a",
          type = "integer",
          optional = true,
          default = "trx.things.A",
        },
      },
      ["things.worse"] = {
        {
          name = "b",
          type = "integer",
          optional = true,
          default = function()
            return "late"
          end,
        },
      },
      ["things.good"] = {
        {
          name = "c",
          type = "integer",
          optional = true,
          default = function()
            return 4
          end,
        },
      },
    },
  })

  local err = raises(
    env.entrypoints.seal,
    "things%.bad: the default for 'a' is not integer"
  )
  assert(
    err:find("things.worse: the default for 'b' is not integer", 1, true),
    err
  )
  assert(not err:find("things.good", 1, true), err)
end)

test("a partial seal tolerates a type it cannot check", function()
  local env = fresh_env({
    functions = {
      ["things.bad"] = { { name = "a", type = "things.Elsewhere" } },
    },
  })
  raises(env.entrypoints.seal, "unknown type 'things%.Elsewhere'")
  env.entrypoints.seal({ partial = true })
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
