
char const * BASIC_VERT_SRC = STRINGIFY_GLSL_SHADER(100,

attribute vec3 i_position;
attribute vec2 i_tex_coord;

uniform mat4 xform;

varying vec2 tex_coord;

void main() {
	gl_Position = xform * vec4(i_position, 1.0);
	tex_coord = i_tex_coord;
}

);
