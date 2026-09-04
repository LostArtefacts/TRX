local api = trx.api

api.module("json", {
  order = 35,
  title = "JSON",
  description = "Writing Lua values out as JSON. The API dump the reference is generated from "
    .. "goes through this, so what a script writes out is encoded the way the engine's own data "
    .. "is.",
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

-- A table with no keys at all is written as an empty list: a declaration that
-- holds none of something reads as none of it rather than as an object with
-- nothing in it.
local function is_list(value)
  return value[1] ~= nil or next(value) == nil
end

local function write(value, out)
  local kind = type(value)
  if value == nil then
    out[#out + 1] = "null"
  elseif kind == "boolean" or kind == "number" then
    out[#out + 1] = tostring(value)
  elseif kind == "string" then
    out[#out + 1] = quoted(value)
  elseif kind == "table" and is_list(value) then
    out[#out + 1] = "["
    for i, held in ipairs(value) do
      if i > 1 then
        out[#out + 1] = ","
      end
      write(held, out)
    end
    out[#out + 1] = "]"
  elseif kind == "table" then
    local keys = {}
    for key in pairs(value) do
      keys[#keys + 1] = key
    end
    table.sort(keys)
    out[#out + 1] = "{"
    for i, key in ipairs(keys) do
      if i > 1 then
        out[#out + 1] = ","
      end
      out[#out + 1] = quoted(tostring(key))
      out[#out + 1] = ":"
      write(value[key], out)
    end
    out[#out + 1] = "}"
  end
end

api.define("json.encode", {
  description = [[
    Writes a value out as JSON, on one line. Keys come out in sorted order, so
    the same value encodes the same way twice and a file that is committed and
    diffed only moves when what it holds does.

    A table is written as a list where it holds entry 1, or holds nothing at
    all, and as an object otherwise. A function, a handle and anything else
    with no JSON of its own is left out.
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
    write(value, out)
    return table.concat(out)
  end,
})
