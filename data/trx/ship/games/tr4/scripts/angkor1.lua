local cutscenes = require("tr4.cutscenes")

trx.events.on_game_start(function(is_save)
  trx.objects.animating_13.properties.collidable = false
  trx.objects.animating_14.properties.collidable = false
  trx.objects.animating_15.properties.collidable = false
  trx.objects.animating_16.properties.collidable = false
  trx.lara.holsters_visible = trx.lara.has_pistol_weapon

  -- Von Croy walks the level beside Lara in the original game. TRX has no
  -- port of his guide behavior yet, so his items would stand frozen next to
  -- the cutscene actor wearing the same face.
  cutscenes.hide_items(trx.catalog.objects.von_croy)
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
  has_backpack = trx.cutscenes.is_played(BACKPACK_CUTSCENE)
  dress()
end)

-- The trigger marks the scene played before it starts, so the frame is what
-- answers once it is on screen.
trx.events.after_control(function()
  if
    not has_backpack
    and trx.cutscenes.current == BACKPACK_CUTSCENE
    and trx.cutscenes.frame_num >= BACKPACK_FRAME
  then
    has_backpack = true
    dress()
  end
end)

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

trx.events.on_cutscene_start(function(cutscene_num)
  if cutscene_num == BACKPACK_CUTSCENE then
    -- The scene starts while Lara is crawling, and the crawlspace she triggers
    -- it from is no place to stand up in. The original engine carries her out
    -- to the floor beyond it.
    trx.cutscenes.set_lara_return({ x = 100938, y = 768, z = 58040 }, -32552)
  elseif cutscene_num == ENTRANCE_CUTSCENE then
    -- The scene plays at the temple entrance and leaves her facing the way in.
    trx.cutscenes.set_lara_return({ x = 5632, y = 1280, z = 86528 }, 28987)
  end
end)

-- Lara and Von Croy arrive at the temple entrance and talk it over.
cutscenes.register(ENTRANCE_CUTSCENE, {
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
