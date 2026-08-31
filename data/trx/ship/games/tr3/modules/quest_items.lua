local quest_items = {}

-- A quest item stays where a carrier drops it rather than sliding to the
-- middle of the sector, and goes on rotating and glowing once it lands.
function quest_items.initialise()
  trx.events.on_game_start(function()
    for _, id in ipairs(trx.objects.query:quest():ids()) do
      trx.objects[id].properties.snap_to_sector = false
      trx.objects[id].properties.keep_simulated = true
    end
  end)
end

return quest_items
