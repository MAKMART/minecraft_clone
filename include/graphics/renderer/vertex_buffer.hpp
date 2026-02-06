#pragma once
#include <glad/glad.h>
#include <cstddef>

class VB {
public:
    VB() = default;

    // Core modern constructor
    VB(const void* data, std::size_t size, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) noexcept
        : m_size(size)
    {
        glCreateBuffers(1, &m_id);
        glNamedBufferStorage(m_id, size, data, flags);
    }

    // Convenience factories (highly recommended)
    static VB Immutable(const void* data, std::size_t size) {
        return VB(data, size, 0);                                   // never updated
    }

    static VB Dynamic(const void* data, std::size_t size) {
        return VB(data, size, GL_DYNAMIC_STORAGE_BIT);              // glNamedBufferSubData
    }

    static VB PersistentWrite(const void* data, std::size_t size) {
        constexpr GLbitfield f = GL_DYNAMIC_STORAGE_BIT |
                                 GL_MAP_WRITE_BIT |
                                 GL_MAP_PERSISTENT_BIT |
                                 GL_MAP_COHERENT_BIT;
        return VB(data, size, f);                                   // zero-copy streaming
    }

    // move semantics (unchanged)
    VB(const VB&) = delete;
    VB& operator=(const VB&) = delete;

    VB(VB&& other) noexcept : m_id(other.m_id), m_size(other.m_size) {
        other.m_id = 0; other.m_size = 0;
    }
    VB& operator=(VB&& other) noexcept {
        if (this != &other) {
            release();
            m_id = other.m_id; m_size = other.m_size;
            other.m_id = 0; other.m_size = 0;
        }
        return *this;
    }

    inline ~VB() { release(); }

    [[nodiscard]] inline GLuint id() const noexcept { return m_id; }
    [[nodiscard]] inline std::size_t size() const noexcept { return m_size; }

    void update_data(const void* data, std::size_t size, size_t offset = 0) const noexcept {
        glNamedBufferSubData(m_id, offset, size, data);
    }

private:
    GLuint m_id = 0;
    size_t m_size = 0;

    void release() noexcept {
        if (m_id) glDeleteBuffers(1, &m_id);
    }
};
