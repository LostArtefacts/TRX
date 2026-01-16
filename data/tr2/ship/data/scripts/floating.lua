trx.events.after_level_file(function(level)
  trx.objects.swap_mesh(trx.catalog.objects.O_SECRET_2_OPTION, trx.catalog.objects.O_SECRET_3_OPTION, 0, 0)
end)
