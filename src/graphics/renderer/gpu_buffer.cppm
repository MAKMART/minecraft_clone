module;
#include <gl.h>
export module gpu_buffer;

import std;

export enum class buffer_type : GLenum {
    vertex  = GL_ARRAY_BUFFER,
    index   = GL_ELEMENT_ARRAY_BUFFER,
    ssbo    = GL_SHADER_STORAGE_BUFFER,
    uniform = GL_UNIFORM_BUFFER,
};

constexpr std::string buffer_type_to_string(buffer_type type) {
  switch (type) {
    case buffer_type::vertex:
      return "GL_ARRAY_BUFFER";
      break;
    case buffer_type::index:
      return "GL_ELEMENT_ARRAY_BUFFER";
      break;
    case buffer_type::ssbo:    
      return "GL_SHADER_STORAGE_BUFFER";
      break;
    case buffer_type::uniform:
      return "GL_UNIFORM_BUFFER";
      break;
    default:
      return "UNKNOWN";
      break;
  }
}

export class gpu_buffer {
public:

    gpu_buffer(std::size_t size, GLbitfield flags, buffer_type type) noexcept;
    gpu_buffer(std::size_t size, const void* data, GLbitfield flags, buffer_type type) noexcept;
    ~gpu_buffer() noexcept;

    gpu_buffer(gpu_buffer&&) noexcept;
    gpu_buffer& operator=(gpu_buffer&&) noexcept;

    gpu_buffer(const gpu_buffer&) = delete;
    gpu_buffer& operator=(const gpu_buffer&) = delete;

    [[nodiscard]] GLuint id() const noexcept { return m_id; }
    [[nodiscard]] std::size_t size() const noexcept { return m_size; }

    void set_debug_name(std::string_view name) const noexcept;

    void bind() const noexcept;
    void bind_base(GLuint slot) const noexcept;

	// 🔒 Only derived classes can touch these
protected:
    // LOW LEVEL — no policy
    void sub_data(const void* data, std::size_t size, std::size_t offset = 0) noexcept;

    void* map_range(std::size_t offset, std::size_t length, GLbitfield access) noexcept;

    void unmap() noexcept {
      if (m_mapped_ptr) {
        glUnmapNamedBuffer(m_id);
        m_mapped_ptr = nullptr;
      }
    }

    void recreate(std::size_t new_size) noexcept;

    void flush(std::size_t offset, std::size_t length) noexcept;

    [[nodiscard]] void* mapped() const noexcept;
    [[nodiscard]] GLbitfield flags() const noexcept { return m_flags; }

private:
    void release() noexcept;

    GLuint m_id = 0;
    GLbitfield m_flags = 0;
    std::size_t m_size = 0;
    buffer_type m_type;
    void* m_mapped_ptr = nullptr;
};
