-- Lists the tracks, shows the soundtrack's state, stops it, or plays a track.
--
-- Usages:
--   /music         list the available track ids
--   /music status  report what is playing
--   /music stop    stop all music
--   /music 5       play track 5

local function available_ids()
  local ids = {}
  for id in pairs(trx.music.tracks) do
    ids[#ids + 1] = id
  end
  return ids
end

local function overlay_ids()
  local ids = {}
  for _, stream in ipairs(trx.music.streams) do
    if stream:is_valid() and stream.mode == trx.music.PlayMode.OVERLAY then
      ids[#ids + 1] = stream.track_id
    end
  end
  return ids
end

local function log_available()
  local ids = available_ids()
  if #ids == 0 then
    return
  end
  trx.console.log.info(
    trx.locale.format(
      "console/cmd/music/available_tracks",
      trx.strings.collapse_ranges(ids)
    )
  )
end

local function show_status()
  local current = trx.music.current_track
  if current == nil then
    trx.console.log.info(trx.locale.get("console/cmd/music/current_none"))
  else
    trx.console.log.info(
      trx.locale.format("console/cmd/music/current", current.id)
    )
  end

  local looped = trx.music.looped_track
  if current ~= nil and looped ~= nil and current.id ~= looped.id then
    trx.console.log.info(
      trx.locale.format("console/cmd/music/deferred_ambient", looped.id)
    )
  end

  local overlay = overlay_ids()
  if #overlay > 0 then
    trx.console.log.info(
      trx.locale.format(
        "console/cmd/music/overlay",
        trx.strings.collapse_ranges(overlay)
      )
    )
  end
end

trx.console.register({
  name = "music",
  help = "console/cmd/music/help",
  run = function(args)
    if args == "" then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/music/available_tracks",
          trx.strings.collapse_ranges(available_ids())
        )
    end

    local lowered = args:lower()
    if lowered == "status" then
      show_status()
      return trx.console.Result.OK
    end

    if lowered == "stop" then
      trx.music.stop()
      return trx.console.Result.OK, trx.locale.get("console/cmd/music/stopped")
    end

    local num = tonumber(args)
    if num == nil or num % 1 ~= 0 then
      return trx.console.Result.BAD_INVOCATION
    end

    if num == 0 or num == -1 then
      trx.music.stop()
      return trx.console.Result.OK, trx.locale.get("console/cmd/music/stopped")
    end

    local track = trx.music.tracks[num]
    if track == nil then
      trx.console.log.error(trx.locale.get("console/cmd/music/invalid_track"))
      log_available()
      return trx.console.Result.OK
    end

    track:play()
    return trx.console.Result.OK,
      trx.locale.format("console/cmd/music/track", num)
  end,
})
