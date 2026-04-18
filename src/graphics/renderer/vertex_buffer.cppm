module;
#include <gl.h>
export module vertex_buffer;

import std;
import gpu_buffer;

// 🔹 Dynamic Vertex Buffer — CPU can update via update()
export class vertex_buffer_dynamic : public gpu_buffer {
public:
    vertex_buffer_dynamic(std::size_t size) noexcept
        : gpu_buffer(size, GL_DYNAMIC_STORAGE_BIT, buffer_type::vertex) {}

    void update(const void* data, std::size_t size, std::size_t offset = 0) noexcept {
        sub_data(data, size, offset);
    }
};

// 🔹 Immutable Vertex Buffer — write once at creation
export class vertex_buffer_immutable : public gpu_buffer {
public:
    vertex_buffer_immutable(const void* data, std::size_t size) noexcept
        : gpu_buffer(size, data, 0, buffer_type::vertex) {}

    // ❌ No update() exposed
};

// 🔹 Persistent Vertex Buffer — memory-mapped, coherent
export class vertex_buffer_persistent : public gpu_buffer {
public:
    vertex_buffer_persistent(std::size_t size, const void* data = nullptr) noexcept
        : gpu_buffer(size, GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT, buffer_type::vertex)
    {
        m_ptr = map_range(0, size, flags());
        if (data) std::memcpy(m_ptr, data, size);
    }

    void* data() noexcept { return m_ptr; }

    void flush_range(std::size_t offset, std::size_t length) noexcept {
        flush(offset, length);
    }

private:
    void* m_ptr = nullptr;
};
