#ifdef VERTEX

out vec2 vertTexCoords;

void main(void) {
    vec2 pos = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2));
    vertTexCoords = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}

#elif defined(FRAGMENT)

uniform sampler2D uTex0;

in vec2 vertTexCoords;
out vec4 outColor;

void main(void) {
    outColor = texture(uTex0, vertTexCoords);
}

#endif
