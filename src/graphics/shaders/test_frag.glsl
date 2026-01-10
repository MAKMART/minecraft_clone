#version 460 core

out vec4 FragColor;

in vec2 uv;

void main() {
	if(uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0)
		discard;
	FragColor = vec4(1.0, 0.0, 1.0, 1.0);
}
