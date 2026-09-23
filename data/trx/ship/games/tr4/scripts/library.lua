local SERPENT_ROOM = 25
trx.store.level.serpent_puzzle_solved = false

local serpent_puzzle = {
  flames = nil,

  initialise = function(self)
    local query = trx.items.query:where(function(id, item)
      return item.room_num == SERPENT_ROOM
        and item.object_id == trx.catalog.objects.flame_emitter_tr4_ground
    end)

    self.flames = query:matches()

    for _, item in ipairs(self.flames) do
      item:on_trigger(function(item, trigger)
        self:on_flame_trigger(item, trigger)
      end)
    end
  end,

  on_flame_trigger = function(self, item, trigger)
    if trx.store.level.serpent_puzzle_solved then
      return
    end

    for _, item in ipairs(self.flames) do
      if item.trigger_mask ~= 31 then
        return
      end
    end

    self:on_solved()
  end,

  on_solved = function(self)
    local query = trx.items.query:where(function(id, item)
      return item.room_num == SERPENT_ROOM
        and (
          item.object_id == trx.catalog.objects.raising_block_1
          or item.object_id == trx.catalog.objects.raising_block_2
        )
    end)

    for _, item in ipairs(query:matches()) do
      item:trigger()
    end
    trx.store.level.serpent_puzzle_solved = true
  end,
}

trx.events.on_game_start(function()
  trx.items[12].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[135].properties.pickup_mode = trx.items.PickupMode.PLINTH_LOW
  trx.items[79].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[140].properties.pickup_mode = trx.items.PickupMode.CROWBAR
  trx.items[141].properties.pickup_mode = trx.items.PickupMode.CROWBAR

  local expanding_blocks = require("tr4.expanding_blocks")
  expanding_blocks.initialise(trx.objects.raising_block_1, true, false)
  expanding_blocks.initialise(trx.objects.raising_block_2, true, false)

  serpent_puzzle:initialise()
end)
