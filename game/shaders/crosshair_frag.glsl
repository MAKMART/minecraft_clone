#version 460 core

in vec2 vTexCoord; // Interpolated texture coordinates
out vec4 FragColor;

layout (binding = 2)  uniform sampler2D uTexture; // Texture sampler

void main()
{
    vec4 texColor = texture(uTexture, vTexCoord);
    if (texColor.a == 0.0) {
        discard;
    }
    //vec3 finalColor = vec3(1.0) - texColor.rgb;
    //FragColor = vec4(finalColor, texColor.a);
    FragColor = texColor;
}

