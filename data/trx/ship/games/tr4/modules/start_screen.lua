-- The screen that the game shows once as it starts: the logo, with the
-- copyright notice under it, on black. It is the legal picture of the title
-- sequence, which has no image of its own, so the Legal screens setting hides
-- it. Every other picture draws as usual.
--
-- The game script requires this.
--
--   require("tr4.start_screen")

local logo = require("tr4.logo")
local picture = require("common.ui.screens.picture")

-- Where the original's start screen puts the logo and the notice, as parts of
-- the canvas height.
local LOGO_WIDTH = 384
local LOGO_Y = 0.32
local NOTICE = {
  { text = trx.locale.text("general/legal/tr4_core"), y = 0.574 },
  { text = trx.locale.text("general/legal/tr4_eidos"), y = 0.633 },
}

local function centered_at(y)
  return function(w, h)
    return (trx.ui.canvas.width - w) / 2, trx.ui.canvas.height * y - h / 2
  end
end

trx.ui.screens.define(trx.ui.Screen.PICTURE, function(ctx)
  local main = picture.open(ctx)
  if not ctx.is_legal or ctx.background ~= nil then
    return main
  end

  ctx:push({
    root = trx.ui.widgets.Image({ path = logo.path, w = LOGO_WIDTH }),
    place = centered_at(LOGO_Y),
  })
  for _, line in ipairs(NOTICE) do
    ctx:push({
      root = trx.ui.widgets.Label({ text = line.text }),
      place = centered_at(line.y),
    })
  end
  return main
end, { override = true })
