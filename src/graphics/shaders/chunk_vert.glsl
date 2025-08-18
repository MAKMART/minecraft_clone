#version 460 core

#extension GL_ARB_shader_storage_buffer_object : enable

// Output data (to fragment shader)
out vec2 TexCoord;
out vec3 DebugColor;

const vec3 debugColor[6] = vec3[6](
    vec3(1, 0, 0), // front
    vec3(0, 1, 0), // back
    vec3(0, 0, 1), // left
    vec3(1, 1, 0), // right
    vec3(0, 1, 1), // top
    vec3(1, 0, 1)  // bottom
);


layout(std430, binding = 0) readonly buffer vertexPullBuffer
{
	uvec2 packedMeshData[];
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

const vec3 facePositions[6][4] = vec3[6][4](
    // FRONT (+Z)
    vec3[4](
        vec3(0, 0, 1),
        vec3(1, 0, 1),
        vec3(1, 1, 1),
        vec3(0, 1, 1)
    ),
    // BACK (-Z)
    vec3[4](
        vec3(1, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 1, 0)
    ),
    // LEFT (-X)
    vec3[4](
        vec3(0, 0, 0),
        vec3(0, 0, 1),
        vec3(0, 1, 1),
        vec3(0, 1, 0)
    ),
    // RIGHT (+X)
    vec3[4](
        vec3(1, 0, 1),
        vec3(1, 0, 0),
        vec3(1, 1, 0),
        vec3(1, 1, 1)
    ),
    // TOP (+Y)
    vec3[4](
        vec3(0, 1, 1),
        vec3(1, 1, 1),
        vec3(1, 1, 0),
        vec3(0, 1, 0)
    ),
    // BOTTOM (-Y)
    vec3[4](
        vec3(0, 0, 0),
        vec3(1, 0, 0),
        vec3(1, 0, 1),
        vec3(0, 0, 1)
    )
);
const int triangleCornerIndices[6] = int[6](0, 1, 2, 0, 2, 3);


void main()
{
  const int currVertexID = gl_VertexID % 6;
  const int index = gl_VertexID / 6;
  const uint packedPosition = packedMeshData[index].x;
  const uint packedTexCoord = packedMeshData[index].y;
 
  uint x = packedPosition & 0x3FFu;
  uint y = (packedPosition >> 10) & 0x3FFu;
  uint z = (packedPosition >> 20) & 0x3FFu;

  uint u = packedTexCoord & 0x3FFu;
  uint v = (packedTexCoord >> 10) & 0x3FFu;
  uint face_id = (packedTexCoord >> 20) & 0x7u;
  uint block_type = (packedTexCoord >> 23) & 0x1FFu; // 9 bits
  u = min(u, 15u);
  v = min(v, 15u);
  face_id = min(face_id, 5u);

  DebugColor = debugColor[face_id];
  //DebugColor = mix(debugColor[face_id], vec3(float(gl_VertexID % 6) / 6.0, 0.0, 1.0), 0.5);
  //DebugColor = vec3(float(face_id) / 6.0, 0.0, 1.0);

  /*
  switch(block_type) {
    case 0: DebugColor = vec3(0, 0, 0); break;
    case 1: DebugColor = vec3(0.46f, 0.33f, 0.17f); break;
    case 2: DebugColor = vec3(0.25f, 0.61f, 0.04f); break;
    case 3: DebugColor = vec3(0.72f, 0.69f, 0.61f); break;
    case 4: DebugColor = vec3(1f, 0.4f, 0f); break;
    case 5: DebugColor = vec3(0.14f, 0.54f, 0.85f); break;
    case 6: DebugColor = vec3(0.63f, 0.46f, 0.29f); break;
  }*/

  ivec3 blockPos = ivec3(x, y, z);

  vec3 position = vec3(blockPos) + facePositions[face_id][triangleCornerIndices[currVertexID]];

  

  // EFFECTS
  vec4 worldPos = model * vec4(position, 1.0);

  // WATER EFFECTS
  if (block_type == 5 || block_type == 4) {
      // Wobble parameters
      float wobbleStrength = 0.1;
      float wobbleSpeed = 2.0;
      float wobbleFrequency = 4.0;

      // Apply wobble (e.g., vertical wobble along Y based on X and Z)
      worldPos.y += sin(worldPos.x * wobbleFrequency + time * wobbleSpeed) * wobbleStrength;
      worldPos.y += cos(worldPos.z * wobbleFrequency + time * wobbleSpeed) * wobbleStrength;
  }

  // END



  const vec2 cellSize = vec2(16, 16);	//Size of each texture in the atlas

  // Compute local UV within the tile, based on gl_VertexID % 6
  vec2 localUV;

  switch (currVertexID) {
      case 0: localUV = vec2(0.0, 0.0); break;
      case 1: localUV = vec2(1.0, 0.0); break;
      case 2: localUV = vec2(1.0, 1.0); break;
      case 3: localUV = vec2(0.0, 0.0); break;
      case 4: localUV = vec2(1.0, 1.0); break;
      case 5: localUV = vec2(0.0, 1.0); break;
  }
  TexCoord = vec2(u, v) / cellSize + localUV / cellSize;
  gl_Position = projection * view * worldPos;
}
