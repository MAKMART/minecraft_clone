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

struct FaceGPU {
	uvec3 pos;
	uint face_id;
	uint block_type;
};

layout(std430, binding = 2) readonly buffer vertexPullBuffer
{
	FaceGPU faces[];
};

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform float time;

const vec3 facePositions[6][4] = vec3[6][4](
    // FRONT (+Z) face_id = 0
    vec3[4](
        vec3(0, 0, 1),
        vec3(1, 0, 1),
        vec3(1, 1, 1),
        vec3(0, 1, 1)
    ),
    // BACK (-Z) face_id = 1
    vec3[4](
        vec3(1, 0, 0),
        vec3(0, 0, 0),
        vec3(0, 1, 0),
        vec3(1, 1, 0)
    ),
    // LEFT (-X) face_id = 2
    vec3[4](
        vec3(0, 0, 0),
        vec3(0, 0, 1),
        vec3(0, 1, 1),
        vec3(0, 1, 0)
    ),
    // RIGHT (+X) face_id = 3
    vec3[4](
        vec3(1, 0, 1),
        vec3(1, 0, 0),
        vec3(1, 1, 0),
        vec3(1, 1, 1)
    ),
    // TOP (+Y) face_id = 4
    vec3[4](
        vec3(0, 1, 1),
        vec3(1, 1, 1),
        vec3(1, 1, 0),
        vec3(0, 1, 0)
    ),
    // BOTTOM (-Y) face_id = 5
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

  FaceGPU f = faces[index];
  uint face_id = min(f.face_id, 5u);
  uint block_type = f.block_type;

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
  }
  */

  vec3 position = vec3(f.pos) + facePositions[face_id][triangleCornerIndices[currVertexID]];

  //DebugColor = vec3(f.pos) / 16.0;

  
  // EFFECTS
  vec4 worldPos = model * vec4(position, 1.0);

  // WATER EFFECTS
  if (block_type == 5u || block_type == 4u) {
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
  vec2 localUV = vec2(0, 0);

  switch (currVertexID) {
      case 0: localUV = vec2(0.0, 0.0); break;
      case 1: localUV = vec2(1.0, 0.0); break;
      case 2: localUV = vec2(1.0, 1.0); break;
      case 3: localUV = vec2(0.0, 0.0); break;
      case 4: localUV = vec2(1.0, 1.0); break;
      case 5: localUV = vec2(0.0, 1.0); break;
  }

  uint u = 15;
  uint v = 15;

  switch (block_type) {
	  //case 0u: u = 0, v = 0; break;	//AIR
	  case 1u: u = 0, v = 0; break;	//DIRT
	  case 2u: 
			   switch (face_id) {
				   case 0u: u = 1, v = 0; break; // + Z
				   case 1u: u = 1, v = 0; break; // - Z
				   case 2u: u = 1, v = 0; break; // - X
				   case 3u: u = 1, v = 0; break; // + X
				   case 4u: u = 1, v = 1; break; // + Y
				   case 5u: u = 0, v = 0; break; // - Y
			   }
			   break;	//GRASS
	  case 3u: u = 0, v = 1; break;	//STONE
	  case 4u: u = 5, v = 0; break;	//LAVA
	  case 5u: u = 0, v = 4; break;	//WATER
	  case 6u: u = 2, v = 0; break;	//WOOD
	  case 7u: u = 0, v = 2; break;	//LEAVES
	  case 8u: u = 4, v = 0; break;	//SAND
	  case 9u: u = 6, v = 0; break;	//PLANKS
  }
  
  TexCoord = vec2(u, v) / cellSize + localUV / cellSize;
  //TexCoord = localUV / cellSize;
  gl_Position = projection * view * worldPos;
}
