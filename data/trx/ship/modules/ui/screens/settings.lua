-- The settings dialogs of the inventory ring: the graphics, the sound and the
-- gameplay settings, each a dialog of tabs.

local color_editor = require("common.ui.settings.color_editor")
local dialog = require("common.ui.settings.dialog")
local editor = require("common.ui.settings.editor")
local handlers = require("common.ui.settings.handlers")
local preset_tab = require("common.ui.settings.preset_tab")
local ring = require("common.ui.ring")
local tabs = require("common.ui.settings.tabs")

local M = {}

-- What each dialog holds: its title, and its tabs, each the settings tab it
-- shows or the presets. The text is read as the dialog opens, so that it
-- follows the player's language.
local DIALOGS = {
  graphics = function()
    return trx.locale.get("general/settings/graphic_settings/title"),
      {
        {
          trx.locale.get("general/settings/graphic_settings/tabs/visuals"),
          "graphic_visuals",
        },
        {
          trx.locale.get("general/settings/graphic_settings/tabs/ui"),
          "graphic_ui",
        },
        {
          trx.locale.get("general/settings/graphic_settings/tabs/stats"),
          "graphic_ui_stats",
        },
        {
          trx.locale.get("general/settings/graphic_settings/tabs/bars"),
          "graphic_ui_bars",
        },
        {
          trx.locale.get("general/settings/graphic_settings/tabs/rendering"),
          "graphic_rendering",
        },
      }
  end,
  sound = function()
    return trx.locale.get("general/settings/sound/title"),
      {
        {
          trx.locale.get("general/settings/sound/tabs/volume"),
          "sound_volume",
        },
        { trx.locale.get("general/settings/sound/tabs/misc"), "sound_misc" },
      }
  end,
  gameplay = function()
    return trx.locale.get("general/settings/gameplay/title"),
      {
        {
          trx.locale.get("general/settings/gameplay/tabs/general"),
          "gameplay_general",
        },
        {
          trx.locale.get("general/settings/gameplay/tabs/controls"),
          "gameplay_controls",
        },
        {
          trx.locale.get("general/settings/gameplay/tabs/mods"),
          "gameplay_mods",
        },
        {
          trx.locale.get("general/settings/gameplay/tabs/fixes"),
          "gameplay_fixes",
        },
        { trx.locale.get("general/settings/gameplay/tabs/presets") },
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
  if trx.inventory_ring.selection_anim() ~= nil then
    trx.inventory_ring.animate_selection(0, -1)
  end
  ctx:cancel()
end

local function opener(which)
  ---@param ctx trx.ui.EntryContext
  return function(ctx)
    local title, specs = DIALOGS[which]()
    local declared = declared_by_tab()
    local push = function(settings)
      return ctx:push(settings)
    end

    local settings_dialog = dialog.new({
      title = title,
      tabs = function(self)
        local result = {}
        for i, spec in ipairs(specs) do
          local name = spec[2]
          local tab
          if name == nil then
            tab = preset_tab.new({
              title = spec[1],
              editing = self.editing,
              push = push,
            })
          else
            tab = editor.new({
              title = spec[1],
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
          result[i] = tab
        end
        return result
      end,
    })

    local resting = ring.resting()
    ctx:on_end(function()
      settings_dialog:close()
      resting:stop()
    end)
    return ctx:push({
      root = trx.ui.widgets.Stack({
        shown = resting,
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

function M.setup()
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
