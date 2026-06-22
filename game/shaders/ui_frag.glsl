#version 460 core
in vec4 vColor;
in vec2 TexCoord;
layout (binding = 0) uniform sampler2D uTexture;
uniform int uHasTexture;    // Flag to check if texture is used
out vec4 fragColor;

void main() {
	if (uHasTexture == 1)
		fragColor = texture(uTexture, TexCoord) * vColor;
	else
		fragColor = vColor;
}
