local raw = trxc.catalog

local catalog = {
  objects = raw.objects,
  flip_effects = raw.flip_effects,
}

trx.catalog = catalog
