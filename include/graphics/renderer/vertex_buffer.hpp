#pragma once
#include <glad/glad.h>
#include <cstddef>

class VB {
public:
    VB() = default;

    // Core modern constructor
    VB(const void* data, std::size_t size, GLbitfield flags = GL_DYNAMIC_STORAGE_BIT) noexcept;

    // Convenience factories (highly recommended)
    static VB Immutable(const void* data, std::size_t size);

	static VB Dynamic(const void* data, std::size_t size);
    static VB PersistentWrite(const void* data, std::size_t size);

    // move semantics (unchanged)
    VB(const VB&) = delete;
    VB& operator=(const VB&) = delete;

    VB(VB&& other) noexcept;
	VB& operator=(VB&& other) noexcept;

    ~VB();

    [[nodiscard]] GLuint id() const noexcept;
    [[nodiscard]] std::size_t size() const noexcept;

    void update_data(const void* data, std::size_t size, size_t offset = 0) const noexcept;

private:
    GLuint m_id = 0;
    size_t m_size = 0;

    void release() noexcept;
};
