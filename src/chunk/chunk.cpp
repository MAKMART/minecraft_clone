#include "chunk/chunk.hpp"
#include <cstdlib>

#include "core/timer.hpp"
#include "graphics/renderer/shader_storage_buffer.hpp"
#include "graphics/shader.hpp"
#include "core/defines.hpp"
#if defined(TRACY_ENABLE)
#include <tracy/Tracy.hpp>
#endif

Chunk::Chunk(const glm::ivec3& chunkPos)
    : position(chunkPos), logSizeX(std::log2(chunkSize.x)), logSizeY(std::log2(chunkSize.y))
{

	chunkData.resize(chunkSize.x * chunkSize.y * chunkSize.z); // Preallocate memory

	// Construct AABB
	glm::vec3 worldOrigin = chunkToWorld(position);
	glm::vec3 worldMax    = worldOrigin + glm::vec3(chunkSize);
	aabb                  = AABB(worldOrigin, worldMax);
	srand(static_cast<unsigned int>(position.x ^ position.y ^ position.z));
}
Chunk::~Chunk()
{
}
void Chunk::generate(std::span<const float> fullNoise, int regionWidth, int noiseOffsetX, int noiseOffsetZ)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	Timer generation_timer("Chunk::generate");

	constexpr int dirtDepth  = 3;
	constexpr int beachDepth = 1; // How wide the beach is vertically

	for (int z = 0; z < chunkSize.z; ++z) {
		for (int x = 0; x < chunkSize.x; ++x) {
			int noiseIndex = (noiseOffsetZ + z) * regionWidth + (noiseOffsetX + x);
#if defined(DEBUG)
			if (noiseIndex < 0 || noiseIndex >= static_cast<int>(fullNoise.size())) {
				log::system_warn("Chunk", "Noise index out of bounds: {}", noiseIndex);
				continue;
			}
#endif
			int height = static_cast<int>(fullNoise[noiseIndex] * chunkSize.y);
			height     = std::clamp(height, 0, chunkSize.y - 1);

			for (int y = 0; y < chunkSize.y; ++y) {
				int index = getBlockIndex(x, y, z);
				if (index == -1)
					continue;

				if (y > height) {
					chunkData[index].type = Block::blocks::AIR;
					continue;
				}

				if (y == height) {
					if (height <= seaLevel + beachDepth) {
						chunkData[index].type = Block::blocks::SAND; // Beach top
					} else {
						chunkData[index].type = Block::blocks::GRASS;
					}
				} else if (y >= height - dirtDepth) {
					if (height <= seaLevel + beachDepth) {
						chunkData[index].type = Block::blocks::SAND; // Beach body
					} else {
						chunkData[index].type = Block::blocks::DIRT;
					}
				} else {
					chunkData[index].type = Block::blocks::STONE;
				}
			}
		}
	}
}
void Chunk::genWaterPlane(std::span<const float> fullNoise, int regionWidth, int noiseOffsetX, int noiseOffsetZ)
{

#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	for (int z = 0; z < chunkSize.z; ++z) {
		for (int x = 0; x < chunkSize.x; ++x) {
			int noiseIndex = (noiseOffsetZ + z) * regionWidth + (noiseOffsetX + x);
			int height     = static_cast<int>(fullNoise[noiseIndex] * chunkSize.y);
			height         = std::clamp(height, 0, chunkSize.y - 1);

			int upperLimit = std::min(seaLevel, chunkSize.y - 1);
			for (int y = height + 1; y <= upperLimit; ++y) {
				int index = getBlockIndex(x, y, z);
				if (index == -1)
					continue;
				Block& block = chunkData[index];
				if (block.type == Block::blocks::AIR)
					setBlockAt(x, y, z, Block::blocks::WATER);
			}
		}
	}
}
void Chunk::uploadData()
{
	if (faces.empty() && waterFaces.empty()) {
		log::system_error("Chunk", "Empty combined face buffer for chunk ({}, {}, {})", position.x, position.y, position.z);
		return;
	}

	opaqueFaceCount      = static_cast<GLuint>(faces.size());
	transparentFaceCount = static_cast<GLuint>(waterFaces.size());

	std::vector<Face> combinedFaces;
	combinedFaces.reserve(faces.size() + waterFaces.size());
	combinedFaces.insert(combinedFaces.end(), faces.begin(), faces.end());
	combinedFaces.insert(combinedFaces.end(), waterFaces.begin(), waterFaces.end());

	ssbo = SSBO(combinedFaces.data(), combinedFaces.size() * sizeof(Face), SSBO::usage::dynamic_draw);
}

void Chunk::updateMesh()
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif
	faces.clear();
	for (int z = 0; z < chunkSize.z; ++z) {
		for (int x = 0; x < chunkSize.x; ++x) {
			for (int y = 0; y < chunkSize.y; ++y) {
				int index = getBlockIndex(x, y, z);
				if (index == -1)
					continue;

				const Block& block = chunkData[index];

				if (block.type == Block::blocks::AIR)
					continue;
				generateBlockFace(block, x, y, z);
			}
		}
	}
	uploadData();
}
bool Chunk::isFaceVisible(const Block& block, int x, int y, int z)
{
	return Block::isTransparent(getBlockAt(x, y, z).type);
}
void Chunk::generateBlockFace(const Block& block, int x, int y, int z)
{
#if defined(TRACY_ENABLE)
	ZoneScoped;
#endif

	auto pushFace = [this](int x, int y, int z, int u, int v, int face, int block_type) {
		this->faces.emplace_back(Face(x, y, z, u, v, face, block_type));
	};

	auto pushWaterFace = [this](int x, int y, int z, int u, int v, int face, int block_type) {
		this->waterFaces.emplace_back(Face(x, y, z, u, v, face, block_type));
	};

	/*
	        uint8_t visibilityMask =
	                (isFaceVisible(block, x, y, z + 1) << 0) | // Front Face
	                (isFaceVisible(block, x, y, z - 1) << 1) | // Back Face
	                (isFaceVisible(block, x - 1, y, z) << 2) | // Left Face
	                (isFaceVisible(block, x + 1, y, z) << 3) | // Right Face
	                (isFaceVisible(block, x, y + 1, z) << 4) | // Top Face
	                (isFaceVisible(block, x, y - 1, z) << 5);  // Bottom Face

	*/
	auto pushBlock = [pushFace, pushWaterFace /*, visibilityMask*/](int x, int y, int z, int topX, int topY, int sideX, int sideY, int botX, int botY, int block_type) {
		if (block_type == Block::toInt(Block::blocks::WATER)) {
			// if (visibilityMask & (1 << 0))
			pushWaterFace(x, y, z, sideX, sideY, 0, block_type);
			// if (visibilityMask & (1 << 1))
			pushWaterFace(x, y, z, sideX, sideY, 1, block_type);
			// if (visibilityMask & (1 << 2))
			pushWaterFace(x, y, z, sideX, sideY, 2, block_type);
			// if (visibilityMask & (1 << 3))
			pushWaterFace(x, y, z, sideX, sideY, 3, block_type);
			// if (visibilityMask & (1 << 4))
			pushWaterFace(x, y, z, topX, topY, 4, block_type);
			// if (visibilityMask & (1 << 5))
			pushWaterFace(x, y, z, botX, botY, 5, block_type);

		} else {
			// if (visibilityMask & (1 << 0))
			pushFace(x, y, z, sideX, sideY, 0, block_type);
			// if (visibilityMask & (1 << 1))
			pushFace(x, y, z, sideX, sideY, 1, block_type);
			// if (visibilityMask & (1 << 2))
			pushFace(x, y, z, sideX, sideY, 2, block_type);
			// if (visibilityMask & (1 << 3))
			pushFace(x, y, z, sideX, sideY, 3, block_type);
			// if (visibilityMask & (1 << 4))
			pushFace(x, y, z, topX, topY, 4, block_type);
			// if (visibilityMask & (1 << 5))
			pushFace(x, y, z, botX, botY, 5, block_type);
		}
	};

	int topX, topY, botX, botY, X, Y, topandBotX, topandBotY;
	switch (block.type) {
	case Block::blocks::AIR:
		return; // No geometry for AIR
	case Block::blocks::DIRT:
		X = 0, Y = 0;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
		break;
	case Block::blocks::GRASS:
		topX = 1;
		topY = 1;
		X    = 1;
		Y    = 0;
		botX = 0;
		botY = 0;

		pushBlock(x, y, z, topX, topY, X, Y, botX, botY, block.toInt());
		break;
	case Block::blocks::STONE:
		X = 0, Y = 1;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
		break;
	case Block::blocks::LAVA:
		X = 5, Y = 0;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
		break;
	case Block::blocks::WATER:
		X = 0, Y = 4;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
		break;
	case Block::blocks::WOOD:
		X          = 2;
		Y          = 0;
		topandBotX = 2;
		topandBotY = 1;
		pushBlock(x, y, z, topandBotX, topandBotY, X, Y, topandBotX, topandBotY, block.toInt());
		break;
	case Block::blocks::LEAVES:
		X = 0, Y = 2;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
		break;
	case Block::blocks::SAND:
		X = 4, Y = 0;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
		break;
	case Block::blocks::PLANKS:
		X = 6, Y = 0;
		pushBlock(x, y, z, X, Y, X, Y, X, Y, block.toInt());
	default:
		log::system_error("Chunk", "UNHANDLED BLOCK CASE: {} # {}", block.toString(), block.toInt());
		break;
	}
}
void Chunk::renderOpaqueMesh(const Shader& shader)
{
	shader.setMat4("model", getModelMatrix());
	if (!faces.empty()) {
		ssbo.bind_to_slot(0);
		DrawArraysWrapper(GL_TRIANGLES, 0, opaqueFaceCount * 6);
	}
}
void Chunk::renderTransparentMesh(const Shader& shader)
{
	shader.setMat4("model", getModelMatrix());
	if (!waterFaces.empty()) {
		ssbo.bind_to_slot(0);
		DrawArraysWrapper(GL_TRIANGLES, opaqueFaceCount * 6, transparentFaceCount * 6);
	}
}
