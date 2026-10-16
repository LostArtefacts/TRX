-- Plays the secret music when the game announces a secret that Lara collects.

local M = {}

local function is_secret(object)
  return trx.objects.query
    :secret()
    :where(function(id)
      return id == object
    end)
    :count() > 0
end

-- Listens for the announcements. Called once, as the shipped scripts load.
function M.setup()
  trx.events.on_show_pickup(function(object)
    if not is_secret(object) then
      return
    end
    -- The fix plays the music over the level's own, rather than in its place.
    local mode = trx.config.get("audio.fix_secrets_killing_music")
        and trx.music.PlayMode.OVERLAY
      or trx.music.PlayMode.ONCE
    trx.music.play(trx.catalog.music.SECRET, { mode = mode })
  end)
end

return M
