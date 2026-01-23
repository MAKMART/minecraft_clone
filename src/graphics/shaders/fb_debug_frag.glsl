#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D color;
uniform sampler2D depth;
uniform float near_plane;
uniform float far_plane;
uniform float time;
uniform int toggle;

float linearize_depth(float z) {
	z = 1.0 - z;	// I use reverse-Z for the depth buffer
	float ndc = z * 2.0 - 1.0; // convert [0,1] depth to NDC [-1,1]
	return (2.0 * near_plane * far_plane) / (far_plane + near_plane - ndc * (far_plane - near_plane));
}
void main()
{
	float d = texture(depth, v_uv).r;
	if (d >= 0.9999) {
		frag_color = vec4(0.5,0.5,0.0,1.0);
		return;
	}
	float linear_depth = linearize_depth(d);
	float depth0 = (linear_depth - near_plane) / (far_plane - near_plane);
	depth0 = clamp(depth0, 0.0, 1.0);

	if(toggle == 0) {
		frag_color = texture(color, v_uv);
	} else {
		frag_color = vec4(vec3(depth0), 1.0f);
	}
}
