#version 460 core

out vec4 FragColor;

in vec2 uv;

layout(binding = 0) uniform sampler2D myTexture;

void main() {
	if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
		discard;
	vec2 nuv = (uv + vec2(2, 1)) * (1.0f / 16.0f);
	FragColor = texture(myTexture, nuv);
}
