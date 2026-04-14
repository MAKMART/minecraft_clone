module;
#include <cassert>
#include <gl.h>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
module texture;

import logger;
import std;


Texture::Texture() noexcept : ID(0), width(0), height(0)
{
}

Texture::Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode) noexcept
{

	static bool flipOnce = []() {
		stbi_set_flip_vertically_on_load(true);
		return true;
	}();

	int desired_channels = 0;
	GLenum data_format = GL_NONE;
	GLenum internal_format = GL_NONE;

	switch (format) {
		case GL_RED:
			desired_channels = 1;
			data_format = GL_RED;
			internal_format = GL_R8;
			break;

		case GL_RGB:
			desired_channels = 3;
			data_format = GL_RGB;
			internal_format = GL_RGB8;
			break;

		case GL_RGBA:
			desired_channels = 4;
			data_format = GL_RGBA;
			internal_format = GL_RGBA8;
			break;

		default:
			log::system_error("Texture", "Unsupported texture format: {}", format);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &ID);

	// Load image using stb_image
	int channels_in_file = 0;
	unsigned char* data = stbi_load(
			path.string().c_str(),
			&width,
			&height,
			&channels_in_file,
			desired_channels
			);
	if (!data) {
		log::system_error("Texture", "Failed to load texture: {}", path.string());
		std::exit(1);
	}
	// Allocate immutable storage
	auto max_dim = std::max(width, height);
	assert(max_dim > 0);

	int levels = std::bit_width(static_cast<unsigned>(max_dim));

	glTextureStorage2D(ID, levels, internal_format, width, height);

	// Upload texture data
	glTextureSubImage2D(ID, 0, 0, 0, width, height, data_format, GL_UNSIGNED_BYTE, data);

	// Generate mipmaps
	glGenerateTextureMipmap(ID);

	// Set texture parameters
	glTextureParameteri(ID, GL_TEXTURE_WRAP_S, wrapMode);
	glTextureParameteri(ID, GL_TEXTURE_WRAP_T, wrapMode);
	glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, filterMode);
	glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER,
			(filterMode == GL_NEAREST || filterMode == GL_NEAREST_MIPMAP_NEAREST) ? GL_NEAREST : GL_LINEAR);
	stbi_image_free(data);
}
// Move constructor
Texture::Texture(Texture&& other) noexcept
    : ID(other.ID), width(other.width), height(other.height)
{
	other.ID     = 0; // Take ownership, invalidate the old object
	other.width  = 0;
	other.height = 0;
}

// Move assignment operator
Texture& Texture::operator=(Texture&& other) noexcept
{
	if (ID != 0) {
		glDeleteTextures(1, &ID);
	}
	ID = std::exchange(other.ID, 0);
	width = std::exchange(other.width, 0);
	height = std::exchange(other.height, 0);
	return *this;
}
Texture::~Texture() noexcept
{
	if (ID != 0) {
		glDeleteTextures(1, &ID);
	}
}
bool Texture::createEmpty(int width, int height, GLenum internalFormat)
{

	if (ID != 0) {
		glDeleteTextures(1, &ID);
		// ID = 0;
		// this->width = 0;
		// this->height = 0;
	}
	if (width <= 0 || height <= 0) {
		log::system_error("Texture", "Invalid texture dimensions: {} x {}", std::to_string(width), std::to_string(height));
		std::exit(1);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &ID);
	if (ID == 0) {
		log::system_error("Texture", "Failed to create OpenGL texture");
		std::exit(1);
	}
	glTextureStorage2D(ID, 1, internalFormat, width, height);

	glTextureParameteri(ID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(ID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	this->width  = width;
	this->height = height;
	return true;
}

void Texture::Bind(int unit) const noexcept
{
	if (ID == 0) {
		log::system_warn("Texture", "Attempt to bind invalid texture ID 0 on unit {}", std::to_string(unit));
		return;
	}
	glBindTextureUnit(unit, ID);
}
GLuint Texture::getID() const noexcept
{
	if (ID == 0) {
		log::system_error("Texture", "Trying to get invalid ID == 0");
		return 0;
	}
	return ID;
}
int Texture::getWidth() const noexcept {
	assert(ID != 0 && "getHeight() on Texture with ID = 0");
	return width;
}
int Texture::getHeight() const noexcept
{
	assert(ID != 0 && "getHeight() on Texture with ID = 0");
	return height;
}
