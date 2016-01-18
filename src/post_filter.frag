
char const * POST_FILTER_FRAG_SRC = STRINGIFY_GLSL_SHADER(100,

precision mediump float;

varying vec2 tex_coord;

uniform sampler2D tex0;
uniform vec2 tex0_dim;

uniform float pixelate_scale;
uniform float fade_amount;

void main() {
	vec2 tex0_tex_coord = tex_coord;
	//TODO: Remove this if-statement!!
	if(pixelate_scale < 1.0) {
		vec2 low_res_dim = floor(tex0_dim * pixelate_scale);
		tex0_tex_coord = floor(tex_coord * low_res_dim + 0.5) / low_res_dim;		
	}

	vec3 tex0_color = texture2D(tex0, tex0_tex_coord).rgb;

	gl_FragColor = vec4(mix(tex0_color, vec3(0.0), fade_amount), 1.0);
}

);