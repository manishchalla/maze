#version 410

in  vec2 vuv;
in  vec2 vpos;

out vec4 pColor;

uniform sampler2D tex;

void main() {
	vec4 color = texture(tex,vpos*10.0f).rgba;
	if(color.a < 0.99){
		discard;
	}
	pColor = color;
}
