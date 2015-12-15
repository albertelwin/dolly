
char const * BASIC_VERT_SRC = STRINGIFY_GLSL_SHADER(100,

attribute vec3 i_position;

uniform mat4 xform;

varying vec2 tex_coord;

void main() {
	gl_Position = xform * vec4(i_position, 1.0);
	tex_coord = i_position.xy + 0.5;
}

);
