
char const * SCREEN_QUAD_VERT_SRC = STRINGIFY_GLSL_SHADER(100,

attribute vec2 i_position;

varying vec2 tex_coord;

void main() {
	gl_Position = vec4(i_position, 0.0, 1.0);
	tex_coord = (i_position + 1.0) * 0.5;
}

);