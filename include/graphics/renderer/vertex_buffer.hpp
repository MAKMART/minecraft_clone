#pragma once
#include <glad/glad.h>
#include <cstddef>

class VB {
public:
    enum class usage {
        static_draw,
        dynamic_draw,
        stream_draw
    };

    VB() = default;
    VB(const void* data, size_t size, usage u = usage::static_draw)
        : m_size(size)
    {
        glCreateBuffers(1, &m_id);
        glNamedBufferData(m_id, size, data, to_gl_usage(u));
    }

    // ❌ no copying
    VB(const VB&)            = delete;
    VB& operator=(const VB&) = delete;

    // ✅ move constructor
    VB(VB&& other) noexcept
	    : m_id(other.m_id), m_size(other.m_size)
	    {
		    other.m_id   = 0;
		    other.m_size = 0;
	    }

    // ✅ move assignment
    VB& operator=(VB&& other) noexcept {
	    if (this != &other) {
		    release();
		    m_id        = other.m_id;
		    m_size      = other.m_size;
		    other.m_id  = 0;
		    other.m_size = 0;
	    }
	    return *this;
    }
    ~VB() { release(); }


    GLuint id() const { return m_id; }
    size_t size() const { return m_size; }
    void update_data(const void* data, size_t size, size_t offset = 0) {
	    glNamedBufferSubData(m_id, offset, size, data);
    }

private:
    GLuint m_id;
    size_t m_size;

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
        return GL_STATIC_DRAW; // fallback
    }
};

