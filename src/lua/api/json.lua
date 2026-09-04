local raw = trxc.json
local api = trx.api

api.module("json", {
  order = 35,
  title = "JSON",
  description = "Reading and writing JSON, both as text and as a file on disk. The API "
    .. "dump the reference is generated from goes through this, so what a script writes out is "
    .. "encoded the way the engine's own data is.",
})

local ESCAPED = {
  ["\\"] = "\\\\",
  ['"'] = '\\"',
  ["\n"] = "\\n",
  ["\t"] = "\\t",
  ["\r"] = "\\r",
}

local function quoted(text)
  return '"'
    .. text:gsub('[%c"\\]', function(c)
      return ESCAPED[c] or ("\\u%04x"):format(c:byte())
    end)
    .. '"'
end

local function is_list(value)
  return value[1] ~= nil or next(value) == nil
end

local function is_encodable(value)
  local kind = type(value)
  return kind == "nil"
    or kind == "boolean"
    or kind == "number"
    or kind == "string"
    or kind == "table"
end

local function write(value, out, open_tables)
  local kind = type(value)
  if value == nil then
    out[#out + 1] = "null"
  elseif kind == "boolean" then
    out[#out + 1] = tostring(value)
  elseif kind == "number" then
    assert(
      value == value and value ~= math.huge and value ~= -math.huge,
      "trx.json.encode: " .. tostring(value) .. " has no JSON"
    )
    out[#out + 1] = tostring(value)
  elseif kind == "string" then
    out[#out + 1] = quoted(value)
  elseif kind == "table" then
    assert(
      not open_tables[value],
      "trx.json.encode: the table contains itself"
    )
    open_tables[value] = true
    if is_list(value) then
      out[#out + 1] = "["
      for i, held in ipairs(value) do
        if i > 1 then
          out[#out + 1] = ","
        end
        if not is_encodable(held) then
          held = nil
        end
        write(held, out, open_tables)
      end
      out[#out + 1] = "]"
    else
      local keys, by_name = {}, {}
      for key, entry in pairs(value) do
        local kind_of_key = type(key)
        if
          (kind_of_key == "string" or kind_of_key == "number")
          and is_encodable(entry)
        then
          local name = tostring(key)
          keys[#keys + 1] = name
          by_name[name] = entry
        end
      end
      table.sort(keys)
      out[#out + 1] = "{"
      for i, key in ipairs(keys) do
        if i > 1 then
          out[#out + 1] = ","
        end
        out[#out + 1] = quoted(key)
        out[#out + 1] = ":"
        write(by_name[key], out, open_tables)
      end
      out[#out + 1] = "}"
    end
    open_tables[value] = nil
  else
    out[#out + 1] = "null"
  end
end

local encode = api.define("json.encode", {
  description = [[
    Writes a value out as JSON, on one line. Keys come out in sorted order, so
    the same value encodes the same way twice and a file that is committed and
    diffed only moves when its JSON changes.

    A table is written as a list when entry 1 is present, or when the table is
    empty, and as an object otherwise. A number an object is keyed by comes out
    as text, and a key of any other kind is left out. A function, a handle and anything else
    with no JSON form is left out of an object, and stands as `null` in a list,
    which keeps the entries after it where they were. A number that is not
    finite, and a table that contains itself, raise.
    <!--noref: null-->
  ]],
  params = {
    { name = "value", type = "any", description = "What to write out." },
  },
  returns = { type = "string", description = "The JSON text." },
  examples = {
    [[trx.json.encode({ name = "wolf", ids = { 7, 8 } })
-- {"ids":[7,8],"name":"wolf"}]],
  },
  impl = function(value)
    local out = {}
    write(value, out, {})
    return table.concat(out)
  end,
})

api.define("json.decode", {
  description = [[
    Reads a value out of JSON text, as the game reads its own data files: a
    comment, a trailing comma and an unquoted key are all taken. Text that
    does not parse raises with the line and the column.

    Text nested deeper than 100 levels raises as well.

    An object comes back as a table keyed by name and an array as a table
    keyed from 1, which is what `trx.json.encode` writes back out. `null`
    comes back as `nil`, so a key with `null` reads the same as an absent key,
    and an array entry with `null` ends the list there.
    <!--noref: null-->
  ]],
  params = {
    { name = "text", type = "string", description = "The JSON to read." },
  },
  returns = {
    type = "any",
    nullable = true,
    description = "The decoded value.",
  },
  examples = {
    [[local held = trx.json.decode('{"hp": 6, "seen": ["vilcabamba"]}')
print(held.hp, held.seen[1])]],
  },
  impl = raw.decode,
})

local PATH_PARAM = {
  name = "path",
  type = { "path.Path", "string" },
  description = "Which file, as a path or as the text of one. `trx.path.resolve` finds one "
    .. "the game ships, and `trx.path.config_dir` is for a script's own files.",
}

local function as_path(path)
  return type(path) == "string" and trx.path.new(path) or path
end

api.define("json.read_file", {
  description = [[
    Reads a file as JSON, which is
    `trx.path.Path.read_text` and `trx.json.decode` in one call. A file that
    is not there answers `nil`, and one that does not parse raises with the
    file, the line and the column.

    Every table read from a file carries where it was written, which
    `trx.json.where` reads back. A path outside the directories a script may reach
    raises rather than being read.
  ]],
  params = { PATH_PARAM },
  returns = {
    type = "any",
    nullable = true,
    description = "The decoded value, or `nil` where there is no such file.",
  },
  examples = {
    [[local found = trx.path.resolve("common_config", "weapons.json5")
local weapons = found ~= nil and trx.json.read_file(found) or {}
for key, spec in pairs(weapons) do
  print(key, spec.damage)
end]],
  },
  impl = function(path)
    local file = as_path(path)
    local text = file:read_text()
    if text == nil then
      return nil
    end
    return raw.decode_from(text, tostring(file))
  end,
})

api.define("json.write_file", {
  description = [[
    Writes a value as JSON and saves it to the file, which is
    `trx.json.encode` and `trx.path.Path.write_text` in one call.

    A value `trx.json.encode` has no JSON for raises, and nothing is written.
    A path outside the directories a script may reach raises as well, so
    `trx.path.config_dir` is where a script's own file belongs.
  ]],
  params = {
    PATH_PARAM,
    { name = "value", type = "any", description = "What to write out." },
  },
  examples = {
    [[local kept = trx.path.config_dir / "mymod" / "state.json"
trx.json.write_file(kept, { seen = { "vilcabamba" } })]],
  },
  impl = function(path, value)
    as_path(path):write_text(encode(value))
  end,
})

api.define("json.where", {
  description = [[
    Says where a table read from a file was written: the file, the line and the
    column. Use it when a script must report the source line for bad file
    data. A table a script built itself, and anything that is not a table,
    answer `nil`.
  ]],
  params = {
    {
      name = "value",
      type = "any",
      description = "A table `trx.json.read_file` gave back.",
    },
  },
  returns = {
    type = "string",
    nullable = true,
    description = "Where the table was written, or `nil` without file location data.",
  },
  examples = {
    [[local weapons = trx.json.read_file("weapons.json5")
for key, spec in pairs(weapons) do
  if spec.damage == nil then
    error(("%s: '%s' says no damage"):format(trx.json.where(spec), key))
  end
end]],
  },
  impl = raw.where,
})
