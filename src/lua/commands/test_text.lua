-- Prints a swatch of the kinds of text colour and markup, for a look at the
-- console renderer.
--
-- Usages:
--   /test-text

trx.console.register({
  name = "test-text",
  run = function()
    local text = ""
    for _, color in ipairs({
      "#ffffff",
      "#c08040 #401000",
      "#ff6060 #180000",
      "#b6d164 #b62013",
      "#ffffff80",
      "#808080 pulse",
    }) do
      text = text .. ("\\{color %s}%s\\{/color}\n"):format(color, color)
    end
    text = text .. "Secrets: \\{secret 1}\\{secret 2}\\{secret 3}"

    trx.console.log(text)
    return trx.console.Result.OK
  end,
})
