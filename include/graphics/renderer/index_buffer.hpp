#pragma once
#include <glad/glad.h>
#include <cstddef>

class IB {
public:
    IB() = default;

    // Core modern constructor
    IB(const unsigned int* data, size_t count, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) noexcept
        : m_count(count)
    {
        glCreateBuffers(1, &m_id);
        glNamedBufferStorage(m_id, count * sizeof(unsigned int), data, flags);
    }

    // Convenience factories
    static IB Immutable(const unsigned int* data, size_t count) {
        return IB(data, count, 0);
    }

    static IB Dynamic(const unsigned int* data, size_t count) {
        return IB(data, count, GL_DYNAMIC_STORAGE_BIT);
    }

    static IB PersistentWrite(const unsigned int* data, size_t count) {
        constexpr GLbitfield f = GL_DYNAMIC_STORAGE_BIT |
                                 GL_MAP_WRITE_BIT |
                                 GL_MAP_PERSISTENT_BIT |
                                 GL_MAP_COHERENT_BIT;
        return IB(data, count, f);
    }

    // move semantics (unchanged)
    IB(const IB&) = delete;
    IB& operator=(const IB&) = delete;

    IB(IB&& other) noexcept : m_id(other.m_id), m_count(other.m_count) {
        other.m_id = 0; other.m_count = 0;
    }
    IB& operator=(IB&& other) noexcept {
        if (this != &other) {
            release();
            m_id = other.m_id; m_count = other.m_count;
            other.m_id = 0; other.m_count = 0;
        }
        return *this;
    }

    inline ~IB() { release(); }

    [[nodiscard]] inline GLuint id() const noexcept { return m_id; }
	[[nodiscard]] inline std::size_t count() const noexcept { return m_count; }

    void update_data(const unsigned int* data, std::size_t count, size_t offset = 0) const noexcept {
        glNamedBufferSubData(m_id, offset * sizeof(unsigned int), count * sizeof(unsigned int), data);
    }

private:
    GLuint m_id = 0;
    size_t m_count = 0;

    void release() noexcept {
        if (m_id) glDeleteBuffers(1, &m_id);
    }
};
