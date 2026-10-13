-- Combining an entry in the inventory ring: what it combines with, in a row,
-- slid along to the one wanted, in a frame that names it.
--
-- Usage:
--   require("common.ui.screens.combine").setup()

local M = {}

local L = trx.locale.get
local widgets = trx.ui.widgets
local Role = trx.input.Role

-- Fallback text for the screen's own strings. The strings files and their
-- translations take precedence.
trx.locale.declare({
  ["general/actions/cancel"] = "Cancel",
})

-- The room the frame keeps for the row.
local ROW_WIDTH = 260.0
local ROW_HEIGHT = 150.0
local FRAME_PAD = 10.0

-- The box the entry in the middle is drawn in, and how far apart the middles
-- of two neighbours stand.
local MODEL_WIDTH = 100.0
local MODEL_HEIGHT = 120.0
local SPACING = 90.0

local SPIN_PER_TICK = 4 * trx.math.DEG_1
-- Every mesh of an object, for one the ring states no selection for.
local ALL_MESHES = 0xFFFFFFFF

-- The model of an entry as the ring poses it, turning while it is in the
-- middle.
local function model(object, index, selected)
  local entry = trx.inventory_ring.item(object)
  return widgets.Model({
    object = object,
    w = MODEL_WIDTH,
    h = MODEL_HEIGHT,
    rot_x = entry and entry.base_rot_x or 0,
    rot_y = entry and entry.y_rot_sel + entry.base_rot_y or 0,
    rot_z = entry and entry.base_rot_z or 0,
    mesh_mask = entry and entry.meshes_drawn or ALL_MESHES,
    spin = selected:map(function(at)
      return at == index and SPIN_PER_TICK or 0
    end),
  })
end

local function name_of(object)
  local info = object ~= nil and trx.objects[object] or nil
  return info ~= nil and info.name or ""
end

local function dialog(state)
  local selected = state.selected
  local partners = state.partners
  state.row = widgets.Carousel({
    items = partners,
    selected = selected,
    build = function(object, index)
      return model(object, index, selected)
    end,
    w = ROW_WIDTH,
    h = ROW_HEIGHT,
    spacing = SPACING,
  })
  return widgets.Frame({
    style = trx.ui.FrameStyle.OUTLINE,
    child = widgets.Pad({
      x = FRAME_PAD,
      y = FRAME_PAD,
      child = widgets.Stack({
        spacing = FRAME_PAD,
        align = trx.ui.HAlign.CENTER,
        children = {
          state.row,
          widgets.Label({
            text = selected:map(function(at)
              return name_of(partners[at])
            end),
          }),
        },
      }),
    }),
  })
end

-- Whether the entry has come to rest, which is when the row opens.
local function is_open()
  local anim = trx.inventory_ring.selection_anim()
  return anim ~= nil and anim.frame == anim.goal_frame
end

local function begin(ctx, state, layer)
  state.partners = trx.inventory.combine_partners(ctx.object)
  if #state.partners == 0 then
    ctx:cancel()
    return
  end
  state.selected = trx.signal.new(1)
  layer:set_root(dialog(state))
  -- The frame names the entry in the middle, so the ring names nothing. The
  -- caption goes before the frame first shows, or the frame would move once
  -- the caption gave up its room.
  trx.overlay.set_caption()
  trx.sound.play(trx.catalog.samples.MENU_SPININ)
  trx.inventory_ring.set_input_hints({
    { input = Role.MENU_CONFIRM, text = L("general/actions/combine_item") },
    { input = Role.MENU_BACK, text = L("general/actions/cancel") },
  })
end

local function turn(state, step)
  local count = #state.partners
  state.selected:set((state.selected:get() - 1 + step) % count + 1)
  trx.sound.play(trx.catalog.samples.MENU_ROTATE)
end

local function control(ctx, state, layer, keys)
  if state.partners == nil then
    if is_open() then
      begin(ctx, state, layer)
    end
    return
  end
  -- The ring names the entry it holds again each tick.
  trx.overlay.set_caption()

  if state.row.is_sliding() then
    return
  end
  local many = #state.partners > 1
  if many and keys:held(Role.MENU_RIGHT) then
    turn(state, 1)
  elseif many and keys:held(Role.MENU_LEFT) then
    turn(state, -1)
  elseif keys:pressed(Role.MENU_CONFIRM) then
    trx.inventory.combine(ctx.object, state.partners[state.selected:get()])
    ctx:confirm()
  elseif keys:pressed(Role.MENU_BACK) then
    -- The ring sounds the entry going away as it puts it away.
    ctx:cancel()
  end
end

---@param ctx trx.ui.EntryContext
local function open(ctx)
  local state = {}
  return ctx:push({
    -- Nothing shows until the entry comes to rest and the row opens.
    root = widgets.Stack({ children = {} }),
    on_input = function(layer, keys)
      control(ctx, state, layer, keys)
    end,
  })
end

function M.setup()
  trx.ui.screens.define(trx.ui.Screen.COMBINE, open)
end

return M
