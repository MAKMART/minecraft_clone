#version 460 core
#extension GL_ARB_shader_storage_buffer_object : enable

out vec2 TexCoord;

layout(std430, binding = 1) readonly buffer vertexPullBuffer
{
    uvec2 packedMeshData[];
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 cubeSize;

const vec3 facePositions[6][4] = vec3[6][4](
    // FRONT (+Z)
    vec3[4](
        vec3(-0.5, -0.5,  0.5),
        vec3( 0.5, -0.5,  0.5),
        vec3( 0.5,  0.5,  0.5),
        vec3(-0.5,  0.5,  0.5)
    ),
    // BACK (-Z)
    vec3[4](
        vec3( 0.5, -0.5, -0.5),
        vec3(-0.5, -0.5, -0.5),
        vec3(-0.5,  0.5, -0.5),
        vec3( 0.5,  0.5, -0.5)
    ),
    // LEFT (-X)
    vec3[4](
        vec3(-0.5, -0.5, -0.5),
        vec3(-0.5, -0.5,  0.5),
        vec3(-0.5,  0.5,  0.5),
        vec3(-0.5,  0.5, -0.5)
    ),
    // RIGHT (+X)
    vec3[4](
        vec3( 0.5, -0.5,  0.5),
        vec3( 0.5, -0.5, -0.5),
        vec3( 0.5,  0.5, -0.5),
        vec3( 0.5,  0.5,  0.5)
    ),
    // TOP (+Y)
    vec3[4](
        vec3(-0.5,  0.5,  0.5),
        vec3( 0.5,  0.5,  0.5),
        vec3( 0.5,  0.5, -0.5),
        vec3(-0.5,  0.5, -0.5)
    ),
    // BOTTOM (-Y)
    vec3[4](
        vec3(-0.5, -0.5, -0.5),
        vec3( 0.5, -0.5, -0.5),
        vec3( 0.5, -0.5,  0.5),
        vec3(-0.5, -0.5,  0.5)
    )
);
const int triangleCornerIndices[6] = int[6](0, 1, 2, 0, 2, 3);

const vec2 faceSizes[6] = vec2[6](
    vec2(8, 8),  // Front
    vec2(8, 8),  // Back
    vec2(8, 8),  // Left
    vec2(8, 8),  // Right
    vec2(8, 4),  // Top
    vec2(8, 4)   // Bottom
);

void main() {
    const int currVertexID = gl_VertexID % 6;
    const int index = gl_VertexID / 6;
    const uint packedPosition = packedMeshData[index].x;
    const uint packedTexCoord = packedMeshData[index].y;

    // Unpack position
    float scale = 1.0 / 1023.0; // Adjust based on packing scale
    float x = float(packedPosition & 0x3FFu) * scale;
    float y = float((packedPosition >> 10) & 0x3FFu) * scale;
    float z = float((packedPosition >> 20) & 0x3FFu) * scale;
    float maxDim = max(max(cubeSize.x, cubeSize.y), cubeSize.z);
    vec3 faceCenter = vec3(x, y, z) * maxDim; // Scale to match original offset

    // Unpack tex_info
    uint u_base = packedTexCoord & 0x3FFu;
    uint v_base = (packedTexCoord >> 10) & 0x3FFu;
    uint face_id = (packedTexCoord >> 20) & 0x7u;
    face_id = min(face_id, 5u);

    // Compute localUV
    vec2 localUV;
    switch (gl_VertexID % 6) {
        case 0: localUV = vec2(0.0, 0.0); break;
        case 1: localUV = vec2(1.0, 0.0); break;
        case 2: localUV = vec2(1.0, 1.0); break;
        case 3: localUV = vec2(0.0, 0.0); break;
        case 4: localUV = vec2(1.0, 1.0); break;
        case 5: localUV = vec2(0.0, 1.0); break;
    }

    // Texture coordinates
    vec2 faceSizeUV = faceSizes[face_id] / 64.0;
    vec2 baseUV = vec2(u_base, v_base) / 64.0;
    TexCoord = baseUV + localUV * faceSizeUV;

    // Compute vertex position
    vec3 localPos = facePositions[face_id][triangleCornerIndices[currVertexID]];
    // Scale by cubeSize and center around faceCenter
    vec3 position = faceCenter + localPos * cubeSize;

    // Transform to world space
    vec4 worldPos = model * vec4(position, 1.0);
    gl_Position = projection * view * worldPos;
}
