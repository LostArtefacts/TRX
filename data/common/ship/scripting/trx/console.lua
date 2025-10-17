local raw = trxc.console

local console = {}

function console.log(...)
  return raw.log(...)
end

function console.clear()
  return raw.clear()
end

function console.eval(cmd, opts)
  return raw.eval(cmd, opts)
end

trx.console = console
