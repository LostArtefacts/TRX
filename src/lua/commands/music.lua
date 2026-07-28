-- Lists the tracks, shows the soundtrack's state, stops it, or plays a track.
--
-- Usages:
--   /music         list the available track ids
--   /music status  report what is playing
--   /music stop    stop all music
--   /music 5       play track 5

trx.locale.declare({
  ["console/cmd/music/available_tracks"] = "Available music tracks: %s",
  ["console/cmd/music/current"] = "Current track: %d",
  ["console/cmd/music/current_none"] = "Current track: none",
  ["console/cmd/music/deferred_ambient"] = "Deferred ambient: %d",
  ["console/cmd/music/help"] = "Shows the current music state, stops all music, or plays a music track with the given id.",
  ["console/cmd/music/id_help"] = "play the track with this id",
  ["console/cmd/music/invalid_track"] = "Invalid music track",
  ["console/cmd/music/overlay"] = "Overlay track(s): %s",
  ["console/cmd/music/status_help"] = "report what is playing",
  ["console/cmd/music/stop_help"] = "stop all music",
  ["console/cmd/music/stopped"] = "Music stopped",
  ["console/cmd/music/track"] = "Playing music track %d",
})

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
  args = function(parser)
    -- A track number, or one of the keywords; run tells them apart by type.
    parser:any_of("what", {
      {
        choices = { "status" },
        metavar = "status",
        help = "console/cmd/music/status_help",
      },
      {
        choices = { "stop" },
        metavar = "stop",
        help = "console/cmd/music/stop_help",
      },
      {
        type = "integer",
        metavar = "id",
        help = "console/cmd/music/id_help",
      },
    }, { optional = true })
  end,
  run = function(args)
    if args.what == nil then
      return trx.console.Result.OK,
        trx.locale.format(
          "console/cmd/music/available_tracks",
          trx.strings.collapse_ranges(available_ids())
        )
    end

    if args.what == "status" then
      show_status()
      return trx.console.Result.OK
    end

    if args.what == "stop" then
      trx.music.stop()
      return trx.console.Result.OK, trx.locale.get("console/cmd/music/stopped")
    end

    local num = args.what
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
