#version 460 core

// Input from the vertex shader (make sure this matches the output name in the vertex shader!)
in vec2 TexCoord;


// Output color (to the framebuffer)
out vec4 FragColor;


// Texture sampler (assigned in the application)
layout (binding = 1) uniform sampler2D texture2;

void main() {
    // Sample the texture color using the input texture coordinates
    vec4 TextureColor = texture(texture2, TexCoord);

    // If the sampled alpha is negative, discard the fragment
    if (TextureColor.a < 0.1) 
        discard;

    // Set the fragment color to the sampled texture color
    //FragColor = TextureColor;
    FragColor = vec4(0.0, 1.0, 0.0, 1.0);
}
