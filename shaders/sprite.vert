#version 410

layout(location = 0) in  vec3  pos;
layout(location = 1) in  vec2  uv;

out vec2  vuv;

uniform vec2  scale;
uniform vec2  offset;
uniform vec2  cam_pos;
uniform float depth;

void main() {
	vec3 position = pos;
	position.xy *= scale;
	position.xy += offset;
	position.xy -= cam_pos;
	position.z = depth;
	vuv = uv;
	gl_Position = vec4(position,1);
}

