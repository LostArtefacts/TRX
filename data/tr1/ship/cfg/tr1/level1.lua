local O_BAT = 9;

TRX.Events.Listen(TRX.EventType.LEVEL_LOAD, function(level)
  TRX.Log.Info("hello from caves")
  local lara = TRX.Lara.GetItem()
  TRX.Log.Info(("Lara position: x=%d, y=%d, z=%d"):format(lara.pos.x, lara.pos.y, lara.pos.z))
  lara.hit_points = 500

  local count = TRX.Items.Count()
  for i = 1, count do
    local item = TRX.Items.Get(i)
    if item.object_id == O_BAT then
      item.hit_points = 20
      item.max_hit_points = 20
    end
  end
end)

TRX.Events.Listen(TRX.EventType.PICKUP, function(pickup_item)
  print("Lara pick up item_num=" .. tostring(pickup_item))
  local lara = TRX.Lara.GetItem()
  lara.hit_points = lara.hit_points - 50
  lara.pos = {
    x = 73.5 * 1024,
    y = 3 * 1024,
    z = 3.5 * 1024,
  }
end)
