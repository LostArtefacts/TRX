local raw = trxc.assault
local Track = raw.Track

trx.assault = {
  Track = Track,
  stats = {
    add_record = raw.stats.record,
    remove_record = raw.stats.remove,
    list_records = raw.stats.list,
  },
}

function trx.assault.start(track)
  raw.start(track or Track.COURSE)
end

function trx.assault.stop(track)
  raw.stop(track or Track.COURSE)
end

function trx.assault.reset(track)
  raw.reset(track or Track.COURSE)
end
