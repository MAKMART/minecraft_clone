#version 460 core

// Input from the vertex shader (make sure this matches the output name in the vertex shader!)
in vec2 TexCoord;
in vec3 DebugColor;

// Output color (to the framebuffer)
out vec4 FragColor;

uniform int debug;

layout(binding = 0) uniform sampler2D myTexture;

void main() {
    // Set the fragment color to the sampled texture color
    vec4 texColor = texture(myTexture, TexCoord);

    //FragColor = texColor;
    //FragColor = vec4(TexCoord, 0.0, 1.0);
    //vec2 pixelUV = fract(TexCoord * 16.0); // show inner tile
    //if (any(lessThan(pixelUV, vec2(0.0001))) || any(greaterThan(pixelUV, vec2(0.95))))
    //FragColor = vec4(1, 0, 0, 1); // red tile border
    //else
    if(debug != 0) {
	FragColor = vec4(DebugColor, 1.0);
    }else {
	if (texColor.a < 0.1)
	    discard;
	FragColor = texColor;
    }
}
