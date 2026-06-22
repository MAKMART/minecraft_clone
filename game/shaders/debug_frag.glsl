#version 460 core

in vec3 vColor;
// flat in uint vType;

out vec4 FragColor;

void main()
{
    FragColor = vec4(vColor, 1.0);
}
