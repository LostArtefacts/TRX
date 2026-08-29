require("common.overlay")
require("tr4.fog")
-- How far along a level Lara has got, which its guides follow. TR4 marks the
-- points with two flip effects and carries the number in the trigger's timer.
local LOCATION_EFFECT = 30
local LOCATION_PAD_EFFECT = 45

trx.events.on_flip_effect(LOCATION_EFFECT, function(timer)
  trx.waypoints.current = timer
end)

trx.events.on_flip_effect(LOCATION_PAD_EFFECT, function(timer)
  trx.waypoints.pad = timer
end)
