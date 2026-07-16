-- Prints a swatch of every console text colour and markup, for eyeballing the
-- console renderer.
--
-- Usages:
--   /test-text

trx.console.register({
  name = "test-text",
  run = function()
    local text = ""
    for y = 0, 2 do
      for x = 0, 3 do
        local i = y * 4 + x
        text = text .. string.format("\\{color %d}Color %d\\{/color}   ", i, i)
      end
      text = text .. "\n"
    end
    text = text .. "\\{dim}Dim\\{/dim}\n"
    text = text .. "Secrets: \\{secret 1}\\{secret 2}\\{secret 3}"

    trx.console.log(text)
    return trx.console.Result.OK
  end,
})
