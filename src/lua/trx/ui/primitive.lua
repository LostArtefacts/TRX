local raw = trxc.ui
local h = require("trx.internal.helpers")

require("trx.ui")

local ui = trx.ui

-------------------------------------------------------------------------------
-- The primitives
--
-- The drawing primitives the engine exposes to Lua, plus reservations for
-- things Lua lays out itself. Higher-level widget behavior lives in Lua.
--
-- Widgets use these directly. Scripts should only do so when they also reserve
-- the space they draw into, otherwise regions cannot include it in layout.
-------------------------------------------------------------------------------

---@class (partial) trx.ui
---@field primitive trx.ui.primitive

---Low-level drawing calls and layout reservations.
---
---Use `trx.ui.widgets` for normal UI. Use these primitives only when building
---a custom widget. Primitive drawing does not affect region layout unless code
---reserves space first.
---
---Drawing calls are available only during `trx.events.on_ui_paint`. They
---report an error at any other time.
---@class (partial,exact) trx.ui.primitive
ui.primitive = h.namespace("ui.primitive")

---@class (partial) trx.ui.primitive
local primitive = trx.ui.primitive

---Reserves space in a region and returns a slot for it.
---
---The reservation is stacked with the engine UI in that region. Reserve space
---during `trx.events.on_ui_draw`, then read the assigned box during
---`trx.events.on_ui_paint`.
---
---A slot is valid only for the scene that created it.
---@param region trx.ui.Region Which region to keep room in.
---@param w number How wide, in canvas units.
---@param h number How tall, in canvas units.
---@return integer # The slot.
---@type fun(region: trx.ui.Region, w: number, h: number): integer
primitive.reserve = raw.reserve

---Returns the box assigned to a reservation by the last layout.
---@param slot integer The reservation slot.
---@return number? # The left edge, or `nil` when the slot is no longer valid.
---@return number # The top edge.
---@return number # The width.
---@return number # The height.
---@type fun(slot: integer): number?, number, number, number
primitive.slot_box = raw.slot_box

---Measures one line of text. Available at any time.
---@param text string What to measure.
---@param scale? number Multiplies the text size. `1.0` by default.
---@return number # The width, in canvas units.
---@return number # The height, in canvas units.
---@type fun(text: string, scale?: number): number, number
primitive.measure_text = raw.measure_text

---Draws one line of text on the canvas.
---@param text string What to draw.
---@param x number The left edge.
---@param y number The top edge.
---@param scale? number Multiplies the text size.
---@param z? integer The draw order.
---@type fun(text: string, x: number, y: number, scale?: number, z?: integer)
primitive.text = raw.draw_text

---Converts a canvas length to screen pixels.
---
---The canvas is a fixed 640x480 grid, and the screen size depends on the
---player settings and window. Use this with `trx.ui.primitive.to_canvas` when
---geometry must align to whole screen pixels, such as an even border.
---@param length number A canvas length.
---@return number # The same length in screen pixels.
---@type fun(length: number): number
primitive.to_screen = raw.to_screen

---Converts a screen-pixel length to canvas units.
---
---Use this with `trx.ui.primitive.to_screen` when geometry must align to whole
---screen pixels.
---@param pixels number A length in screen pixels.
---@return number # The same length in canvas units.
---@type fun(pixels: number): number
primitive.to_canvas = raw.to_canvas

---Draws a horizontal rule in the selected menu style.
---@param x0 number The left end.
---@param x1 number The right end.
---@param y number The vertical position.
---@param z? integer The draw order.
---@type fun(x0: number, x1: number, y: number, z?: integer)
primitive.horizontal_line = raw.horizontal_line

---Draws the box the game draws behind a dialog, in the style the player chose.
---
---The look follows the menu style setting, so a panel drawn this way matches
---the game's own dialogs rather than standing apart from them.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer The draw order.
---@param w number The width.
---@param h number The height.
---@param style trx.ui.FrameStyle Which of the game's frames to draw.
---@type fun(x: number, y: number, z: integer, w: number, h: number, style: trx.ui.FrameStyle)
primitive.panel = raw.panel

---Draws a rectangle of one color.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer The draw order.
---@param w number The width.
---@param h number The height.
---@param color trx.math.Color|table What color to fill it with: a `trx.math.Color`, or a table of `r`, `g`, `b` and `a` channels from 0 to 255. The color is opaque where `a` is absent. <!--noref: r, g, b, a-->
---@type fun(x: number, y: number, z: integer, w: number, h: number, color: trx.math.Color|table)
primitive.quad = raw.flat_quad

---Draws a rectangle whose corners each carry a color.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer What to draw in front of.
---@param w number The width.
---@param h number The height.
---@param tl trx.math.Color|table The top-left color, in either form that `trx.ui.primitive.quad` takes.
---@param tr trx.math.Color|table The top-right color, in either form that `trx.ui.primitive.quad` takes.
---@param bl trx.math.Color|table The bottom-left color, in either form that `trx.ui.primitive.quad` takes.
---@param br trx.math.Color|table The bottom-right color, in either form that `trx.ui.primitive.quad` takes.
---@type fun(x: number, y: number, z: integer, w: number, h: number, tl: trx.math.Color|table, tr: trx.math.Color|table, bl: trx.math.Color|table, br: trx.math.Color|table)
primitive.gradient_quad = raw.gradient_quad

---Draws an image file in a box on the canvas.
---
---The image is looked for where the game keeps its images, and stretches to
---fill the box, so a box of the image's own shape keeps that shape. The image
---draws under everything else the canvas holds, whatever order the calls come
---in.
---
---Returns whether the game has such an image, so a script can leave the space
---alone where it does not.
---
---```lua
---trx.ui.primitive.image("uklogo.pak", 64, 0, 512, 256)
---```
---@param path string The image file, named from the images directory.
---@param x number The left edge.
---@param y number The top edge.
---@param w number The width.
---@param h number The height.
---@param opacity? number How solid the image is, from 0 to 1. `1` by default.
---@return boolean # Whether the image was there to draw.
---@type fun(path: string, x: number, y: number, w: number, h: number, opacity?: number): boolean
primitive.image = raw.image

---Reports how many sprites an object has.
---
---An object the level did not load has none, and a model has none as well. Use
---this function to check whether `trx.ui.primitive.sprite` has anything to
---draw.
---@param object trx.catalog.objects The sprite object to count.
---@return integer # How many sprites it has.
---@type fun(object: trx.catalog.objects): integer
primitive.sprite_count = raw.sprite_count

---Reports the edges of one sprite of an object, in canvas units at a scale of
---one.
---
---The edges sit around the point the sprite is drawn at, so both left and top
---are usually negative. Multiply them by the scale the sprite is drawn at.
---
---Raises where the level did not load the object, so check
---`trx.objects.get(object).loaded` first.
---@param object trx.catalog.objects The sprite object to read from.
---@param sprite_num integer Which sprite of the object to read, counted from 0.
---@return number # The left edge.
---@return number # The top edge.
---@return number # The right edge.
---@return number # The bottom edge.
---@type fun(object: trx.catalog.objects, sprite_num: integer): number, number, number, number
primitive.sprite_bounds = raw.sprite_bounds

---Reports the box a model occupies, from the first frame of its first
---animation.
---
---The box sits around the point the model is drawn at, so the low edges are
---usually negative. A script fits a model into a box of its own by comparing
---the two.
---
---Returns nothing where the object carries no model, which is how a script
---tells whether it can draw one at all. Raises where the level did not load
---the object, so check `trx.objects.get(object).loaded` first.
---@param object trx.catalog.objects The model object to measure.
---@return trx.math.Distance # The low edge across, or `nil` where the object carries no model.
---@return trx.math.Distance # The low edge down.
---@return trx.math.Distance # The low edge into the screen.
---@return trx.math.Distance # The high edge across.
---@return trx.math.Distance # The high edge down.
---@return trx.math.Distance # The high edge into the screen.
---@type fun(object: trx.catalog.objects): trx.math.Distance, trx.math.Distance, trx.math.Distance, trx.math.Distance, trx.math.Distance, trx.math.Distance
primitive.mesh_bounds = raw.mesh_bounds

---Draws one sprite of an object on the canvas.
---
---Raises where the level did not load the object, so check
---`trx.objects.get(object).loaded` first.
---
---```lua
---trx.ui.primitive.sprite(
---  trx.catalog.objects.assault_digits, 3, 100, 20, 0, 1,
---  trx.math.color("ffffff"))
---```
---@param object trx.catalog.objects The sprite object to draw from.
---@param sprite_num integer Which sprite of the object to draw, counted from 0.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer The draw order.
---@param scale number Multiplies the sprite size. At 1 the sprite draws at its own size on the canvas.
---@param color trx.math.Color|table What color to tint it with, in either form that `trx.ui.primitive.quad` takes.
---@type fun(object: trx.catalog.objects, sprite_num: integer, x: number, y: number, z: integer, scale: number, color: trx.math.Color|table)
primitive.sprite = raw.sprite

---Draws one sprite of an object, with a color at each corner.
---
---Raises where the level did not load the object, so check
---`trx.objects.get(object).loaded` first.
---@param object trx.catalog.objects The sprite object to draw from.
---@param sprite_num integer Which sprite of the object to draw, counted from 0.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer The draw order.
---@param scale number Multiplies the sprite size. At 1 the sprite draws at its own size on the canvas.
---@param tl trx.math.Color|table The top-left color, in either form that `trx.ui.primitive.quad` takes.
---@param tr trx.math.Color|table The top-right color, in either form that `trx.ui.primitive.quad` takes.
---@param bl trx.math.Color|table The bottom-left color, in either form that `trx.ui.primitive.quad` takes.
---@param br trx.math.Color|table The bottom-right color, in either form that `trx.ui.primitive.quad` takes.
---@type fun(object: trx.catalog.objects, sprite_num: integer, x: number, y: number, z: integer, scale: number, tl: trx.math.Color|table, tr: trx.math.Color|table, bl: trx.math.Color|table, br: trx.math.Color|table)
primitive.gradient_sprite = raw.gradient_sprite
