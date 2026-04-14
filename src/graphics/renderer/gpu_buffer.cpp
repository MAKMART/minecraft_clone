module;
#include <gl.h>
#include <cassert>
module gpu_buffer;

import logger;
gpu_buffer::gpu_buffer(std::size_t size, GLbitfield flags, buffer_type type) noexcept
: m_size(size), m_flags(flags), m_type(type)
{
	glCreateBuffers(1, &m_id);
	glNamedBufferStorage(m_id, size, nullptr, flags);
}
gpu_buffer::gpu_buffer(std::size_t size, const void* data, GLbitfield flags, buffer_type type) noexcept
: m_size(size), m_flags(flags), m_type(type)
{
	glCreateBuffers(1, &m_id);
	glNamedBufferStorage(m_id, size, data, flags);
}

gpu_buffer::~gpu_buffer() noexcept { release(); }

gpu_buffer::gpu_buffer(gpu_buffer&& other) noexcept
: m_id(other.m_id), m_size(other.m_size),
	m_flags(other.m_flags), m_type(other.m_type),
	m_mapped_ptr(other.m_mapped_ptr)
{
	other.m_id = 0;
	other.m_mapped_ptr = nullptr;
	other.m_size = 0;
}

gpu_buffer& gpu_buffer::operator=(gpu_buffer&& other) noexcept {
	if (this != &other) {
		release();
		m_id = other.m_id;
		m_size = other.m_size;
		m_flags = other.m_flags;
		m_type = other.m_type;
		m_mapped_ptr = other.m_mapped_ptr;

		other.m_id = 0;
		other.m_mapped_ptr = nullptr;
		other.m_size = 0;
	}
	return *this;
}

void gpu_buffer::set_debug_name(std::string_view name) const noexcept {
	glObjectLabel(GL_BUFFER, m_id, static_cast<GLsizei>(name.size()), name.data());
}

void gpu_buffer::bind() const noexcept {
	glBindBuffer(static_cast<GLenum>(m_type), m_id);
}
void gpu_buffer::bind_base(GLuint slot) const noexcept {
  glBindBufferBase(static_cast<GLenum>(m_type), slot, m_id);
}
void gpu_buffer::sub_data(const void* data, std::size_t size, std::size_t offset) noexcept {
	assert(offset + size <= m_size);
  log::system_info("gpu_buffer", "buffer_type = {}", buffer_type_to_string(m_type));
  assert((m_flags & GL_DYNAMIC_STORAGE_BIT) && "The buffer wasn't created with GL_DYNAMIC_STORAGE_BIT flag, check buffer type");
	glNamedBufferSubData(m_id, offset, size, data);
}
void* gpu_buffer::map_range(std::size_t offset, std::size_t length, GLbitfield access) noexcept {
    assert(!m_mapped_ptr && "Buffer already mapped!");
	// assert(m_flags & (GL_MAP_READ_BIT | GL_MAP_WRITE_BIT));
	assert(offset + length <= m_size);
    m_mapped_ptr = glMapNamedBufferRange(m_id, offset, length, access);
    return m_mapped_ptr;
}
void gpu_buffer::recreate(std::size_t new_size) noexcept {
    // 🔴 Important: unmap BEFORE deleting
    if (m_mapped_ptr) {
        glUnmapNamedBuffer(m_id);
        m_mapped_ptr = nullptr;
    }

    if (m_id) {
        glDeleteBuffers(1, &m_id);
    }

    m_size = new_size;

    glCreateBuffers(1, &m_id);
    glNamedBufferStorage(m_id, m_size, nullptr, m_flags);
}
void gpu_buffer::flush(std::size_t offset, std::size_t length) noexcept {
	glFlushMappedNamedBufferRange(m_id, offset, length);
}
void* gpu_buffer::mapped() const noexcept { 
	assert(m_mapped_ptr && "Buffer not mapped!");
	return m_mapped_ptr; 
}
void gpu_buffer::release() noexcept {
	if (m_mapped_ptr) {
		glUnmapNamedBuffer(m_id);
		m_mapped_ptr = nullptr;
	}
	if (m_id) glDeleteBuffers(1, &m_id);
}
