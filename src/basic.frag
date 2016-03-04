
char const * BASIC_FRAG_SRC = STRINGIFY_GLSL_SHADER(100,

precision mediump float;

varying vec2 tex_coord;
varying vec4 vert_color;

uniform vec4 color;
uniform sampler2D tex0;

void main() {
	gl_FragColor = texture2D(tex0, tex_coord) * vert_color * color;
}

);