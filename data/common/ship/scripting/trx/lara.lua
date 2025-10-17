local raw = trxc.lara

local lara = {}

function lara.get_item()
  return trx.items[raw.get_item()]
end

trx.lara = lara
