// Draws one point per draw of a multi-draw. The slot in the high bits of
// gl_VertexID selects the pixel: uFirstSlot draws to the left pixel and the
// slot after it to the right one. The point is white where the low bits equal
// uExpected and the slot is one of the two, and black otherwise. The mesh
// batcher draws it at startup to check that gl_VertexID includes the base
// vertex of each draw.

#ifdef VERTEX

uniform int uExpected;
uniform int uFirstSlot;

flat out int vertMatch;

void main(void) {
    uint id = uint(gl_VertexID);
    int slot = int(id >> 24u);
    int index = int(id & 0xFFFFFFu);
    bool known = slot == uFirstSlot || slot == uFirstSlot + 1;
    vertMatch = known && index == uExpected ? 1 : 0;
    gl_Position = vec4(slot == uFirstSlot + 1 ? 0.5 : -0.5, 0.0, 0.0, 1.0);
}

#elif defined(FRAGMENT)

flat in int vertMatch;
out vec4 outColor;

void main(void) {
    outColor = vec4(float(vertMatch));
}

#endif
