local cutscenes = require("tr4.cutscenes")

trx.events.on_game_start(function(is_save)
  -- The level he guides Lara through: elsewhere he runs his markers alone.
  trx.objects.von_croy.properties.guides_lara = true
  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon
  trx.objects.scaled_spikes.properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.ONE_SHOT
  trx.items[13].properties.scaled_spikes_mode =
    trx.items.ScaledSpikesMode.EXTENDED
end)

local BACKPACK_CUTSCENE = 5
local ENTRANCE_CUTSCENE = 6
local BACKPACK_FRAME = 1350
local YOUNG_OUTFIT = "tr4_young"
local BARE_TORSO_MESH = 7

-- Young Lara carries no backpack until she picks one up part-way through the
-- cutscene the crawlspace triggers. The outfit she wears carries the torso she
-- ends up with, and the level carries the one she starts in, so the level's is
-- what she wears until the scene hands the backpack over.
local has_backpack = false

local function dress(outfit)
  outfit = outfit or trx.lara.outfit
  if outfit ~= YOUNG_OUTFIT or has_backpack then
    trx.lara.clear_mesh(trx.lara.Mesh.TORSO)
  else
    trx.lara.set_mesh(
      trx.lara.Mesh.TORSO,
      trx.catalog.objects.lara_skin,
      BARE_TORSO_MESH
    )
  end
end

-- Nothing records whether she is carrying it: the cutscene having run says so,
-- and that is what a savegame already remembers.
trx.events.on_game_start(function()
  has_backpack = trx.cutscenes[BACKPACK_CUTSCENE].is_played
  dress()
end)

local function give_backpack()
  if not has_backpack then
    has_backpack = true
    dress()
  end
end

-- The trigger marks the scene played before it starts, so the frame is what
-- answers once it is on screen. A scene the player skips never reaches that
-- frame, and its end hands the backpack over instead.
cutscenes.register(BACKPACK_CUTSCENE, {
  frames = { [BACKPACK_FRAME] = give_backpack },
  on_end = give_backpack,
})

-- The outfit being put on, rather than the one still on her: a watcher is told
-- before the skin is applied, and applying it is what reads the override. The
-- setting reads as an empty string while the player has chosen nothing, and
-- the engine dresses her in the one the level asks for.
trx.config.on_change("visuals.lara_outfit", function(outfit)
  if outfit == nil or outfit == "" then
    outfit = trx.game.current_level.lara_outfit
  end
  dress(outfit)
end)

trx.events.on_cutscene_start(function(cutscene)
  if cutscene.num == BACKPACK_CUTSCENE then
    -- The scene starts while Lara is crawling, and the crawlspace she triggers
    -- it from is no place to stand up in. The original engine carries her out
    -- to the floor beyond it.
    trx.cutscenes.set_lara_return({ x = 100938, y = 768, z = 58040 }, -32552)
  elseif cutscene.num == ENTRANCE_CUTSCENE then
    -- The scene plays at the temple entrance and leaves her facing the way in.
    trx.cutscenes.set_lara_return({ x = 5632, y = 1280, z = 86528 }, 28987)
  end
end)

-- Lara and Von Croy arrive at the temple entrance and talk it over. No floor
-- trigger names this one; it opens the level.
cutscenes.play_on_start(ENTRANCE_CUTSCENE)

cutscenes.register(ENTRANCE_CUTSCENE, {
  on_start = function()
    cutscenes.dress_von_croy(1)
  end,
  chat = {
    { lara = true, ranges = { { 257, 345 } } },
    {
      actor = 1,
      node = 21,
      ranges = { { 6, 209 } },
      speech_heads = {
        trx.catalog.objects.actor_1_speech_head_1,
        trx.catalog.objects.actor_1_speech_head_2,
      },
    },
  },
})

-- The caption the level opens with, which the level's strings carry.
require("common.legend").setup(function()
  return trx.locale.get("general/legend")
end)
