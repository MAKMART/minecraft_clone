#pragma once
#include <glad/glad.h>
#include <cstddef>

class SSBO {
public:
    SSBO() = default;
    SSBO(const void* data, std::size_t size, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) noexcept
        : m_size(size)
    {
        glCreateBuffers(1, &m_id);
        glNamedBufferStorage(m_id, size, data, flags);
    }
	// Convenience factories – use these everywhere
	static SSBO Immutable(const void* data, std::size_t size) {
		return SSBO(data, size, 0);                                 // truly static, never updated
	}

	static SSBO Dynamic(const void* data, std::size_t size) {
		return SSBO(data, size, GL_DYNAMIC_STORAGE_BIT);            // glNamedBufferSubData updates
	}

	static SSBO PersistentWrite(const void* data, std::size_t size) {
		constexpr GLbitfield flags = GL_DYNAMIC_STORAGE_BIT |
			GL_MAP_WRITE_BIT |
			GL_MAP_PERSISTENT_BIT |
			GL_MAP_COHERENT_BIT;
		return SSBO(data, size, flags);                             // zero-copy CPU streaming
	}

    // no copying
    SSBO(const SSBO&) noexcept = delete;
    SSBO& operator=(const SSBO&) const noexcept = delete;

    // move constructor
    SSBO(SSBO&& other) noexcept
	    : m_id(other.m_id), m_size(other.m_size)
	    {
		    other.m_id   = 0;
		    other.m_size = 0;
	    }

    // move assignment
    SSBO& operator=(SSBO&& other) noexcept {
	    if (this != &other) {
		    release();
		    m_id        = other.m_id;
		    m_size      = other.m_size;
		    other.m_id  = 0;
		    other.m_size = 0;
	    }
	    return *this;
    }

    inline ~SSBO() noexcept { release(); }

    [[nodiscard]] inline GLuint id() const noexcept { return m_id; }
	[[nodiscard]] inline std::size_t size() const noexcept { return m_size; }

    // Bind the SSBO to a specific binding point for shaders
    inline void bind_to_slot(GLuint slot) const noexcept {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, m_id);
    }

    // Update contents (DSA)
    void update_data(const void* data, std::size_t size, std::size_t offset = 0) const noexcept {
		assert(offset + size <= m_size);
        glNamedBufferSubData(m_id, offset, size, data);
    }

private:
    GLuint m_id = 0;
    size_t m_size = 0;

    void release() noexcept {
	    if (m_id != 0) {
		    glDeleteBuffers(1, &m_id);
		    m_id = 0;
	    }
    }
};
