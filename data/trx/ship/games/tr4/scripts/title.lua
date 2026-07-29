-- The title screen alternates flyby sequences with cutscenes: a trigger along
-- a flyby's path starts one, and the cutscene hands over to the next sequence
-- when it ends. The last one wraps to the first and lets every scene in the
-- cycle run again, so it repeats for as long as the menu is up. Each step
-- pairs the cutscene a trigger starts with the flyby that follows it.
local CYCLE = {
  { cutscene = 28, next_flyby = 2 },
  { cutscene = 29, next_flyby = 3 },
  { cutscene = 30, next_flyby = 1 },
}

-- title.tr4 numbers its flyby sequences from 1; sequence 0 has no cameras.
local FIRST_FLYBY = 1

local function cycle_step(num)
  for index, step in ipairs(CYCLE) do
    if step.cutscene == num then
      return step, index
    end
  end
  return nil
end

-- The doors the flyby triggers open would otherwise stay open, and every later
-- pass would begin on a room already unlocked. Rewinding them as the cycle
-- opens gives each pass the same picture to start from, which is what the
-- original engine does here too.
local DOORS = trx.catalog.objects.animating_6

local function rewind_doors()
  for _, item in ipairs(trx.items.query:of_object(DOORS):matches()) do
    item.anim = 0
    item.frame = 0
    item.trigger_mask = 0
  end
end

-- Lara is one of a cutscene's actors, and has no business being on screen
-- between them. The level script runs before the level exists, so she is only
-- ever reached from a handler.
local function set_lara_visible(visible)
  local item = trx.lara.item
  if item ~= nil then
    item.mesh_bits = visible and 0xFFFFFFFF or 0
  end
end

trx.events.on_title_start(function()
  trx.cutscenes.forget_played()
  set_lara_visible(false)
  trx.camera.play_flyby(FIRST_FLYBY)
end)

-- The menu always needs a camera of its own, so a sequence that runs out
-- without a cutscene to follow it plays again.
trx.events.on_flyby_end(function(sequence)
  trx.camera.play_flyby(sequence)
end)

trx.events.on_cutscene_start(function(num)
  set_lara_visible(true)
  if num == CYCLE[1].cutscene then
    rewind_doors()
  end
end)

trx.events.on_cutscene_end(function(num)
  set_lara_visible(false)

  local step, index = cycle_step(num)
  if step == nil then
    return
  end
  if index == #CYCLE then
    trx.cutscenes.forget_played()
  end
  trx.camera.play_flyby(step.next_flyby)
end)
