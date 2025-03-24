#version 130

in  vec2 uv;
in  vec3 pos;

out vec2 vuv;

uniform mat4  modl_transform;
uniform mat4  view_transform;
uniform mat4  proj_transform;

void main() {
	
	vec4 model = vec4(pos,1);
	vec4 world = modl_transform * model;
	vec4 cam   = view_transform * world;
	vec4 proj  = proj_transform * cam;

	vuv = uv;
	gl_Position = proj;
}

