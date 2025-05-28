#version 460 core
layout (location = 0) in vec2 inPos;
layout (location = 1) in vec4 inColor;
layout (location = 2) in vec2 inTexCoord;

out vec4 vColor;
out vec2 TexCoord;

uniform mat4 uProjection;
uniform mat4 uModel;

void main() {
    vColor = inColor;
    TexCoord = inTexCoord;
    gl_Position = uProjection * uModel * vec4(inPos, 0.0, 1.0);
}
