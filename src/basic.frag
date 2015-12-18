
char const * BASIC_FRAG_SRC = STRINGIFY_GLSL_SHADER(100,

precision mediump float;

varying vec2 tex_coord;

// uniform vec3 color;
uniform float tex_offset;
uniform sampler2D tex0;

void main() {
	gl_FragColor = texture2D(tex0, fract(tex_coord + vec2(tex_offset, 0.0)));
}

);