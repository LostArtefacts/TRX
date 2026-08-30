-- The logo the title screen draws, and the setting that says which one.
--
-- The logo carries the game's name in the language it was published in. The
-- original picks it from the language the game itself was built for, which a
-- player cannot change, so it is a setting here, apart from the language the
-- rest of the text is read in.
--
-- The game script requires this, so the setting is declared once for the whole
-- game rather than again with every level.
--
--   local logo = require("tr4.logo")

local M = {}

local OPTION = "visuals.logo_locale"

-- The picture each language has. They hold only what the original samples of
-- them, so each is a little smaller than the box it is stretched over.
local PATHS = {
  ["en-gb"] = "logo.webp",
  ["en-us"] = "logo-en-us.webp",
  ["de"] = "logo-de.webp",
  ["fr"] = "logo-fr.webp",
}

-- Written out in full because the scanner that carries these into the shipped
-- strings reads a key only where it is a literal.
trx.locale.declare({
  ["settings/visuals.logo_locale/title"] = "Title logo",
  ["settings/visuals.logo_locale/description"] = "Which of the game's logos the title screen "
    .. "shows. The original ships one for each language it was published in.",
  ["settings/visuals.logo_locale/values/en-gb"] = "English (UK)",
  ["settings/visuals.logo_locale/values/en-us"] = "English (US)",
  ["settings/visuals.logo_locale/values/de"] = "German",
  ["settings/visuals.logo_locale/values/fr"] = "French",
})

trx.config.declare({
  key = OPTION,
  kind = "dynamic_enum",
  values = { "en-gb", "en-us", "de", "fr" },
  default = "en-gb",
  ui = {
    tab = "graphic_ui",
    after = "ui.show_title_version",
  },
})

-- The picture to draw, which follows the setting, so it changes under the menu
-- as the player moves it.
M.path = trx.signal.config(OPTION):map(function(locale)
  return PATHS[locale]
end)

return M
