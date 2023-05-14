#version 120

varying vec2 vertTexCoords;

uniform sampler2D tex0;

void main(void) {
    gl_FragColor = texture2D(tex0, vertTexCoords);
}
