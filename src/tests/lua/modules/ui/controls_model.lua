-- The setting that picks the model of the controls item. The harness runs as
-- TR1, so a controller shows the Saturn pad.

local h = require("harness")
local test = h.test

local controls_model = require("common.ui.controls_model")
controls_model.setup()

local Context = trx.catalog.Context
local objects = trx.catalog.objects

local MODELS = {
  trx.catalog.mint(Context.OBJECTS, "controls_option_saturn"),
  trx.catalog.mint(Context.OBJECTS, "controls_option_keys"),
  trx.catalog.mint(Context.OBJECTS, "controls_option_ps1"),
}

-- Returns the model that a choice shows with the controls set to a backend,
-- in a level that holds every model.
local function picks(choice, backend)
  for _, model in ipairs(MODELS) do
    fake.set_loaded(model, true)
  end
  trx.config.set("visuals.controls_model", choice)
  trx.config.set("input.backend", backend or "keyboard")
  return controls_model.object()
end

test("automatic follows the controls", function()
  assert(picks("auto", "keyboard") == objects.controls_option_keys)
  assert(picks("auto", "controller") == objects.controls_option_saturn)
  assert(picks("auto", "touch") == objects.controls_option_keys)
end)

test("a model picked by name ignores the controls", function()
  assert(picks("saturn", "keyboard") == objects.controls_option_saturn)
  assert(picks("keyboard", "controller") == objects.controls_option_keys)
  assert(picks("ps1", "keyboard") == objects.controls_option_ps1)
end)

test("the original model is the controls item itself", function()
  assert(picks("original", "controller") == objects.controls_option)
end)

test("a model the level does not hold falls back to the original", function()
  picks("ps1")
  fake.set_loaded(objects.controls_option_ps1, false)
  assert(controls_model.object() == objects.controls_option)
end)

return h.report()
