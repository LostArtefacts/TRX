local raw = trxc.log

local log = {}

function log.info(...)
  raw.generic(raw.LogLevel.INFO, ...)
end
function log.warn(...)
  raw.generic(raw.LogLevel.WARNING, ...)
end
function log.warning(...)
  raw.generic(raw.LogLevel.WARNING, ...)
end
function log.error(...)
  raw.generic(raw.LogLevel.ERROR, ...)
end
function log.debug(...)
  raw.generic(raw.LogLevel.DEBUG, ...)
end

trx.log = log
