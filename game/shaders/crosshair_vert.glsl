#version 460 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

uniform mat4 uProjection; // Updated to use a full projection matrix
uniform vec2 uCenter;
uniform float uSize;

out vec2 vTexCoord;

void main() {
	vec2 scaled = aPos.xy * uSize;
	vec2 pos = scaled + uCenter;
    gl_Position = uProjection * vec4(pos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
