#!/usr/bin/env python3
import argparse
import ast
import json
import sys
import textwrap
from dataclasses import dataclass
from functools import lru_cache
from pathlib import Path

# pip install rectpack numpy lark Pillow
import numpy as np
import rectpack
from lark import Lark, Transformer
from PIL import Image

GLYPH_GRAMMAR = Lark(
    r"""
    start: (command | include | _EMPTY_LINE | _COMMENT)*

    include: "include" _WS quoted_string
    command: glyph_text _WS GLYPH_CLASS _WS source_func (_WS modifier_func)* _NL

    glyph_text: unicode_definition | quoted_string
    GLYPH_CLASS: "N" | "C" | "c" | "R"
    unicode_definition: "U+" /[0-9A-F]{4}/ ":" /./
    number: /-?\d+/
    quoted_string: QUOTED_STRING

    source_func: grid_sprite_func | manual_sprite_func | image_func | combine_func | link_func
    grid_sprite_func: "grid_sprite" func_params
    manual_sprite_func: "manual_sprite" func_params
    image_func: "image" func_params
    combine_func: "combine" func_params
    link_func: "link" func_params

    modifier_func: expand_func | translate_func
    expand_func: "expand" func_params
    translate_func: "translate" func_params

    ?expr: quoted_string | number | unicode_definition
    arglist: arg ("," _WS? arg)*
    ?arg: kwarg | expr
    kwarg: /[a-z][a-z0-9_]*/ _WS? "=" _WS? expr
    func_params: "(" arglist ")"

    _STRING_INNER: /.*?/
    _STRING_ESC_INNER: _STRING_INNER _STRING_INNER2
    _STRING_INNER2: /(?<!\\)(\\\\)*?/
    QUOTED_STRING: "\"" _STRING_ESC_INNER "\""

    _EMPTY_LINE: _WS? _NL
    _COMMENT: "#" _COMMENT_VAL _NL
    _COMMENT_VAL: /[^\n]*/
    _NL: "\n"
    _WS: " "+
""",
    parser="lalr",
    strict=True,
)


@lru_cache(maxsize=10)
def load_image(path: Path) -> np.ndarray:
    return np.array(Image.open(path).convert("RGBA"))


def trim_transparent_pixels(
    image: np.ndarray,
) -> tuple[np.ndarray, tuple[int, int, int, int]]:
    if image.shape[2] != 4:
        raise ValueError("Image must be RGBA")

    # Find non-transparent mask
    non_transparent = image[:, :, 3] != 0

    # If all pixels are transparent
    if not non_transparent.any():
        return image, (0, 0, 0, 0)

    # Where the non-transparent pixels are located
    rows = np.any(non_transparent, axis=1)
    cols = np.any(non_transparent, axis=0)

    # Calculate the number of rows and columns removed
    top = int(np.argmax(rows))
    bottom = int(np.argmax(rows[::-1]))
    left = int(np.argmax(cols))
    right = int(np.argmax(cols[::-1]))
    height, width, _ = image.shape

    # Trim the image
    trimmed_image = image[top : height - bottom, left : width - right]

    return trimmed_image, (top, bottom, left, right)


@dataclass
class Rect:
    x: int
    y: int
    w: int
    h: int


@dataclass
class ParserContext:
    input_dir: Path


class BaseSource:
    """Contains information how to render the given glyph."""

    index: int | None = None
    has_own_index: bool = True

    def load(self) -> tuple[np.ndarray, Rect]:
        """A method to load sprite pixels and the glyph bounding box."""
        raise NotImplementedError("not implemented")


@dataclass
class GridSpriteSource(BaseSource):
    """A grid-based sprite sheet source for the glyph."""

    ctx: ParserContext
    filename: str
    cell_x: int
    cell_y: int
    cell_size: int = 20
    index: int | None = None

    def load(self) -> tuple[np.ndarray, Rect]:
        src_pixels = load_image(self.ctx.input_dir / self.filename)
        x = self.cell_size * self.cell_x
        y = self.cell_size * self.cell_y
        cell_pixels = src_pixels[
            y : y + self.cell_size, x : x + self.cell_size
        ]
        pixels, removed = trim_transparent_pixels(cell_pixels)
        bbox = Rect(
            x=0,
            y=-16 + removed[0],
            w=self.cell_size - removed[2] - removed[3],
            h=self.cell_size - removed[1] - removed[0],
        )
        return pixels, bbox


@dataclass
class ManualSpriteSource(BaseSource):
    """A grid-based sprite sheet source for the glyph."""

    ctx: ParserContext
    filename: str
    x: int
    y: int
    w: int
    h: int
    index: int | None = None

    def load(self) -> tuple[np.ndarray, Rect]:
        src_pixels = load_image(self.ctx.input_dir / self.filename)
        cell_pixels = src_pixels[
            self.y : self.y + self.h, self.x : self.x + self.w
        ]
        pixels, removed = trim_transparent_pixels(cell_pixels)
        bbox = Rect(
            x=0,
            y=-15 + removed[0],
            w=self.w - removed[2] - removed[3],
            h=self.h - removed[1] - removed[0],
        )
        return pixels, bbox


@dataclass
class ImageSource(BaseSource):
    """A full-image source for the glyph."""

    ctx: ParserContext
    filename: str
    x1: int
    y1: int
    x2: int
    y2: int
    index: int | None = None

    def load(self) -> tuple[np.ndarray, Rect]:
        pixels = load_image(self.ctx.input_dir / self.filename)
        bbox = Rect(self.x1, self.y1, self.x2 - self.x1, self.y2 - self.y1)
        assert bbox.w == pixels.shape[1]
        return pixels, bbox


@dataclass
class LinkSource(BaseSource):
    """A source pointing to another glyph's source."""

    ctx: ParserContext
    link_to: str
    index: int | None = None

    has_own_index = False


class CombineSource(BaseSource):
    """A source merging two glyphs into one."""

    def __init__(
        self,
        ctx: ParserContext,
        glyph1: str,
        glyph2: str,
        offset_x: int = 0,
        offset_y: int = 0,
        align: str | None = "top",
    ) -> None:
        self.glyph1 = glyph1
        self.glyph2 = glyph2
        self.offset_x = offset_x
        self.offset_y = offset_y
        self.align = align

        self.index: int | None = None
        self.glyph1_source: BaseSource | None = None
        self.glyph2_source: BaseSource | None = None
        self._cached_render: tuple[np.ndarray, Rect] | None = None

    def load(self) -> tuple[np.ndarray, Rect]:
        assert self.glyph1_source is not None
        assert self.glyph2_source is not None

        if self._cached_render is not None:
            return self._cached_render

        main_pixels, main_bbox = self.glyph1_source.load()
        combining_pixels, combining_bbox = self.glyph2_source.load()
        offset_x, offset_y = self._compute_offset(main_bbox, combining_bbox)
        composed_pixels, composed_bbox = self._compose_pixels(
            main_pixels,
            main_bbox,
            combining_pixels,
            combining_bbox,
            offset_x,
            offset_y,
        )
        self._cached_render = (composed_pixels, composed_bbox)
        return self._cached_render

    def get_offset(self) -> tuple[int, int]:
        assert self.glyph1_source
        assert self.glyph2_source
        _, main_bbox = self.glyph1_source.load()
        _, combining_bbox = self.glyph2_source.load()
        return self._compute_offset(main_bbox, combining_bbox)

    def _compute_offset(self, main_bbox: Rect, combining_bbox: Rect) -> tuple[int, int]:
        offset_x = self.offset_x
        offset_y = self.offset_y

        offset_x += (main_bbox.w - combining_bbox.w) // 2
        if self.align == "top":
            offset_y += main_bbox.y - combining_bbox.y - combining_bbox.h
        elif self.align == "center":
            offset_y += (main_bbox.y - combining_bbox.y) + (
                main_bbox.h - combining_bbox.h
            ) // 2
        elif self.align == "bottom":
            offset_y += main_bbox.y + main_bbox.h - combining_bbox.y
        return offset_x, offset_y

    def _compose_pixels(
        self,
        main_pixels: np.ndarray,
        main_bbox: Rect,
        combining_pixels: np.ndarray,
        combining_bbox: Rect,
        offset_x: int,
        offset_y: int,
    ) -> tuple[np.ndarray, Rect]:
        combining_left = combining_bbox.x + offset_x
        combining_top = combining_bbox.y + offset_y

        left = min(main_bbox.x, combining_left)
        top = min(main_bbox.y, combining_top)
        right = max(main_bbox.x + main_bbox.w, combining_left + combining_bbox.w)
        bottom = max(main_bbox.y + main_bbox.h, combining_top + combining_bbox.h)

        width = right - left
        height = bottom - top
        canvas = np.zeros((height, width, 4), dtype=np.uint8)

        base_x = main_bbox.x - left
        base_y = main_bbox.y - top
        accent_x = combining_left - left
        accent_y = combining_top - top

        canvas[
            base_y : base_y + main_bbox.h,
            base_x : base_x + main_bbox.w,
        ] = main_pixels
        self._alpha_blit(canvas, combining_pixels, accent_x, accent_y)

        return canvas, Rect(x=left, y=top, w=width, h=height)

    @staticmethod
    def _alpha_blit(
        dst: np.ndarray,
        src: np.ndarray,
        dst_x: int,
        dst_y: int,
    ) -> None:
        height, width, _ = src.shape
        region = dst[dst_y : dst_y + height, dst_x : dst_x + width]

        src_rgb = src[:, :, :3].astype(np.float32) / 255.0
        src_alpha = src[:, :, 3:4].astype(np.float32) / 255.0
        dst_rgb = region[:, :, :3].astype(np.float32) / 255.0
        dst_alpha = region[:, :, 3:4].astype(np.float32) / 255.0

        out_alpha = src_alpha + dst_alpha * (1.0 - src_alpha)
        safe_alpha = np.where(out_alpha == 0.0, 1.0, out_alpha)
        out_rgb = (
            src_rgb * src_alpha + dst_rgb * dst_alpha * (1.0 - src_alpha)
        ) / safe_alpha

        region[:, :, :3] = np.clip(out_rgb * 255.0, 0, 255).astype(np.uint8)
        region[:, :, 3] = np.clip(out_alpha * 255.0, 0, 255).astype(np.uint8)[
            :, :, 0
        ]


class BaseModifier:
    def modify_glyph(self, glyph: "Glyph") -> None:
        pass


@dataclass
class ExpandModifier(BaseModifier):
    w: int = 0

    def modify_glyph(self, glyph: "Glyph") -> None:
        glyph.extra_width += self.w


@dataclass
class TranslateModifier(BaseModifier):
    x: int = 0
    y: int = 0

    def modify_glyph(self, glyph: "Glyph") -> None:
        glyph.extra_x += self.x
        glyph.extra_y += self.y


@dataclass
class Glyph:
    text: str
    glyph_class: str
    source: BaseSource
    modifiers: list[BaseModifier]

    extra_width: int = 0
    extra_x: int = 0
    extra_y: int = 0

    def __str__(self) -> str:
        if len(self.text) == 1:
            return f"U+{ord(self.text):04X}:{self.text}"
        return self.text


class GlyphParser(Transformer):
    """Parse the mapping.txt file into Python objects."""

    def __init__(self, ctx: ParserContext) -> None:
        self.ctx = ctx
        return super().__init__()

    GLYPH_CLASS = lambda self, items: items[0]

    glyph_text = lambda self, items: items[0]
    arglist = lambda self, items: items
    kwarg = lambda self, items: (items[0].value, items[1])
    func_kwargs = lambda self, items: dict(items)

    def func_params(self, items):
        args = []
        kwargs = {}
        for arg in items[0]:
            if isinstance(arg, tuple):
                kwargs[arg[0]] = arg[1]
            else:
                args.append(arg)
        return (args, kwargs)

    number = lambda self, items: int(items[0])
    quoted_string = lambda self, items: ast.literal_eval(items[0].value)

    def start(self, items) -> list[Glyph]:
        return sum(items, [])

    def unicode_definition(self, items):
        codepoint, char = items
        char = char.value
        decoded_codepoint = chr(int(codepoint, 16))
        assert decoded_codepoint == char, (
            f"Mismatching unicode codepoint for {char}: "
            f"U+{ord(decoded_codepoint):04X} != U+{ord(char):04X}"
        )
        return decoded_codepoint

    def include(self, items):
        return _read_mapping(self.ctx, filename=items[0])

    def command(self, items):
        text, glyph_class, source, *modifiers = items
        return [
            Glyph(
                text=text,
                glyph_class=glyph_class,
                source=source,
                modifiers=modifiers,
            )
        ]

    def source_func(self, items):
        func, args, kwargs = items[0]
        return func(self.ctx, *args, **kwargs)

    grid_sprite_func = lambda self, items: (GridSpriteSource, *items[0])
    manual_sprite_func = lambda self, items: (ManualSpriteSource, *items[0])
    image_func = lambda self, items: (ImageSource, *items[0])
    combine_func = lambda self, items: (CombineSource, *items[0])
    link_func = lambda self, items: (LinkSource, *items[0])

    def modifier_func(self, items):
        func, args, kwargs = items[0]
        return func(*args, **kwargs)

    expand_func = lambda self, items: (ExpandModifier, *items[0])
    translate_func = lambda self, items: (TranslateModifier, *items[0])


def _read_mapping(ctx, filename: str = "mapping.txt") -> list[Glyph]:
    """Read and parse the .txt mapping file."""
    text = (ctx.input_dir / filename).read_text()
    tree = GLYPH_GRAMMAR.parse(text)
    results = GlyphParser(ctx).transform(tree)
    return results


def reindex_sprites(glyphs: list[Glyph]) -> None:
    """Assign linearly sprite indices to sprites that do not have them."""
    sources = [g.source for g in glyphs if g.source.has_own_index]
    index = max((s.index for s in sources if s.index is not None), default=-1)
    for source in sources:
        if source.index is None:
            index += 1
            source.index = index
    indices = [-1 if (v := source.index) is None else v for source in sources]
    assert len(set(indices)) == len(indices), "Duplicate sprite indices found!"
    assert sorted(indices)[-1] + 1 == len(
        indices
    ), "Found gaps in sprite indices!"


def resolve_links(glyphs: list[Glyph]) -> None:
    """Resolve glyph sources so that they know of their source sprites."""
    glyph_map = {glyph.text: glyph for glyph in glyphs}
    for glyph in glyphs:
        if isinstance(glyph.source, LinkSource):
            glyph.source = glyph_map[glyph.source.link_to].source
    for glyph in glyphs:
        if isinstance(glyph.source, CombineSource):
            glyph.source.glyph1_source = glyph_map[glyph.source.glyph1].source
            glyph.source.glyph2_source = glyph_map[glyph.source.glyph2].source


def apply_modifiers(glyphs: list[Glyph]) -> None:
    for glyph in glyphs:
        for modifier in glyph.modifiers:
            modifier.modify_glyph(glyph)


def get_glyph_map(input_dir: Path) -> list[Glyph]:
    glyphs = _read_mapping(ctx=ParserContext(input_dir=input_dir))
    reindex_sprites(glyphs)
    resolve_links(glyphs)
    apply_modifiers(glyphs)
    return glyphs
