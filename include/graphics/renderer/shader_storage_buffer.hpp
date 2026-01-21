#pragma once
#include <cstddef>

class SSBO {
public:
    enum class usage {
        static_draw,
        dynamic_draw,
        stream_draw
    };

    SSBO() = default;
    SSBO(const void* data, size_t size, usage u = usage::static_draw)
        : m_size(size)
    {
        glCreateBuffers(1, &m_id);
        glNamedBufferData(m_id, size, data, to_gl_usage(u));
    }

    // ❌ no copying
    SSBO(const SSBO&)            = delete;
    SSBO& operator=(const SSBO&) = delete;

    // ✅ move constructor
    SSBO(SSBO&& other) noexcept
	    : m_id(other.m_id), m_size(other.m_size)
	    {
		    other.m_id   = 0;
		    other.m_size = 0;
	    }

    // ✅ move assignment
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

    ~SSBO() { release(); }

    GLuint id() const { return m_id; }
    size_t size() const { return m_size; }

    // Bind the SSBO to a specific binding point for shaders
    void bind_to_slot(GLuint slot) const {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, slot, m_id);
    }

    // Update contents (DSA)
    void update_data(const void* data, size_t size, size_t offset = 0) {
		assert(offset + size <= m_size);
        glNamedBufferSubData(m_id, offset, size, data);
    }

private:
    GLuint m_id = 0;
    size_t m_size = 0;


    void release() {
	    if (m_id != 0) {
		    glDeleteBuffers(1, &m_id);
		    m_id = 0;
	    }
    }

    static GLenum to_gl_usage(usage u) {
        switch (u) {
            case usage::static_draw:  return GL_STATIC_DRAW;
            case usage::dynamic_draw: return GL_DYNAMIC_DRAW;
            case usage::stream_draw:  return GL_STREAM_DRAW;
        }
        return GL_STATIC_DRAW;
    }
};

