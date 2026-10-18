local raw = trxc.ui
local h = require("trx.internal.helpers")
local themes = require("trx.internal.themes")

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
---Drawing calls are available only during `trx.ui.on_paint`. They
---report an error at any other time.
---@class (partial,exact) trx.ui.primitive
ui.primitive = h.namespace("ui.primitive")

---@class (partial) trx.ui.primitive
local primitive = trx.ui.primitive

---Reserves space in a region and returns a slot for it.
---
---The reservation is stacked with the engine UI in that region. Reserve space
---during `trx.ui.on_reserve`, then read the assigned box during
---`trx.ui.on_paint`.
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

---Breaks text into lines that fit a width, the way the game wraps its own
---text. Available at any time.
---
---```lua
---local lines = trx.ui.primitive.wrap_text(description, 300)
---```
---@param text string What to wrap.
---@param width number The widest a line may be, in canvas units.
---@param scale? number Multiplies the text size. `1.0` by default.
---@return string # The text with line breaks where the lines end.
---@type fun(text: string, width: number, scale?: number): string
primitive.wrap_text = raw.wrap_text

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
function primitive.horizontal_line(x0, x1, y, z)
  raw.horizontal_line(x0, x1, y, z, themes.menu())
end

-- What each frame style draws: the menu colors of one text style, and whether
-- it fills the box behind its child or draws only the outline.
local frames = (function()
  local look = h.constants("TEXT_STYLE")
  local FrameStyle = ui.FrameStyle
  return {
    [FrameStyle.DIALOG] = { look = look.BACKGROUND, fill = true },
    [FrameStyle.DIALOG_HEAVY] = { look = look.BACKGROUND_HEAVY, fill = true },
    [FrameStyle.HEADING] = { look = look.HEADING, fill = true },
    [FrameStyle.SELECTED] = { look = look.REQUESTED, fill = true },
    [FrameStyle.OUTLINE] = { look = look.REQUESTED, fill = false },
    [FrameStyle.HEADING_OUTLINE] = { look = look.HEADING, fill = false },
  }
end)()

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
function primitive.panel(x, y, z, w, h, style)
  local frame = frames[style]
  if frame == nil then
    error("unknown frame style", 2)
  end
  raw.panel(x, y, z, w, h, frame.look, frame.fill, themes.menu())
end

---Draws a rectangle of one color.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer The draw order.
---@param w number The width.
---@param h number The height.
---@param color trx.math.Color|table What color to fill it with: a `trx.math.Color`, or a table of `r`, `g`, `b` and `a` channels from 0 to 255. The color is opaque where `a` is absent. <!--noref: r, g, b, a-->
---@type fun(x: number, y: number, z: integer, w: number, h: number, color: trx.math.Color|table)
primitive.quad = raw.flat_quad

---Draws a straight line of one color between two points, with smooth edges.
---@param x0 number Where the line starts across.
---@param y0 number Where the line starts down.
---@param x1 number Where the line ends across.
---@param y1 number Where the line ends down.
---@param z integer The draw order.
---@param width number How thick the line is.
---@param color trx.math.Color|table What color to draw it in, in either form that `trx.ui.primitive.quad` takes.
---@type fun(x0: number, y0: number, x1: number, y1: number, z: integer, width: number, color: trx.math.Color|table)
primitive.line = raw.line

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

---The colour of a light, from `0` to `255` in each channel, where a channel
---may fall between two whole values as the game's own lights do. A channel
---above `255`, up to `510`, lights a model brighter than its own colour, as
---the TR4 inventory lights the entry the ring rests on.
---@class trx.ui.LightColor
---@field r number The red channel.
---@field g number The green channel.
---@field b number The blue channel.

---A light shining on a model from one direction.
---@class (exact) trx.ui.Light
---@field dir trx.math.Vec3 Where the light comes from, as the viewer sees the model: across to the right, down, and away from the viewer. Only the direction counts.
---@field color trx.math.Color|trx.ui.LightColor The light's colour. TR1 and TR2 read its brightness alone.

---The light a model is drawn in. Each game reads it its own way: TR3 lights a
---model in colour from all of it, TR4 by the ambient light alone, and TR1 and
---TR2 by the brightness of the ambient light and of the first light.
---@class (exact) trx.ui.ModelLight
---@field ambient? trx.math.Color|trx.ui.LightColor The light that reaches every mesh from all round. Black by default.
---@field lights? trx.ui.Light[] Up to three lights shining from a direction each. None by default.
---@field meshes? table<integer, trx.math.Color|trx.ui.LightColor> In TR3, a colour every light takes on the meshes it names, by their place in the model counted from 0.

---@class (exact) trx.ui.primitive.model.opts
---@field rot? trx.math.Rot How far to turn the model. Not turned by default.
---@field mesh_mask? integer Which of its meshes to draw, one bit each. Every mesh by default.
---@field light? trx.ui.ModelLight The light to draw it in. The light the game holds up a pickup in by default.
---@field held? boolean Whether to tilt and turn it first as the game holds up a pickup, which shows its face. `true` by default; `false` draws it as it was built.
---@field tilt? trx.math.Angle How far to tilt it towards the viewer before anything else, about the screen's horizontal axis, as a camera looking down on it would. Not tilted by default.

-- The light each game holds a pickup up in, by its number. Built once the
-- colours can be.
---@type table<integer, trx.ui.ModelLight>?
local pickup_lights

---@return table<integer, trx.ui.ModelLight>
local function get_pickup_lights()
  if pickup_lights ~= nil then
    return pickup_lights
  end
  local color = trx.math.color
  local tr1 = {
    ambient = color(96, 96, 96),
    lights = {
      {
        dir = { x = 10011, y = 8185, z = 10027 },
        color = color(128, 128, 128),
      },
    },
  }
  pickup_lights = {
    [1] = tr1,
    [2] = tr1,
    [3] = {
      ambient = color(64, 64, 64),
      -- The sun, a spot and a dynamic light, as TR3's SetPickupLight sets
      -- them.
      lights = {
        {
          dir = { x = 8192, y = -6144, z = -8192 },
          color = color(191, 105, 40),
        },
        {
          dir = { x = -8192, y = -12288, z = -16384 },
          color = color(64, 64, 64),
        },
        {
          dir = { x = 0, y = -12288, z = 8192 },
          color = color(40, 151, 254),
        },
      },
    },
    [4] = { ambient = color(255, 255, 255) },
  }
  return pickup_lights
end

---Draws an object's model in a box on the canvas, fitted into the box. The
---model stands where it is painted among the rest of the interface: over what
---was painted before it, under what is painted after.
---
---Draws nothing where the level did not load the object, or where the object
---carries no model.
---
---```lua
---trx.ui.primitive.model(trx.catalog.objects.small_medipack_option,
---  100, 50, 64, 48, { rot = { x = 0, y = 30 * trx.math.DEG_1, z = 0 } })
---```
---@param object trx.catalog.objects The object to draw.
---@param x number The left edge.
---@param y number The top edge.
---@param w number The width of the box.
---@param h number The height of the box.
---@param opts? trx.ui.primitive.model.opts How to turn it, which meshes to draw and how to light them.
function primitive.model(object, x, y, w, h, opts)
  if opts == nil or opts.light == nil then
    opts = opts or {}
    ---@type trx.ui.primitive.model.opts
    opts = {
      rot = opts.rot,
      mesh_mask = opts.mesh_mask,
      light = get_pickup_lights()[trx.game.tr_version],
      held = opts.held,
      tilt = opts.tilt,
    }
  end
  raw.draw_model(object, x, y, w, h, opts)
end

---A camera looking into a world of a model's own.
---@class (exact) trx.ui.WorldCamera
---@field pos trx.math.Vec3 Where the camera stands.
---@field target trx.math.Vec3 The point it looks at.
---@field fov trx.math.Angle Its field of view, measured as the game measures the inventory's.

---One step of placing a model in its world. A step takes one of the fields.
---@class (exact) trx.ui.PoseStep
---@field move? trx.math.Vec3 How far to move, along the axes as the steps before have turned them.
---@field turn? trx.math.Rot How far to turn, by its yaw, then its pitch, then its roll.
---@field scale? number How much to scale the model, with `1` for its own size.

---A turn that one bone of a model takes on top of the frame the model is posed
---in, as the compass needle and the hands of the stopwatch turn. A field left
---out turns the bone not at all about that axis.
---@class (exact) trx.ui.BoneTurn
---@field bone integer The bone, counted from 0, which moves the mesh after it.
---@field x? trx.math.Angle How far to pitch it.
---@field y? trx.math.Angle How far to yaw it.
---@field z? trx.math.Angle How far to roll it.

---@class (exact) trx.ui.primitive.world_model.opts
---@field camera trx.ui.WorldCamera The camera that sees the model.
---@field pose trx.ui.PoseStep[] The steps that place the model, applied in order from the world's origin, up to sixteen.
---@field frame? integer The frame of the model's first animation to pose it in, counted from 0. The first by default.
---@field next_frame? integer The frame to blend towards. The same frame by default.
---@field blend? number How far to blend towards the next frame, from `0` to `1`. `0` by default.
---@field bones? trx.ui.BoneTurn[] Turns that bones take on top of the frame, up to eight.
---@field at_pivot? boolean Whether to draw the model about its pivot, without the offset its frame stands it at. `false` by default.
---@field mesh_mask? integer Which of its meshes to draw, one bit each. Every mesh by default.
---@field mesh_opacity? table<integer, number> How solid to draw some of its meshes, from `0` to `1`, keyed by the mesh counted from 0. A mesh not named is solid.
---@field light? trx.ui.ModelLight The light to draw it in, with its directions in the model's world. A plain white light by default.

---Draws an object's model where it stands in a world of its own, as that
---world's camera sees it over the whole canvas: placed by moves and turns
---applied in order, the way the engine places a model, so that a script can
---show a model as the game would have shown it. It stands behind the whole
---interface, over what went behind it before, such as a background.
---
---Draws nothing where the level did not load the object, or where the object
---carries no model.
---@param object trx.catalog.objects The object to draw.
---@param opts trx.ui.primitive.world_model.opts Where it stands, the camera that sees it, and its light.
---@type fun(object: trx.catalog.objects, opts: trx.ui.primitive.world_model.opts)
primitive.world_model = raw.draw_world_model

---Darkens the screen behind the whole interface, edge to edge, as the game
---darkens it behind a menu and fades it to black. It goes over what went
---behind the interface before it, such as a background or a model in a world
---of its own, and under the whole interface.
---@param opacity number How dark, from `0` for not at all to `1` for black.
---@type fun(opacity: number)
primitive.darken = raw.darken

---Draws a backdrop behind the whole interface, as the game draws the ones
---behind its menus, part way in by the opacity. It goes under any model in a
---world of its own drawn after it, as the inventory ring's models stand on
---their backdrop.
---
---```lua
---trx.ui.primitive.backdrop("transparent", 0.5)
---```
---@param style string The backdrop style, by the names that the `ui.inventory_background_style` setting takes. <!--noref: ui.inventory_background_style-->
---@param opacity number How far it has come in, from `0` for not at all to `1`.
---@param path? string The picture of an `image` backdrop, as `trx.ui.primitive.background` takes it. <!--noref: image-->
---@type fun(style: string, opacity: number, path?: string)
primitive.backdrop = raw.draw_backdrop

---Draws an image behind the whole interface, as the game draws the pictures
---behind its screens, fitted to the screen's shape and darkened by the dim.
---Without an image, or where it does not exist, the screen behind the
---interface is black. The game's scene behind the canvas is drawn over.
---
---```lua
---trx.ui.primitive.background("india.webp", true, 0.5)
---```
---@param path? string The image file, named from the images directory, or as a screen reports it.
---@param mono? boolean Whether to draw it without its colour.
---@param dim? number How much to darken it, from `0` for not at all to `1` for black. Not darkened by default.
---@return boolean # Whether the image exists.
---@type fun(path?: string, mono?: boolean, dim?: number): boolean
primitive.background = raw.draw_background

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

---Draws one sprite of an object stretched over a box. Unlike
---`trx.ui.primitive.sprite`, it ignores the edges the sprite declares, which
---suits sprites that carry none, as the TR4 interface sprites do.
---@param object trx.catalog.objects The sprite object to draw from.
---@param sprite_num integer Which sprite of the object to draw, counted from 0.
---@param x number The left edge.
---@param y number The top edge.
---@param z integer The draw order.
---@param w number The width.
---@param h number The height.
---@param color trx.math.Color|table What color to tint it with, in either form that `trx.ui.primitive.quad` takes.
---@type fun(object: trx.catalog.objects, sprite_num: integer, x: number, y: number, z: integer, w: number, h: number, color: trx.math.Color|table)
primitive.sprite_rect = raw.sprite_rect

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
