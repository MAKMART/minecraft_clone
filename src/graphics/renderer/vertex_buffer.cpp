#include "graphics/renderer/vertex_buffer.hpp"

VB::VB(const void* data, std::size_t size, GLbitfield flags) noexcept
: m_size(size)
{
	glCreateBuffers(1, &m_id);
	glNamedBufferStorage(m_id, size, data, flags);
}
VB VB::Immutable(const void* data, std::size_t size) {
	return VB(data, size, 0);                                   // never updated
}

VB VB::Dynamic(const void* data, std::size_t size) {
	return VB(data, size, GL_DYNAMIC_STORAGE_BIT);              // glNamedBufferSubData
}

VB VB::PersistentWrite(const void* data, std::size_t size) {
	constexpr GLbitfield f = GL_DYNAMIC_STORAGE_BIT |
		GL_MAP_WRITE_BIT |
		GL_MAP_PERSISTENT_BIT |
		GL_MAP_COHERENT_BIT;
	return VB(data, size, f);                                   // zero-copy streaming
}

VB::VB(VB&& other) noexcept : m_id(other.m_id), m_size(other.m_size) {
	other.m_id = 0; other.m_size = 0;
}

VB& VB::operator=(VB&& other) noexcept {
	if (this != &other) {
		release();
		m_id = other.m_id; m_size = other.m_size;
		other.m_id = 0; other.m_size = 0;
	}
	return *this;
}
VB::~VB() { 
	release(); 
}

[[nodiscard]] GLuint VB::id() const noexcept {
	return m_id; 
}
[[nodiscard]] std::size_t VB::size() const noexcept {
	return m_size; 
}

void VB::update_data(const void* data, std::size_t size, size_t offset) const noexcept {
	glNamedBufferSubData(m_id, offset, size, data);
}

void VB::release() noexcept {
	if (m_id) glDeleteBuffers(1, &m_id);
}
