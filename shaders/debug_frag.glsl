#version 460 core
in vec3 fragColor;
out vec4 FragColor;

void main() {
    //if (fragColor.a < 0.01)
//	discard;
    FragColor = vec4(fragColor, 1.0);
}
