
char const * BASIC_VERT_SRC = STRINGIFY_GLSL_SHADER(100,

attribute vec2 i_position;
attribute vec2 i_tex_coord;
attribute vec4 i_color;

uniform mat3 transform;
uniform vec4 color;

varying vec2 tex_coord;
varying vec4 vert_color;

void main() {
	gl_Position = vec4(transform * vec3(i_position, 1.0), 1.0);
	tex_coord = i_tex_coord;
	vert_color = i_color * color;
}

);
