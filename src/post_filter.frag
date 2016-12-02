
char const * POST_FILTER_FRAG_SRC = STRINGIFY_GLSL_SHADER(100,

precision mediump float;

varying vec2 tex_coord;

uniform sampler2D tex0;

uniform float pixelate_scale;
uniform vec4 pixelate_dim;

uniform float brightness;

void main() {
	vec2 uv = tex_coord;
	//TODO: Remove this if-statement!!
	if(pixelate_scale < 1.0) {
		uv = floor(tex_coord * pixelate_dim.xy + 0.5) * pixelate_dim.zw;
	}

	gl_FragColor.rgb = texture2D(tex0, uv).rgb * brightness;
	gl_FragColor.a = 1.0;
}

);