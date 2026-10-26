-- The settings dialogs of the inventory ring: the graphics, the sound and the
-- gameplay settings, each a dialog of tabs, and the controls. A menu outside
-- the ring lists them through `dialogs`.

local color_editor = require("common.ui.settings.color_editor")
local controls = require("common.ui.settings.controls")
local dialog = require("common.ui.settings.dialog")
local editor = require("common.ui.settings.editor")
local handlers = require("common.ui.settings.handlers")
local preset_tab = require("common.ui.settings.preset_tab")
local ring = require("common.ui.ring")
local tabs = require("common.ui.settings.tabs")

local M = {}

trx.locale.declare({
  ["general/settings/controls/title"] = "Controls",
})

-- What each dialog holds: its title, and its tabs, each the settings tab it
-- shows or the presets.
local DIALOGS = {
  graphics = function()
    return trx.locale.text("general/settings/graphic_settings/title"),
      {
        {
          trx.locale.text("general/settings/graphic_settings/tabs/visuals"),
          "graphic_visuals",
        },
        {
          trx.locale.text("general/settings/graphic_settings/tabs/ui"),
          "graphic_ui",
        },
        {
          trx.locale.text("general/settings/graphic_settings/tabs/stats"),
          "graphic_ui_stats",
        },
        {
          trx.locale.text("general/settings/graphic_settings/tabs/bars"),
          "graphic_ui_bars",
        },
        {
          trx.locale.text("general/settings/graphic_settings/tabs/rendering"),
          "graphic_rendering",
        },
      }
  end,
  sound = function()
    return trx.locale.text("general/settings/sound/title"),
      {
        {
          trx.locale.text("general/settings/sound/tabs/volume"),
          "sound_volume",
        },
        { trx.locale.text("general/settings/sound/tabs/misc"), "sound_misc" },
      }
  end,
  gameplay = function()
    return trx.locale.text("general/settings/gameplay/title"),
      {
        {
          trx.locale.text("general/settings/gameplay/tabs/general"),
          "gameplay_general",
        },
        {
          trx.locale.text("general/settings/gameplay/tabs/controls"),
          "gameplay_controls",
        },
        {
          trx.locale.text("general/settings/gameplay/tabs/mods"),
          "gameplay_mods",
        },
        {
          trx.locale.text("general/settings/gameplay/tabs/fixes"),
          "gameplay_fixes",
        },
        { trx.locale.text("general/settings/gameplay/tabs/presets") },
      }
  end,
}

-- The rows that scripts declared, by the tab that each sits on.
local function declared_by_tab()
  handlers.add_declared()
  local result = {}
  for _, entry in ipairs(trx.config.declared_rows()) do
    local tab = entry.row.tab
    if not tabs.exists(tab) then
      trx.log.warn(
        ("the setting %s names no settings tab: %s"):format(entry.key, tab)
      )
    else
      result[tab] = result[tab] or {}
      table.insert(result[tab], {
        key = entry.key,
        before = entry.row.before,
        after = entry.row.after,
      })
    end
  end
  return result
end

-- Puts the entry away, playing its animation back to the start as it goes.
local function close(ctx)
  if ring.selection_anim() ~= nil then
    ring.animate_selection(0, -1)
  end
  ctx:cancel()
end

-- Builds one of the dialogs, opening anything it opens over itself through
-- `push`.
local function build_dialog(which, push)
  local heading, specs = DIALOGS[which]()
  local declared = declared_by_tab()
  return dialog.new({
    title = heading,
    tabs = function(self)
      local result = {}
      for i, spec in ipairs(specs) do
        local title, name = spec[1], spec[2]
        if name == nil then
          result[i] = preset_tab.new({
            title = title,
            editing = self.editing,
            changed = self.changed,
            push = push,
          })
        else
          result[i] = editor.new({
            title = title,
            keys = tabs.arrange(name, declared[name] or {}),
            editing = self.editing,
            width = self.width,
            dialog_width = self.dialog_width,
            changed = self.changed,
            layout = self.layout,
            push = push,
            edit_color = function(key)
              color_editor.open(push, key)
            end,
          })
        end
      end
      return result
    end,
  })
end

-- Where the player left each dialog, which it opens on again until the level
-- ends, as the original games do.
---@type table<string, common.ui.settings.Dialog.Place>
local places = {}

-- Returns a function that pushes a layer over a dialog, and a signal that
-- holds whether the dialog shows. Without frames, nothing sets a layer apart
-- from the dialog under it, so the dialog hides until the layer closes.
local function covering(ctx)
  local shown = trx.signal.new(true)
  local count = 0
  local function follow()
    shown:set(count == 0 or trx.ui.frames_shown)
  end
  local function push(settings)
    local copy = {}
    for key, value in pairs(settings) do
      copy[key] = value
    end
    copy.on_close = function(layer)
      count = count - 1
      follow()
      if settings.on_close ~= nil then
        settings.on_close(layer)
      end
    end
    count = count + 1
    follow()
    return ctx:push(copy)
  end
  return push, shown
end

local function opener(which)
  ---@param ctx trx.ui.EntryContext
  return function(ctx)
    local push, shown = covering(ctx)
    local settings_dialog = build_dialog(which, push)
    if places[which] ~= nil then
      settings_dialog:restore(places[which])
    end
    local resting = ring.resting()
    ctx:on_end(function()
      places[which] = settings_dialog:remember()
      settings_dialog:close()
      resting:stop()
    end)
    return ctx:push({
      root = trx.ui.widgets.Stack({
        visible = resting & shown,
        children = { settings_dialog.widget },
      }),
      place = dialog.PLACE,
      on_input = ring.while_resting(function(_, keys)
        if settings_dialog:control(keys) then
          close(ctx)
        end
      end),
    })
  end
end

-- Opens the controls dialog. Confirming out of it leaves the ring, as using
-- an entry does, except on the title screen, where it puts the entry away.
---@param ctx trx.ui.EntryContext
local function open_controls(ctx)
  local resting = ring.resting()
  local layer, close_dialog = controls.open({
    push = function(settings)
      return ctx:push(settings)
    end,
    finish = function(confirmed)
      local title = ring.mode() == trx.ui.MenuMode.TITLE
      if confirmed and not title then
        ctx:confirm()
      else
        close(ctx)
      end
    end,
    visible = resting,
    wrap = ring.while_resting,
  })
  ctx:on_end(function()
    close_dialog()
    resting:stop()
  end)
  return layer
end

-- Pushes one of the dialogs over a screen outside the ring, in the middle of
-- the screen. Backing out of it closes it and runs `on_close`.
local function push_dialog(ctx, which, on_close)
  local push, shown = covering(ctx)
  local settings_dialog = build_dialog(which, push)
  if places[which] ~= nil then
    settings_dialog:restore(places[which])
  end
  local is_open = true
  local function finish()
    if is_open then
      is_open = false
      places[which] = settings_dialog:remember()
      settings_dialog:close()
    end
  end
  ctx:on_end(finish)
  return ctx:push({
    root = trx.ui.widgets.Stack({
      visible = shown,
      children = { settings_dialog.widget },
    }),
    place = { y = 0.5 },
    on_input = function(layer, keys)
      if settings_dialog:control(keys) then
        finish()
        layer:close()
        on_close()
      end
    end,
  })
end

-- Pushes the controls dialog over a screen outside the ring, in the middle of
-- the screen. Leaving it closes it and runs `on_close`.
local function push_controls(ctx, on_close)
  local is_open = true
  ---@type trx.ui.StackLayer
  local layer
  ---@type fun()
  local close_dialog
  local function finish()
    if is_open then
      is_open = false
      close_dialog()
    end
  end
  layer, close_dialog = controls.open({
    push = function(settings)
      return ctx:push(settings)
    end,
    centred = true,
    finish = function()
      finish()
      layer:close()
      on_close()
    end,
  })
  ctx:on_end(finish)
  return layer
end

-- The dialogs, in the order a menu outside the ring lists them: each with
-- its title, and a function that pushes it over a screen and runs `on_close`
-- once the player leaves it.
---@return { title: string, push: fun(ctx: trx.ui.ScreenContext, on_close: fun()): trx.ui.StackLayer }[]
function M.dialogs()
  local result = {}
  for _, which in ipairs({ "graphics", "sound", "gameplay" }) do
    result[#result + 1] = {
      title = (DIALOGS[which]()),
      push = function(ctx, on_close)
        return push_dialog(ctx, which, on_close)
      end,
    }
  end
  result[#result + 1] = {
    title = trx.locale.text("general/settings/controls/title"),
    push = push_controls,
  }
  return result
end

function M.setup()
  trx.events.on_level_unload(function()
    places = {}
    controls.forget()
  end)
  trx.ui.screens.define(
    trx.ui.Screen.RING_ENTRY,
    open_controls,
    { object = trx.catalog.objects.CONTROLS_OPTION }
  )
  for object, which in pairs({
    [trx.catalog.objects.GRAPHICS_OPTION] = "graphics",
    [trx.catalog.objects.SOUND_OPTION] = "sound",
    [trx.catalog.objects.PDA_OPTION] = "gameplay",
  }) do
    trx.ui.screens.define(
      trx.ui.Screen.RING_ENTRY,
      opener(which),
      { object = object }
    )
  end
end

return M
