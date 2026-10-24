// Draws one point that is white where gl_VertexID equals uExpected, and
// black otherwise. The mesh batcher draws it at startup to check that
// gl_VertexID includes the base vertex of the draw.

#ifdef VERTEX

uniform int uExpected;

flat out int vertMatch;

void main(void) {
    vertMatch = gl_VertexID == uExpected ? 1 : 0;
    gl_Position = vec4(0.0, 0.0, 0.0, 1.0);
}

#elif defined(FRAGMENT)

flat in int vertMatch;
out vec4 outColor;

void main(void) {
    outColor = vec4(float(vertMatch));
}

#endif
