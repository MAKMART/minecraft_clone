module;
#include <glad/glad.h>
export module index_buffer;

import std;
import gpu_buffer;

export template<typename T>
requires std::is_same_v<T, std::uint16_t> ||
         std::is_same_v<T, std::uint32_t> ||
         std::is_same_v<T, std::int32_t>
class index_buffer_dynamic : public gpu_buffer {
public:
    index_buffer_dynamic(std::size_t count) noexcept
        : gpu_buffer(count * sizeof(T), GL_DYNAMIC_STORAGE_BIT, buffer_type::index),
          m_count(count) {}

    void update(const T* data, std::size_t count, std::size_t offset = 0) noexcept {
        sub_data(data, count * sizeof(T), offset * sizeof(T));
    }

    [[nodiscard]] std::size_t count() const noexcept { return m_count; }

private:
    std::size_t m_count = 0;
};

export template<typename T>
class index_buffer_immutable : public gpu_buffer {
public:
    index_buffer_immutable(const T* data, std::size_t count) noexcept
        : gpu_buffer(count * sizeof(T), data, 0, buffer_type::index),
          m_count(count) {}

    [[nodiscard]] std::size_t count() const noexcept { return m_count; }

private:
    std::size_t m_count = 0;
};

export template<typename T>
class index_buffer_persistent : public gpu_buffer {
public:
    index_buffer_persistent(std::size_t count) noexcept
        : gpu_buffer(
            count * sizeof(T),
            GL_MAP_WRITE_BIT |
            GL_MAP_PERSISTENT_BIT |
            GL_MAP_COHERENT_BIT,
            buffer_type::index),
          m_count(count)
    {
        m_ptr = map_range(0, size(), flags());
    }

    T* data() noexcept {
        return reinterpret_cast<T*>(m_ptr);
    }

    void flush_range(std::size_t offset, std::size_t count) noexcept {
        flush(offset * sizeof(T), count * sizeof(T));
    }

    [[nodiscard]] std::size_t count() const noexcept { return m_count; }

private:
    void* m_ptr = nullptr;
    std::size_t m_count = 0;
};
