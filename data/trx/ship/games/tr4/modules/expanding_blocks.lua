local M = {}

M.initialise = function(block_object, shake, activate)
  block_object.properties.shake_camera = shake
  if not activate then
    return
  end

  local query = trx.items.query:where(function(id, item)
    return trx.objects[item.object_id] == block_object
  end)
  for _, item in ipairs(query:matches()) do
    item:trigger()
  end
end

return M
