module;
#include <cstring>
#include <glad/glad.h>
export module chunk_renderer;

import std;
import mesher;

static constexpr int BUFFER_SIZE = 5e8; // 500 mb
static constexpr uint32_t QUAD_SIZE = 8;
static constexpr int MAX_DRAW_COMMANDS = 100000;

struct BufferSlot {
  uint32_t startByte;
  uint32_t sizeBytes;
};

export struct DrawElementsIndirectCommand {
  uint32_t indexCount;    // (count) Quad count * 6
  uint32_t instanceCount; // 1
  uint32_t firstIndex;    // 0
  uint32_t baseQuad;      // (baseVertex) Starting index in the SSBO
  uint32_t baseInstance;  // Chunk x, y z, face index
};

struct BufferFit {
  uint32_t pos;
  uint32_t space;
  std::vector<BufferSlot>::iterator iter;
};

export class ChunkRenderer {
public:
  void init() {
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    glGenBuffers(1, &commandBuffer);
    glGenBuffers(1, &SSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);

	// Allocate buffer with persistent/coherent flags
	glBufferStorage(GL_SHADER_STORAGE_BUFFER, BUFFER_SIZE, nullptr,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

	// Map it once and keep the pointer
	ssbo_ptr = glMapBufferRange(
			GL_SHADER_STORAGE_BUFFER, 0, BUFFER_SIZE,
			GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
			);

    glGenBuffers(1, &IBO);
    int maxQuads = CS * CS * CS * 6;
    std::vector<uint32_t> indices;
    for (uint32_t i = 0; i < maxQuads; i++) {
      indices.push_back((i << 2) | 2u);
      indices.push_back((i << 2) | 0u);
      indices.push_back((i << 2) | 1u);
      indices.push_back((i << 2) | 1u);
      indices.push_back((i << 2) | 3u);
      indices.push_back((i << 2) | 2u);
    }
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(VAO);
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	glBufferStorage(
			GL_DRAW_INDIRECT_BUFFER,
			MAX_DRAW_COMMANDS * sizeof(DrawElementsIndirectCommand),
			nullptr,
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT
			);

	command_ptr = (DrawElementsIndirectCommand*)glMapBufferRange(
			GL_DRAW_INDIRECT_BUFFER,
			0,
			MAX_DRAW_COMMANDS * sizeof(DrawElementsIndirectCommand),
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT
			);
  };

  // TODO: Deallocate buffers
  ~ChunkRenderer() {
	  glBindBuffer(GL_SHADER_STORAGE_BUFFER, SSBO);
	  glUnmapBuffer(GL_SHADER_STORAGE_BUFFER); // unmap before deletion
	  glDeleteBuffers(1, &SSBO);
  };

  DrawElementsIndirectCommand getDrawCommand(int quadCount, uint32_t baseInstance) {
    unsigned int requestedSize = quadCount * QUAD_SIZE;
	BufferSlot slot;

    // Allocate at the end if possible
    if ((BUFFER_SIZE - allocationEnd) > requestedSize) {
      slot ={ allocationEnd, requestedSize };
      usedSlots.emplace_back(slot);
      allocationEnd += requestedSize;
      return createCommand(slot, baseInstance);
    }

    // Otherwise, iterate through and find gaps to allocate in
    // Slow!

    auto prev = usedSlots.begin();
    BufferFit bestFit;
    bool foundFit = false;

    unsigned int spaceInFront = usedSlots.front().startByte;
    if (spaceInFront >= requestedSize) {
      foundFit = true;
      bestFit = BufferFit({
        0,
        spaceInFront,
        usedSlots.begin()
      });
    }

    if (usedSlots.size() == 1) {
      slot = { usedSlots.back().startByte + usedSlots.back().sizeBytes, requestedSize };
      usedSlots.insert(usedSlots.end(), slot);
      return createCommand(slot, baseInstance);
    }

    for (auto it = usedSlots.begin() + 1; it != usedSlots.end(); ++it) {
      unsigned int pos = (*prev).startByte + (*prev).sizeBytes;
      unsigned int spaceBetween = (*it).startByte - pos;
      if (spaceBetween >= requestedSize && (!foundFit || spaceBetween < bestFit.space)) {
        foundFit = true;
        bestFit = BufferFit({
          pos,
          spaceBetween,
          it
        });
      }
      prev = it;
    }

    if (foundFit) {
      slot = { bestFit.pos, requestedSize };
      usedSlots.insert(bestFit.iter, slot);
      return createCommand(slot, baseInstance);
    }

    slot = { usedSlots.back().startByte + usedSlots.back().sizeBytes, requestedSize };
    usedSlots.insert(usedSlots.end(), slot);
    return createCommand(slot, baseInstance);
  };

  void removeDrawCommand(const DrawElementsIndirectCommand& command) {
	  for (auto it = usedSlots.begin(); it != usedSlots.end(); ++it) {
		  if ((*it).startByte == (command.baseQuad >> 2) * QUAD_SIZE) {
			  // merge with previous if adjacent
			  if (it != usedSlots.begin()) {
				  auto prev = it - 1;
				  if (prev->startByte + prev->sizeBytes == it->startByte) {
					  prev->sizeBytes += it->sizeBytes;
					  it = usedSlots.erase(it);
				  }
			  }

			  // merge with next if adjacent
			  if (it != usedSlots.end() - 1) {
				  auto next = it + 1;
				  if (it->startByte + it->sizeBytes == next->startByte) {
					  it->sizeBytes += next->sizeBytes;
					  usedSlots.erase(next);
				  }
			  }

			  usedSlots.erase(it);
			  return;
		  }
	  }
  }

  void buffer(const DrawElementsIndirectCommand& command, void* vertices) {
	  size_t offset = (command.baseQuad >> 2) * QUAD_SIZE;
	  size_t size = (command.indexCount / 6) * QUAD_SIZE;

	  std::memcpy(static_cast<uint8_t*>(ssbo_ptr) + offset, vertices, size);
  }

  inline void addDrawCommand(const DrawElementsIndirectCommand& command) {
    drawCommands.insert(drawCommands.end(), command);
  };

  void render() {
    int numCommands = drawCommands.size();

    if (numCommands == 0) {
      return;
    }

    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, commandBuffer);
	std::memcpy(command_ptr, drawCommands.data(), numCommands * sizeof(DrawElementsIndirectCommand));

    glBindVertexArray(VAO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, SSBO);

    glMultiDrawElementsIndirect(
      GL_TRIANGLES,
      GL_UNSIGNED_INT,
      (void*)0,
      numCommands,
      0
    );

    // clear but don't deallocate
    drawCommands.clear();
  };

private:
  DrawElementsIndirectCommand createCommand(BufferSlot& slot, uint32_t baseInstance) {
	  DrawElementsIndirectCommand cmd;
	  cmd.indexCount = (slot.sizeBytes / QUAD_SIZE) * 6;
	  cmd.instanceCount = 1;
	  cmd.firstIndex = 0;
	  cmd.baseQuad = (slot.startByte / QUAD_SIZE) << 2;
	  cmd.baseInstance = baseInstance;
	  return cmd;
  }

  unsigned int VAO = 0;
  unsigned int IBO = 0;
  unsigned int SSBO = 0;
  unsigned int commandBuffer = 0;

  void* ssbo_ptr = nullptr;
  DrawElementsIndirectCommand* command_ptr = nullptr;
  std::vector<BufferSlot> usedSlots;
  std::vector<DrawElementsIndirectCommand> drawCommands;
  uint32_t allocationEnd = 0;
};
