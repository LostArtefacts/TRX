---@meta trx.internal.signatures
-- The build generates this module from the annotations, for strict mode and
-- the seal; see tools/gen_lua_signatures.py.

---@class trx.internal.signatures
---@field functions table<string, table[]>
---@field calls table<string, table[]>
---@field methods table<string, table[]>
---@field types table<string, string>
---@field records table<string, table[]>
---@field properties table<string, any>
---@field containers table<string, any>
---@field members table<string, string[]>
---@field writable table<string, table<string, boolean>>
---@field constants table<string, any>
return {}
