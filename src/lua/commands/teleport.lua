-- Teleports Lara.
--
-- Usages:
--   /tp 35 -1 42          to a sector, as /pos counts them
--   /tp precise 1 2 3     to raw world coordinates
--   /tp room 7, /tp r7    somewhere in a room
--   /tp item 12, /tp i12  to an item by number
--   /tp wolf              to the nearest wolf, then to the next one
--   /tp enemy             to the nearest hostile, and so through the level

trx.locale.declare({
  ["console/cmd/teleport/invalid_room"] = "Invalid room: %d. Valid rooms are 0-%d",
  ["console/cmd/teleport/item"] = "Teleported to item: %d",
  ["console/cmd/teleport/item_fail"] = "Failed to teleport to item: %d",
  ["console/cmd/teleport/object"] = "Teleported to object: %s",
  ["console/cmd/teleport/object_fail"] = "Failed to teleport to object: %s",
  ["console/cmd/teleport/pos"] = "Teleported to position: %.3f %.3f %.3f",
  ["console/cmd/teleport/pos_fail"] = "Failed to teleport to position: %.3f %.3f %.3f",
  ["console/cmd/teleport/room"] = "Teleported to room: %d",
  ["console/cmd/teleport/room_fail"] = "Failed to teleport to room: %d",
  ["console/cmd/tp/help"] = "Teleports Lara to a given position or room number.",
})

local WALL_L = trx.math.WALL_L
local HALF_TURN = 2 * trx.math.DEG_90

-- Lara lands a quarter of a step above what she was sent to, so she is standing
-- on it rather than in it.
local LIFT = WALL_L // 16

-- Which objects belong to a family does not change over a session, so each set
-- is built the once.
local families = {}
local function in_family(family, object_id)
  local set = families[family]
  if set == nil then
    set = {}
    local query = trx.objects.query
    for _, id in ipairs(query[family](query):ids()) do
      set[id] = true
    end
    families[family] = set
  end
  return set[object_id] == true
end

-- A pickup Lara has taken is gone from the level, so an object whose every
-- pickup has been collected is a name that reaches nothing.
local function still_placed(object_id)
  return not in_family("pickup", object_id)
    or trx.items.query:of_object(object_id):visible():count() > 0
end

-- The objects a typed name may reach: everything that stands in the world, as
-- against an inventory icon, an animation or a null placeholder.
local function matchable_objects()
  return trx.objects.query:spawnable():where(still_placed)
end

-- How far Lara stands from the face of something she operates: her own radius,
-- so she comes to a stop beside it rather than inside it.
local FRONT_GAP = 100

-- Which way Lara ends up facing: the same way as something she operates from
-- the front, and away from a pickup or a door, which is where she stands once
-- she has taken it or opened it. A block is pushed and pulled from all four of
-- its sides, so it names none of them and takes whichever one is free.
local FACE_ITEM = 0
local FACE_AWAY = HALF_TURN
local FACE_ANY_SIDE = "any_side"

local function facing_turn(item)
  local object_id = item.object_id
  if in_family("pickup", object_id) or in_family("door", object_id) then
    return FACE_AWAY
  elseif in_family("pushable", object_id) then
    return FACE_ANY_SIDE
  elseif
    in_family("switch", object_id)
    or in_family("receptacle", object_id)
    or object_id == trx.catalog.objects.zipline_handle
  then
    return FACE_ITEM
  else
    return nil
  end
end

-- Which quarter turns off the item's own rotation Lara may be met from: the one
-- front face, or every side.
local function approach_quarters(item)
  local turn = facing_turn(item)
  if turn == FACE_ANY_SIDE then
    return { 0, 1, 2, 3 }
  elseif turn == FACE_ITEM then
    return { 0 }
  end
  return {}
end

-- Where Lara stands to reach a side of an item, and which way she looks from
-- there. A switch set into a wall has its item inside the solid block behind,
-- with only the model reaching through into the room, so the spot comes from
-- the model's own bounding box rather than from the sector the item sits in.
local function beside(item, quarter)
  local bounds = item.bounds
  local reach = ({
    -bounds.min_z,
    -bounds.min_x,
    bounds.max_z,
    bounds.max_x,
  })[quarter + 1] + FRONT_GAP
  local angle = item.rot.y + quarter * trx.math.DEG_90
  return {
    x = math.floor(item.pos.x - reach * trx.math.sin(angle)),
    y = item.pos.y - LIFT,
    z = math.floor(item.pos.z - reach * trx.math.cos(angle)),
  },
    angle
end

-- Somewhere to put Lara down: beside one of the item's faces, or the sector the
-- item stands in for the ones with no side to approach from. A block fills its
-- own sector, so its sides are all it has.
local function has_floor(item)
  for _, quarter in ipairs(approach_quarters(item)) do
    local pos = beside(item, quarter)
    if trx.rooms.floor_height(pos) ~= nil then
      return true
    end
  end
  if facing_turn(item) == FACE_ANY_SIDE then
    return false
  end
  local room = item.room
  return room == nil or room:floor_height(item.pos) ~= nil
end

-- Whether Lara can be put where this item stands.
local function can_teleport_to(item)
  if
    in_family("pickup", item.object_id)
    and (not item.is_visible or item.is_finished or item.room == nil)
  then
    return false
  end
  -- Killed enemies and items taken out of the level.
  if item.is_killed then
    return false
  end
  -- An item in solid geometry has no floor to put Lara on.
  return has_floor(item)
end

-- Lara is put down beside the item, or where it stands for the ones she has no
-- side to approach from, facing whichever way it asks for. A side is looked up
-- without naming a room, since it often lies in the room next door; somewhere
-- with no floor moves nothing, and falls back to the item itself.
local function teleport_to_item(item)
  for _, quarter in ipairs(approach_quarters(item)) do
    local pos, angle = beside(item, quarter)
    if trx.lara.teleport(pos) then
      trx.lara.item.rot = { x = 0, y = angle, z = 0 }
      return true
    end
  end

  local turn = facing_turn(item)
  if turn == FACE_ANY_SIDE then
    return false
  end

  local room = item.room
  local room_num = room ~= nil and room.num or nil
  local pos = { x = item.pos.x, y = item.pos.y - LIFT, z = item.pos.z }
  if not trx.lara.teleport(pos, room_num) then
    return false
  end

  if turn ~= nil then
    trx.lara.item.rot = { x = 0, y = item.rot.y + turn, z = 0 }
  end
  return true
end

local function teleport_to_item_num(item_num)
  local item = trx.items[item_num]
  if
    item == nil
    or item.is_killed
    or item.room == nil
    or not teleport_to_item(item)
  then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/teleport/item_fail", item_num)
  end
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/teleport/item", item_num)
end

-- Somewhere in the room, rather than a spot of the player's choosing: the
-- floor is sampled until one of the samples has somewhere to stand.
local function teleport_to_room(room_num)
  local room = trx.rooms[room_num]
  if room == nil then
    trx.console.log.warning(
      trx.locale.format(
        "console/cmd/teleport/invalid_room",
        room_num,
        #trx.rooms - 1
      )
    )
    return trx.console.Result.FAILURE
  end

  -- A room only a sector across has no room left once the walls are taken off.
  local function between(low, high)
    return low < high and math.random(low, high) or low
  end

  local bounds = room.bounds
  for _ = 1, 100 do
    local pos = {
      x = between(bounds.min_x, bounds.max_x),
      y = bounds.max_y,
      z = between(bounds.min_z, bounds.max_z),
    }
    if trx.lara.teleport(pos, room_num) then
      return trx.console.Result.OK,
        trx.locale.format("console/cmd/teleport/room", room_num)
    end
  end

  return trx.console.Result.FAILURE,
    trx.locale.format("console/cmd/teleport/room_fail", room_num)
end

-- A whole number names a sector, and its middle is where the player means; a
-- precise position is world coordinates, taken as they were typed.
local function teleport_to_pos(x, y, z, precise)
  if not precise then
    if x % 1 == 0 then
      x = x + 0.5
    end
    if z % 1 == 0 then
      z = z + 0.5
    end
  end

  local scale = precise and 1 or WALL_L
  local pos = {
    x = math.floor(x * scale),
    y = math.floor(y * scale),
    z = math.floor(z * scale),
  }
  if not trx.lara.teleport(pos) then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/teleport/pos_fail", x, y, z)
  end
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/teleport/pos", x, y, z)
end

-- The name an object goes by, for reporting where Lara ended up. The player's
-- language first, falling back on the names the engine was built with.
local function object_name(object_id, typed)
  local object = trx.objects[object_id]
  if object == nil then
    return typed
  end
  local names = object.names
  if #names == 0 then
    names = object.default_names
  end
  return names[1] or typed
end

-- The matching item nearest Lara, unless she is already standing on it: then
-- the next one along, so typing the same name again walks her through them all.
local function nearest_item(items)
  local lara_pos = trx.lara.item.pos

  local nearest, nearest_dist, nearest_idx
  for i, item in ipairs(items) do
    local dist = item:distance_to(lara_pos)
    if nearest_dist == nil or dist < nearest_dist then
      nearest, nearest_dist, nearest_idx = item, dist, i
    end
  end

  if nearest == nil or nearest_dist > WALL_L then
    return nearest
  end

  for i = 1, #items do
    local item = items[(nearest_idx + i - 1) % #items + 1]
    if item ~= nearest and item:distance_to(lara_pos) >= WALL_L then
      return item
    end
  end
  return nil
end

local function teleport_to_object(name)
  local wanted = {}
  for _, id in ipairs(matchable_objects():by_name(name):best()) do
    wanted[id] = true
  end

  local items = {}
  for _, item in ipairs(trx.items.query:matches()) do
    if wanted[item.object_id] and can_teleport_to(item) then
      items[#items + 1] = item
    end
  end

  local item = nearest_item(items)
  if item == nil then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/teleport/object_fail", name)
  end

  local reported = object_name(item.object_id, name)
  if not teleport_to_item(item) then
    return trx.console.Result.FAILURE,
      trx.locale.format("console/cmd/teleport/object_fail", reported)
  end
  return trx.console.Result.OK,
    trx.locale.format("console/cmd/teleport/object", reported)
end

-- A number as the player types one: whole or fractional, and negative below the
-- origin.
local NUMBER = "%-?%d*%.?%d+"

local function triplet(text)
  if text == nil then
    return nil
  end
  local x, y, z = text:match(
    "^(" .. NUMBER .. ")%s+(" .. NUMBER .. ")%s+(" .. NUMBER .. ")$"
  )
  if x == nil then
    return nil
  end
  return tonumber(x), tonumber(y), tonumber(z)
end

-- "room 7" and "r7" name the same room, and the same for "item 12" and "i12".
local function tagged_number(text, keyword)
  local num = text:match("^" .. keyword .. "%s+(%-?%d+)$")
    or text:match("^" .. keyword:sub(1, 1) .. "(%-?%d+)$")
  return num ~= nil and tonumber(num) or nil
end

local function teleport(target)
  if target == nil then
    return trx.console.Result.BAD_INVOCATION
  end

  local x, y, z = triplet(target:match("^precise%s+(.*)$"))
  if x ~= nil then
    return teleport_to_pos(x, y, z, true)
  end

  x, y, z = triplet(target)
  if x ~= nil then
    return teleport_to_pos(x, y, z, false)
  end

  local num = tagged_number(target, "item")
  if num ~= nil then
    return teleport_to_item_num(num)
  end

  num = tagged_number(target, "room")
  if num ~= nil then
    return teleport_to_room(num)
  end

  -- A bare number is a room, as it was before rooms had to be said.
  num = target:match("^%-?%d+$")
  if num ~= nil then
    return teleport_to_room(tonumber(num))
  end

  return teleport_to_object(target)
end

trx.console.register({
  name = "tp",
  help = "console/cmd/tp/help",
  args = function(parser)
    parser:rest("target", {
      optional = true,
      suggest = function()
        return matchable_objects():names()
      end,
    })
  end,
  run = function(args)
    -- A flyby has Lara standing still somewhere she cannot act, and teleporting
    -- out of one is how the player takes the level back.
    local flyby = trx.camera.is_flyby_active
    if not flyby and not trx.game.is_playable then
      return trx.console.Result.UNAVAILABLE
    end

    local lara = trx.lara.item
    if lara == nil or lara.hit_points <= 0 then
      return trx.console.Result.UNAVAILABLE
    end

    local result, message = teleport(args.target)
    if result == trx.console.Result.OK and flyby then
      trx.camera.cancel_flyby()
    end
    return result, message
  end,
})
