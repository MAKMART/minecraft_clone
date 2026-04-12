module;
#include <FastNoise/FastNoise.h>  // FastNoise2 API
export module noise_2;
import std;
import glm;
import mesher;
import utility;

export class Noise {
public:

  Noise() {
	  simplex = FastNoise::New<FastNoise::Simplex>();  
	  fractal = FastNoise::New<FastNoise::FractalFBm>();  
	  fractal->SetSource(simplex);
	  fractal->SetOctaveCount(5);
	  fractal->SetLacunarity(0.2f);
	  fractal->SetGain(0.5f);

  }

  void generateTerrainV1(uint8_t* voxels, uint64_t* opaqueMask, int seed) {
    // --- Generate a 3D uniform grid of noise ---
    std::vector<float> noiseData(CS_P3);
    fractal->GenUniformGrid3D(
      noiseData.data(),
      0.0f, 0.0f, 0.0f,   // offsets
      CS_P, CS_P, CS_P,  // counts
      0.02f, 0.02f, 0.02f, // step sizes (roughly corresponds to frequency)
      seed
    );

    // --- Now do voxel filling the old way ---
    for (int x = 0; x < CS_P; x++) {
      for (int y = CS_P - 1; y--;) {
        for (int z = 0; z < CS_P; z++) {
          float val = (noiseData[get_zxy_index(x, y, z)] + 1.0f) * 0.5f;

          if (val > glm::smoothstep(0.15f, 1.0f, float(y) / CS_P)) {
            int i = get_zxy_index(x, y, z);
            int i_above = get_zxy_index(x, y + 1, z);

            opaqueMask[(y * CS_P) + x] |= 1ULL << z;

            voxels[i] = (voxels[i_above] == 0 ? 3 : 2);
          }
        }
      }
    }
  }

  /*
  void generateTerrainV2(uint8_t* voxels, uint64_t* opaqueMask, int offsetX, int offsetZ, int seed)
  {
    std::vector<float> noiseData(CS_P3);
    fractal->GenUniformGrid3D(
      noiseData.data(),
      offsetX * float(CS_P), 0.0f, offsetZ * float(CS_P),
      CS_P, CS_P, CS_P,
      0.6f, 0.6f, 0.6f,
      seed
    );

    for (int x = 0; x < CS_P; x++) {
      for (int y = CS_P - 1; y--;) {
        for (int z = 0; z < CS_P; z++) {
          float val = noiseData[get_zxy_index(x, y, z)];

          int i = get_zxy_index(x, y, z);
          if (val > float(y) / CS_P) {
            int i_above = get_zxy_index(x, y + 1, z);
            opaqueMask[(y * CS_P) + x] |= 1ULL << z;
            voxels[i] = (voxels[i_above] ? 2 : 3);
          }
          else if (y < 25) {
            opaqueMask[(y * CS_P) + x] |= 1ULL << z;
            voxels[i] = 1;
          }
        }
      }
    }
  }
  */


  void generateTerrainV2(
		  uint8_t* voxels,
		  uint64_t* opaqueMask,
		  int chunkX,           // renamed for clarity
		  int chunkZ,
		  int seed)
  {
	  // ────────────────────────────────────────────────
	  //   1. World-space starting coordinate
	  // ────────────────────────────────────────────────
	  float worldStartX = chunkX * float(CS);     // ← important: CS, not CS_P
	  float worldStartZ = chunkZ * float(CS);

	  // ────────────────────────────────────────────────
	  //   2. Generate continuous noise grid
	  // ────────────────────────────────────────────────
	  std::vector<float> noiseData(CS_P * CS_P * CS_P);

	  fractal->GenUniformGrid3D(
			  noiseData.data(),
			  worldStartX, 0.0f, worldStartZ,           // world position
			  CS_P, CS_P, CS_P,                                 // grid size (with padding)
			  1.0f, 1.0f, 1.0f,                           // frequency / step size
			  seed
			  );

	  // Optional: scale amplitude / remap to more useful range
	  // You can multiply or remap here if needed
	  // for (auto& v : noiseData) v = (v + 1.0f) * 0.5f;   // → [0..1]

	  // ────────────────────────────────────────────────
	  //   3. Voxel filling logic (same as before)
	  // ────────────────────────────────────────────────
	  for (int lx = 0; lx < CS_P; lx++) {
		  for (int ly = CS_P - 1; ly >= 0; ly--) {     // ← better to go top→bottom
			  for (int lz = 0; lz < CS_P; lz++) {
				  int idx = get_zxy_index(lx, ly, lz);
				  float heightNoise = noiseData[idx];

				  // Simple height-based threshold
				  // You can make this more sophisticated later
				  bool shouldBeSolid = heightNoise > (float(ly) / float(CS_P));

				  if (shouldBeSolid) {
					  int idx_above = get_zxy_index(lx, ly + 1, lz);
					  opaqueMask[(ly * CS_P) + lx] |= (1ULL << lz);

					  // Simple surface / subsurface logic
					  if (voxels[idx_above] == 0) {
						  voxels[idx] = 3;           // grass / surface
					  } else {
						  voxels[idx] = 2;           // dirt / stone
					  }
				  }
				  // Fill bedrock / deep stone below sea level
				  else if (ly < 25) {
					  opaqueMask[(ly * CS_P) + lx] |= (1ULL << lz);
					  voxels[idx] = 1;               // stone
				  }
			  }
		  }
	  }
  }

  void gen_uniform_3d(std::vector<float>& voxels, float x, float y, float z, float grid_size, float step_size, int seed)
  {
	  fractal->GenUniformGrid3D(voxels.data(), x, y, z, grid_size, grid_size, grid_size, step_size, step_size, step_size, seed);
  }

  void generateWhiteNoiseTerrain(uint8_t* voxels, uint64_t* opaqueMask, int seed) {
    // FastNoise2 doesn’t have a simple white noise generator node,
    // but you can sample Simplex with low feature scale and treat it as pseudo‑white.
	  /*
    auto white = FastNoise::New<FastNoise::WhiteNoise>(); 
    white->SetSeed(seed);

    std::vector<float> noiseData(CS_P * CS_P * CS_P);
    white->GenUniformGrid3D(
      noiseData.data(),
      0.0f, 0.0f, 0.0f,
      CS_P, CS_P, CS_P,
      1.0f, 1.0f, 1.0f,
      seed
    );

    for (int x = 1; x < CS_P; x++) {
      for (int y = CS_P; y-- ;) {
        for (int z = 1; z < CS_P; z++) {
          float n = noiseData[get_zxy_index(x, y, z)];

          int i = get_zxy_index(x, y, z);
          if (n > 0.5f) opaqueMask[(y * CS_P) + x] |= 1ULL << z;
          if      (n > 0.8f) voxels[i] = 1;
          else if (n > 0.6f) voxels[i] = 2;
          else if (n > 0.5f) voxels[i] = 3;
        }
      }
    }
	*/
  }
private:
  FastNoise::SmartNode<FastNoise::Simplex> simplex;
  FastNoise::SmartNode<FastNoise::FractalFBm> fractal;

};
