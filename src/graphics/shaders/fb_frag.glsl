#version 460 core

in vec2 v_uv;
out vec4 frag_color;

uniform sampler2D color;
uniform sampler2D depth;
uniform float near_plane;
uniform float far_plane;
uniform float time;
uniform int toggle;
uniform mat4 curr_inv_view_proj;
uniform mat4 prev_view_proj;
uniform float scale;

float linearize_depth(float z) {
	z = 1.0 - z;	// I use reverse-Z for the depth buffer
	float ndc = z * 2.0 - 1.0; // convert [0,1] depth to NDC [-1,1]
	return (2.0 * near_plane * far_plane) / (far_plane + near_plane - ndc * (far_plane - near_plane));
}
vec2 compute_velocity(vec2 uv, sampler2D depth, mat4 curr_inv_view_proj, mat4 prev_view_proj)
{
    // 1️⃣ Sample depth
    float d = texture(depth, uv).r;

    // 2️⃣ Ignore background / sky
    if(d >= 0.9999)
        return vec2(0.0);

    // 3️⃣ Reconstruct world position
    vec4 ndc;
    ndc.xy = uv * 2.0 - 1.0;   // [0,1] → [-1,1]
    ndc.z  = (1.0 - d) * 2.0 - 1.0;    // depth → NDC
    ndc.w  = 1.0;

    vec4 world = curr_inv_view_proj * ndc;
    vec3 world_pos = world.xyz / world.w;

    // 4️⃣ Project into previous frame
    vec4 prev_clip = prev_view_proj * vec4(world_pos, 1.0);
    prev_clip /= prev_clip.w;

    vec2 prev_uv = prev_clip.xy * 0.5 + 0.5; // [-1,1] → [0,1]

    // 5️⃣ Screen-space velocity
    vec2 velocity = uv - prev_uv;

    return velocity;
}
vec4 motion_blur(int samples, vec2 velocity, vec2 uv) {
	vec4 col = vec4(0.0);
	// simple linear blur along the motion vector
	for(int i = 0; i < samples; i++)
	{
		float t = float(i) / float(samples - 1);
		vec2 sample_uv = uv - velocity * t; // move backward along velocity
		col += texture(color, sample_uv);
	}
	col /= float(samples);
	return col;
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

	vec2 velocity = compute_velocity(v_uv, depth, curr_inv_view_proj, prev_view_proj);

	vec2 vis = clamp(velocity * scale, -1.0, 1.0) * 0.5 + 0.5;
	float mag = clamp(length(velocity) * scale, 0.0, 1.0);
	if(toggle == 0) {
		frag_color = texture(color, v_uv);
		//frag_color = mix(frag_color, vec4(vis, 0.0, 1.0), 0.8);
		//frag_color = mix(frag_color, vec4(vec3(mag), 1.0), 0.5);
		frag_color = motion_blur(16, velocity * scale, v_uv);

	} else {
		frag_color = vec4(vec3(depth0), 1.0f);
	}

	/*	vignette effect */
	vec2 center = vec2(0.5, 0.5);
	float dist = distance(v_uv, center);
	frag_color.rgb *= smoothstep(0.8, 0.5, dist); // darkens edges

	//frag_color.rgb = floor(frag_color.rgb * 8.0) / 8.0; // Pixelation with 8 levels per channel


	//vec2 offset = sin(v_uv.yx * 5.0 + time) * 0.01;
	//frag_color = texture(color, v_uv + offset);
}

