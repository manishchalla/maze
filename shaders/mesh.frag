#version 440

in  vec2 vuv;

out vec4 pColor;

uniform sampler2D tex;


void main() {
	vec4 color = texture(tex,vuv).rgba;
	pColor = color;
}


