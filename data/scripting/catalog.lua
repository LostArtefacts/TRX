local raw = trxc.catalog

local catalog = {
  objects = raw.objects,
}

trx.catalog = catalog
