#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
// layout(location = 2) in uint aType;

uniform mat4 viewProj;

// flat out uint vType;
out vec3 vColor;

void main()
{
    vColor = aColor;
    // vType = aType;
    gl_Position = viewProj * vec4(aPos, 1.0);
}
