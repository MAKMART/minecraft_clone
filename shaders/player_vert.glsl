#version 460 core

layout(location = 0) in vec3 aPos;       // Position of the vertex

layout(location = 1) in vec2 aTexCoord;  // Texture coordinates of the vertex

out vec2 TexCoord;


uniform mat4 model;      // Model matrix
uniform mat4 view;       // View matrix (camera position/orientation)
uniform mat4 projection; // Projection matrix (perspective/ortho)

void main() {

    TexCoord = aTexCoord;

    gl_Position = projection * view * model * vec4(aPos, 1.0f);
}
