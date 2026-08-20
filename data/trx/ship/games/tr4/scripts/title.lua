-- The title screen alternates flyby sequences with cutscenes: a trigger along
-- a flyby's path starts one, and the cutscene hands over to the next sequence
-- when it ends. The last one wraps to the first and lets every scene in the
-- cycle run again, so it repeats for as long as the menu is up. Each step
-- pairs the cutscene a trigger starts with the flyby that follows it.
local CYCLE = {
  { cutscene_num = 28, next_flyby_num = 2 },
  { cutscene_num = 29, next_flyby_num = 3 },
  { cutscene_num = 30, next_flyby_num = 1 },
}

-- title.tr4 numbers its flyby sequences from 1; sequence 0 has no cameras.
local FIRST_FLYBY_NUM = 1

local function cycle_step(cutscene_num)
  for index, step in ipairs(CYCLE) do
    if step.cutscene_num == cutscene_num then
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
    item.anim_num = 0
    item.frame_num = 0
    item.trigger_mask = 0
  end
end

-- A trigger a flyby crosses antitriggers one of the ground flames, and the
-- level holds none that lights it again, so every pass after the first is
-- missing it. Putting the trigger back as it fires leaves the flame burning,
-- which is what dropping the trigger from the level data would do.
local FLAMES = trx.catalog.objects.flame_emitter_tr4_ground
trx.events.on_trigger(function(item, trigger)
  if
    item.object_id == FLAMES
    and trigger.type == trx.items.TriggerType.ANTITRIGGER
  then
    item.is_reversed = true
  end
end)

-- Places blood where the mummy lands on the spikes.
local BLOOD_CUTSCENE_NUM = CYCLE[3].cutscene_num
local BLOOD_FIRST_FRAME = 349
local BLOOD_LAST_FRAME = 357
local BLOOD_Z = 76209

trx.cutscenes[BLOOD_CUTSCENE_NUM]:on_frame(function(_, frame_num)
  if
    frame_num < BLOOD_FIRST_FRAME
    or frame_num > BLOOD_LAST_FRAME
    or frame_num % 2 == 0
  then
    return
  end

  trx.fx.blood({
    pos = {
      x = 6799 - trx.random.draw:randint(0, 255),
      y = trx.random.draw:randint(0, 511) - 768,
      z = BLOOD_Z,
    },
    strength = 7,
  })
end)

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
  trx.camera.play_flyby(FIRST_FLYBY_NUM)
end)

-- The menu always needs a camera of its own, so a sequence that runs out
-- without a cutscene to follow it plays again.
trx.events.on_flyby_end(function(sequence_num)
  trx.camera.play_flyby(sequence_num)
end)

trx.events.on_cutscene_start(function(cutscene)
  set_lara_visible(true)
  if cutscene.num == CYCLE[1].cutscene_num then
    rewind_doors()
  end
end)

trx.events.on_cutscene_end(function(cutscene)
  set_lara_visible(false)

  local step, index = cycle_step(cutscene.num)
  if step == nil then
    return
  end
  if index == #CYCLE then
    trx.cutscenes.forget_played()
  end
  trx.camera.play_flyby(step.next_flyby_num)
end)
