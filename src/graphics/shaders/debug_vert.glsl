#version 460 core
layout(location = 0) in vec3 aPos;
uniform mat4 model;
uniform mat4 viewProj;
out vec3 fragColor;
uniform vec3 debugColor;
void main() {
    gl_Position = viewProj * model * vec4(aPos, 1.0);
    fragColor = debugColor;
}
