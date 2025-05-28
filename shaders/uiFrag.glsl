#version 460 core
in vec4 vColor;
in vec2 TexCoord;
uniform sampler2D uTexture;
uniform int uHasTexture;    // Flag to check if texture is used
out vec4 FragColor;

void main() {
    if (uHasTexture == 0) {
	// If no texture is provided, use the color from the CSS
	FragColor = vColor; 
    } else {
	// Otherwise, sample the texture and apply the vertex color
	vec4 tex = texture(uTexture, TexCoord);
	FragColor = tex * vColor;
    }
}
