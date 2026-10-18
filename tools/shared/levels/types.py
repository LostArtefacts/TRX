from dataclasses import dataclass, field
from enum import Enum


class Format(Enum):
    TR1_PC = "tr1_pc"
    TR2_PC = "tr2_pc"
    TR1_PSX = "tr1_psx"
    TR2_PSX = "tr2_psx"

    @property
    def is_psx(self) -> bool:
        return self in (Format.TR1_PSX, Format.TR2_PSX)

    @property
    def is_tr2(self) -> bool:
        return self in (Format.TR2_PC, Format.TR2_PSX)


class FaceType(Enum):
    # Matches FACE_TYPE in src/trx/game/inject/enum.h.
    TEXTURED_QUAD = 0
    TEXTURED_TRIANGLE = 1
    COLOURED_QUAD = 2
    COLOURED_TRIANGLE = 3


Vertex = tuple[int, int, int]


@dataclass
class Face:
    type: FaceType
    index: int
    texture: int
    vertices: list[Vertex]

    @property
    def loop(self) -> tuple[Vertex, ...]:
        # The vertices in order from the smallest, so that the same face
        # compares equal in both releases while a face on the same vertices
        # turned the other way does not.
        count = len(self.vertices)
        return min(
            tuple(self.vertices[k:] + self.vertices[:k]) for k in range(count)
        )


@dataclass
class Model:
    object_id: int
    mesh_start: int
    mesh_count: int


@dataclass
class StaticObject:
    static_id: int
    mesh_idx: int


@dataclass
class Level:
    format: Format
    room_faces: list[list[Face]] = field(default_factory=list)
    mesh_faces: list[list[Face]] = field(default_factory=list)
    models: list[Model] = field(default_factory=list)
    statics: list[StaticObject] = field(default_factory=list)
    texture_attributes: list[int] = field(default_factory=list)
