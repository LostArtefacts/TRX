local cutscenes = require("tr4.cutscenes")

-- The race for the Iris. Von Croy's heavy triggers name LARA_LOST when he
-- reaches the finish first; Lara's own trigger then plays whichever of the two
-- endings happened, and both run on into IRIS_CHAMBER.
local LARA_WON = 7
local LARA_LOST = 8
local IRIS_CHAMBER = 9

trx.events.on_game_start(function(is_save)
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
  trx.objects.switch_type_generic_1.properties.switch_mode =
    trx.items.SwitchMode.HIDDEN_REACH

  -- Von Croy races Lara through the course in the original game. TRX has no
  -- port of that behavior yet, so his items would stand frozen along it.
  cutscenes.hide_items(trx.catalog.objects.von_croy)
end)

trx.events.on_cutscene_trigger(function(cutscene_num)
  -- His trigger only records that he got there first: the engine marks the
  -- number played before asking, and that mark is the whole answer.
  if cutscene_num == LARA_LOST then
    return true
  end
  if cutscene_num ~= LARA_WON then
    return false
  end

  -- Her trigger sits past the finish, so she can cross it again after the
  -- scenes have run.
  if trx.cutscenes.is_played(IRIS_CHAMBER) then
    return true
  end

  if trx.cutscenes.is_played(LARA_LOST) then
    trx.cutscenes.play(LARA_LOST)
  else
    -- She won, so the ending where he did is spent along with it.
    trx.cutscenes.set_played(LARA_LOST, true)
    trx.cutscenes.play(LARA_WON)
  end
  return true
end)

local function von_croy_chat(ranges)
  return {
    actor = 1,
    node = 21,
    ranges = ranges,
    speech_heads = {
      trx.catalog.objects.actor_1_speech_head_1,
      trx.catalog.objects.actor_1_speech_head_2,
    },
  }
end

-- Both endings play at the finish, where the racers themselves stand as level
-- items and the scene brings its own.
local function clear_the_finish()
  cutscenes.hide_items(trx.catalog.objects.animating_3)
  cutscenes.hide_items(trx.catalog.objects.von_croy)
end

cutscenes.register(LARA_WON, {
  on_start = clear_the_finish,
  chat = {
    { lara = true, ranges = { { 675, 877 } } },
    von_croy_chat({ { 396, 654 } }),
  },
  chain = IRIS_CHAMBER,
})

cutscenes.register(LARA_LOST, {
  on_start = clear_the_finish,
  chat = {
    { lara = true, ranges = { { 592, 688 } } },
    von_croy_chat({ { 160, 273 }, { 323, 446 } }),
  },
  chain = IRIS_CHAMBER,
})

-- The Iris chamber, which ends the level. Its cast comes and goes: the statues
-- are not there until the Iris is taken, and Von Croy leaves the frame in the
-- middle of it.
local VON_CROY_ACTOR = 1
local FIRST_STATUE_ACTOR = 3

local function show_statues(visible)
  for actor = FIRST_STATUE_ACTOR, trx.cutscenes.actor_count - 1 do
    trx.cutscenes.set_actor_visible(actor, visible)
  end
end

cutscenes.register(IRIS_CHAMBER, {
  on_start = function()
    show_statues(false)
  end,
  frames = {
    [1300] = function()
      trx.cutscenes.set_actor_visible(VON_CROY_ACTOR, false)
    end,
    [1677] = function()
      trx.cutscenes.set_actor_visible(VON_CROY_ACTOR, true)
    end,
    [3000] = function()
      trx.cutscenes.set_actor_visible(VON_CROY_ACTOR, false)
      show_statues(true)
    end,
  },
  chat = {
    {
      lara = true,
      ranges = {
        { 149, 276 },
        { 297, 348 },
        { 844, 957 },
        { 1180, 1217 },
        { 1249, 1277 },
        { 2231, 2251 },
        { 2714, 2786 },
        { 3019, 3037 },
      },
    },
    von_croy_chat({
      { 12, 24 },
      { 30, 145 },
      { 361, 528 },
      { 539, 556 },
      { 564, 577 },
      { 581, 765 },
      { 781, 839 },
      { 985, 1136 },
      { 1921, 2084 },
      { 2464, 2645 },
    }),
  },
  on_end = trx.game.end_level,
})
