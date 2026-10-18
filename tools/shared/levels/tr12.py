"""Reads the geometry of TR1 and TR2 levels from the PC and PlayStation
releases: the faces of every room and mesh, the models and static objects
that own the meshes, and the attribute of every object texture.
"""

from shared.levels.reader import Reader
from shared.levels.types import Face, FaceType, Format, Level, Model, StaticObject

PSX_TILE_SIZE = 256 * 256 // 2
PSX_CLUT_SIZE = 16 * 2
TR1_PSX_TILE_COUNT = 13
TR1_PSX_CLUT_COUNT = 1024


def _read_faces(
    reader: Reader, count: int, face_type: FaceType, vertex_count: int
) -> list[tuple[FaceType, list[int], int]]:
    faces = []
    for _ in range(count):
        indices = reader.u16s(vertex_count)
        texture = reader.u16()
        faces.append((face_type, indices, texture))
    return faces


def _number(faces: list[tuple[FaceType, list[int], int]], vertices) -> list[Face]:
    counts: dict[FaceType, int] = {}
    result = []
    for face_type, indices, texture in faces:
        index = counts.get(face_type, 0)
        counts[face_type] = index + 1
        result.append(
            Face(
                type=face_type,
                index=index,
                texture=texture & 0x7FFF,
                vertices=[vertices[i] for i in indices],
            )
        )
    return result


def _read_room(reader: Reader, fmt: Format) -> list[Face]:
    reader.skip(12)  # x, z, y bottom
    y_top = reader.s32()
    data_size = reader.u32()
    start = reader.pos
    if fmt == Format.TR1_PSX:
        reader.skip(2)

    vertex_count = reader.s16()
    vertices = []
    for _ in range(vertex_count):
        if fmt == Format.TR2_PSX:
            packed = reader.u32()
            vertices.append(
                (
                    ((packed >> 26) & 0x1F) << 10,
                    (((packed >> 21) & 0x1F) << 8) + y_top,
                    ((packed >> 16) & 0x1F) << 10,
                )
            )
        else:
            vertices.append(tuple(reader.s16s(3)))
            reader.skip(2)  # lighting
            if fmt == Format.TR2_PC:
                reader.skip(4)  # attributes, lighting

    if fmt == Format.TR2_PSX:
        # Quads keep their textures apart from their vertices, and both
        # vertex lists hold offsets four bytes to a vertex.
        reader.skip(2)
        quad_count = reader.s16()
        textures = reader.u16s(quad_count)
        if (reader.pos - start) % 4:
            reader.skip(2)
        raw = [
            (FaceType.TEXTURED_QUAD, [v >> 2 for v in reader.u16s(4)], tex)
            for tex in textures
        ]
        triangle_count = reader.s16()
        reader.skip(2)
        for _ in range(triangle_count):
            texture = reader.u16()
            indices = [v >> 2 for v in reader.u16s(3)]
            raw.append((FaceType.TEXTURED_TRIANGLE, indices, texture))
    else:
        raw = _read_faces(reader, reader.s16(), FaceType.TEXTURED_QUAD, 4)
        raw += _read_faces(reader, reader.s16(), FaceType.TEXTURED_TRIANGLE, 3)

    if fmt.is_psx:
        # The PlayStation stores quads as strips; reorder them into loops.
        for face_type, indices, _ in raw:
            if face_type == FaceType.TEXTURED_QUAD:
                indices[2], indices[3] = indices[3], indices[2]

    reader.pos = start + data_size * 2

    reader.skip(reader.u16() * 32)  # portals
    z_sectors = reader.u16()
    x_sectors = reader.u16()
    reader.skip(z_sectors * x_sectors * 8)
    reader.skip(2)  # ambient
    if fmt.is_tr2:
        reader.skip(4)  # second ambient, light mode

    light_size = {
        Format.TR1_PC: 18,
        Format.TR1_PSX: 20,
        Format.TR2_PC: 24,
        Format.TR2_PSX: 24,
    }[fmt]
    reader.skip(reader.u16() * light_size)

    mesh_size = {
        Format.TR1_PC: 18,
        Format.TR1_PSX: 20,
        Format.TR2_PC: 20,
        Format.TR2_PSX: 20,
    }[fmt]
    reader.skip(reader.u16() * mesh_size)
    reader.skip(4)  # alternate room, flags

    return _number(raw, vertices)


def _read_mesh(data: bytes, offset: int, fmt: Format) -> list[Face]:
    reader = Reader(data, offset)
    reader.skip(10)  # centre, collision radius
    vertex_count = reader.s16()

    if fmt.is_psx:
        # The sign of the vertex count says whether normals or shades
        # follow, and every vertex and normal takes eight bytes.
        has_normals = vertex_count > 0
        vertex_count = abs(vertex_count)
        vertices = []
        for _ in range(vertex_count):
            vertices.append(tuple(reader.s16s(3)))
            reader.skip(2)
        reader.skip(vertex_count * (8 if has_normals else 2))
        if fmt == Format.TR2_PSX and has_normals:
            reader.skip(reader.u16() * 12)
            reader.skip(reader.u16() * 10)
        raw = _read_faces(reader, reader.u16(), FaceType.TEXTURED_QUAD, 4)
        raw += _read_faces(reader, reader.u16(), FaceType.TEXTURED_TRIANGLE, 3)
        if fmt == Format.TR2_PSX:
            # Vertex indices count eight bytes to a vertex.
            raw = [(t, [v >> 3 for v in idx], tex) for t, idx, tex in raw]
    else:
        vertices = [tuple(reader.s16s(3)) for _ in range(vertex_count)]
        normal_count = reader.s16()
        reader.skip(normal_count * 6 if normal_count > 0 else -normal_count * 2)
        raw = _read_faces(reader, reader.s16(), FaceType.TEXTURED_QUAD, 4)
        raw += _read_faces(reader, reader.s16(), FaceType.TEXTURED_TRIANGLE, 3)
        raw += _read_faces(reader, reader.s16(), FaceType.COLOURED_QUAD, 4)
        raw += _read_faces(reader, reader.s16(), FaceType.COLOURED_TRIANGLE, 3)

    return _number(raw, vertices)


def _read_data_arrays(reader: Reader, level: Level) -> None:
    fmt = level.format
    reader.skip(4)
    room_count = reader.u16()
    level.room_faces = [_read_room(reader, fmt) for _ in range(room_count)]

    reader.skip(reader.u32() * 2)  # floor data
    mesh_data_size = reader.u32() * 2
    mesh_data = reader.data[reader.pos : reader.pos + mesh_data_size]
    reader.skip(mesh_data_size)
    mesh_pointers = reader.s32s(reader.u32())
    level.mesh_faces = [_read_mesh(mesh_data, ptr, fmt) for ptr in mesh_pointers]

    reader.skip(reader.u32() * 32)  # animations
    reader.skip(reader.u32() * 6)  # state changes
    reader.skip(reader.u32() * 8)  # anim ranges
    reader.skip(reader.u32() * 2)  # anim commands
    reader.skip(reader.u32() * 4)  # bones
    reader.skip(reader.u32() * 2)  # frames

    for _ in range(reader.u32()):
        object_id = reader.u16()
        reader.skip(2)
        mesh_count = reader.u16()
        mesh_start = reader.u16()
        reader.skip(10)  # bone index, frame offset, animation
        if fmt.is_psx:
            reader.skip(2)
        level.models.append(Model(object_id, mesh_start, mesh_count))

    for _ in range(reader.u32()):
        static_id = reader.u32()
        mesh_idx = reader.u16()
        reader.skip(26)  # boxes, flags
        level.statics.append(StaticObject(static_id, mesh_idx))


def _read_object_texture_attributes(reader: Reader, fmt: Format) -> list[int]:
    count = reader.u32()
    if fmt.is_psx:
        records = [reader.data[reader.pos + i * 16 : reader.pos + i * 16 + 16] for i in range(count)]
        return [int.from_bytes(rec[14:16], "little") for rec in records]
    return [Reader(reader.data, reader.pos + i * 20).u16() for i in range(count)]


def read_level(data: bytes, fmt: Format, is_cutscene: bool = False) -> Level:
    level = Level(format=fmt)
    reader = Reader(data)

    if fmt == Format.TR1_PC:
        reader.skip(4)
        reader.skip(reader.u32() * 256 * 256)
        _read_data_arrays(reader, level)
    elif fmt == Format.TR2_PC:
        reader.skip(4 + 768 + 1024)
        tile_count = reader.u32()
        reader.skip(tile_count * 256 * 256 * 3)
        _read_data_arrays(reader, level)
    elif fmt == Format.TR1_PSX:
        if is_cutscene:
            reader.pos = 8
        else:
            reader.pos = Reader(data, 16).u32() + 8
        reader.skip(TR1_PSX_TILE_COUNT * PSX_TILE_SIZE)
        reader.skip(TR1_PSX_CLUT_COUNT * PSX_CLUT_SIZE)
        _read_data_arrays(reader, level)
    elif fmt == Format.TR2_PSX:
        sample_count = reader.u32()
        offsets = [reader.u32() for _ in range(sample_count + 1)]
        reader.skip(offsets[-1] - offsets[0])
        _read_data_arrays(reader, level)
        reader.skip(reader.u32() * PSX_TILE_SIZE)
        clut_count = reader.u32()
        if clut_count > 1024:
            # The Japanese release keeps a kanji palette index in the low half.
            clut_count >>= 16
        reader.skip(clut_count * PSX_CLUT_SIZE + 4)

    level.texture_attributes = _read_object_texture_attributes(reader, fmt)
    return level
