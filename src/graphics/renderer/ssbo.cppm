module;
#include <gl.h>
#include <cassert>
export module ssbo;
import std;
import gpu_buffer;

export class dynamic_ssbo : public gpu_buffer {
public:
    explicit dynamic_ssbo(std::size_t size) noexcept
        : gpu_buffer(size, GL_DYNAMIC_STORAGE_BIT, buffer_type::ssbo) {}

    void upload(const void* data, std::size_t size, std::size_t offset = 0) noexcept {
        sub_data(data, size, offset);
    }
    void resize(std::size_t new_size) noexcept {
      recreate(new_size);
    }
};

export class persistent_ssbo : public gpu_buffer {
public:
    explicit persistent_ssbo(std::size_t size) noexcept
        : gpu_buffer(size,
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT,
            buffer_type::ssbo)
    {
        map_range(0, size, flags()); // reuse flags
        assert(flags()  & GL_MAP_PERSISTENT_BIT);
    }

    template<typename T>
    T* data() noexcept {
        return reinterpret_cast<T*>(mapped());
    }

    // Optional: explicit flush (for non-coherent setups later)
    void flush_range(std::size_t offset, std::size_t length) noexcept {
        flush(offset, length);
    }
};
export class gpu_only_ssbo : public gpu_buffer {
public:
    explicit gpu_only_ssbo(std::size_t size) noexcept
        : gpu_buffer(size, 0, buffer_type::ssbo) {}

    // Intentionally no upload, no mapping
};
