#version 460 core

// Input vertex data (from VBO)
layout(location = 0) in ivec3 aPos;       // Position of the vertex
layout(location = 1) in ivec2 aTexCoord;  // Texture coordinates of the vertex

// Output data (to fragment shader)
out vec2 TexCoord;

// Uniforms (for transformations)
uniform mat4 projection; // Projection matrix
uniform mat4 view;       // View matrix
uniform mat4 model;      // Model matrix
uniform float uTexScale;

void main() {
    // Convert the integer texture coordinates to float and divide by the scale factor.
    //TexCoord = vec2(aTexCoord / uTexScale);
    TexCoord = vec2(aTexCoord) / 10000;

    // Apply transformations (model, view, and projection matrices)
    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
