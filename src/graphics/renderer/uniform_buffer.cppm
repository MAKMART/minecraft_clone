module;
#include <gl.h>
#include <cassert>
export module uniform_buffer;
import std;
import gpu_buffer;
class uniform_buffer_base : public gpu_buffer {
protected:
    uniform_buffer_base(std::size_t size, GLbitfield flags)
        : gpu_buffer(size, flags, buffer_type::uniform) {}

    void validate_alignment(std::size_t offset) noexcept {
        GLint alignment = 0;
        glGetIntegerv(GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT, &alignment);
        assert(offset % alignment == 0 && "UBO offset misaligned");
    }
};

export class uniform_buffer_persistent : public uniform_buffer_base {
public:
    uniform_buffer_persistent(std::size_t size)
        : uniform_buffer_base(
            size,
            GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT
        )
    {
        m_ptr = map_range(0, size, flags());
    }

    void write(const void* data, std::size_t size, std::size_t offset = 0) noexcept {
        validate_alignment(offset);
        std::memcpy(static_cast<char*>(m_ptr) + offset, data, size);
    }

    void bind(GLuint slot) const noexcept {
        bind_base(slot);
    }

private:
    void* m_ptr = nullptr;
};
