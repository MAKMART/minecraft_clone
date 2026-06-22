module;
#include <glad/gl.h>
#include <cassert>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
export module engine.renderer:gl_backend.texture;

import std;
import engine.core;

export class Texture {
  public:
    // WARN: Not sure whether this ctor is needed
    // Texture() noexcept = default;
    // Constructor for loading a texture from file.
    Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode) noexcept;

    // Delete copy semantics (prevent copying)
    Texture(const Texture&)            = delete;
    Texture& operator=(const Texture&) = delete;

    // Implement move semantics (allow moving)
    Texture(Texture&& other) noexcept;
    Texture& operator=(Texture&& other) noexcept;

    ~Texture() noexcept { glDeleteTextures(1, &ID); }

    // Create an empty texture suitable for framebuffer attachments.
    // 'internalFormat' should be a sized internal format (e.g. GL_RGBA8 or GL_DEPTH24_STENCIL8).
    // 'pixelFormat' is the format of the pixel data (e.g. GL_RGBA).
    // 'dataType' is the type of the pixel data (e.g. GL_UNSIGNED_BYTE or GL_UNSIGNED_INT_24_8).
    bool createEmpty(u32 width, u32 height, GLenum internalFormat);

    void Bind(u32 unit) const noexcept { glBindTextureUnit(unit, ID); }
    static void Unbind(u32 unit) noexcept { glBindTextureUnit(unit, 0); }

    [[nodiscard]] GLuint getID() const noexcept { return ID; }

    [[nodiscard]] u32 getWidth() const noexcept { return width; }
    [[nodiscard]] u32 getHeight() const noexcept { return height; }
  private:
    GLuint ID = 0;
    u32 width{};
    u32 height{};
};

using namespace engine::core;
Texture::Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode) noexcept {
	static bool flipOnce = []() {
		stbi_set_flip_vertically_on_load(true);
		return true;
	}();



	// Load image using stb_image
  u32 desired_channels = 0;
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
      logger::system_error("Texture", "Unsupported texture format: {}", format);
  }
  i32 channels_in_file = 0;
  i32 loaded_width{};
  i32 loaded_height{};
  unsigned char* data = ::stbi_load(
      path.string().c_str(),
      &loaded_width,
      &loaded_height,
      &channels_in_file,
      desired_channels
      );
  if (!data) {
    logger::system_error("Texture", "Failed to load texture: {}", path.string());
    std::exit(1);
  }
  width = static_cast<u32>(loaded_width);
  height = static_cast<u32>(loaded_height);
	// Allocate immutable storage
	u32 levels = std::bit_width(std::max(width, height));

	glCreateTextures(GL_TEXTURE_2D, 1, &ID);

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
bool Texture::createEmpty(u32 width, u32 height, GLenum internalFormat) {
	if (ID != 0) {
		glDeleteTextures(1, &ID);
		// ID = 0;
		// this->width = 0;
		// this->height = 0;
	}
	if (width <= 0 || height <= 0) {
		logger::system_error("Texture", "Invalid texture dimensions: {} x {}", std::to_string(width), std::to_string(height));
		std::exit(1);
	}

	glCreateTextures(GL_TEXTURE_2D, 1, &ID);
	if (ID == 0) {
		logger::system_error("Texture", "Failed to create OpenGL texture");
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
