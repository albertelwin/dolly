
char const * FONT_FRAG_SRC = STRINGIFY_GLSL_SHADER(100,

precision mediump float;

varying vec2 tex_coord;

// uniform vec3 color;
uniform sampler2D tex0;

void main() {
	gl_FragColor = vec4(1.0, 1.0, 1.0, texture2D(tex0, tex_coord).r);
}

);