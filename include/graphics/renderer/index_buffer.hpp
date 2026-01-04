#pragma once
#include <glad/glad.h>
#include <cstddef>

class IB {
public:
    enum class usage {
        static_draw,
        dynamic_draw,
        stream_draw
    };

    IB() = default;
    IB(const unsigned int* data, size_t count, usage u = usage::static_draw)
        : m_count(count)
    {
        glCreateBuffers(1, &m_id);
        glNamedBufferData(m_id, count * sizeof(unsigned int), data, to_gl_usage(u));
    }

    // ❌ no copying
    IB(const IB&)            = delete;
    IB& operator=(const IB&) = delete;

    // ✅ move constructor
    IB(IB&& other) noexcept
	    : m_id(other.m_id), m_count(other.m_count)
	    {
		    other.m_id   = 0;
		    other.m_count = 0;
	    }

    // ✅ move assignment
    IB& operator=(IB&& other) noexcept {
	    if (this != &other) {
		    release();
		    m_id        = other.m_id;
		    m_count      = other.m_count;
		    other.m_id  = 0;
		    other.m_count = 0;
	    }
	    return *this;
    }
    ~IB() { release(); }

    GLuint id() const { return m_id; }
    size_t count() const { return m_count; }
    void update_data(const unsigned int* data, size_t count, size_t offset = 0) {
	    glNamedBufferSubData(m_id, offset * sizeof(unsigned int), count * sizeof(unsigned int), data);
    }


private:
    GLuint m_id;
    size_t m_count;

    
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
