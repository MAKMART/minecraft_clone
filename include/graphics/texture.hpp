#pragma once
#include <string>
#include <glad/glad.h>
#include <filesystem>
#include <stdexcept>
#include "core/logger.hpp"

class Texture
{
      public:
	Texture();
	// Constructor for loading a texture from file.
	Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode);

	// Delete copy semantics (prevent copying)
	Texture(const Texture&)            = delete;
	Texture& operator=(const Texture&) = delete;

	// Implement move semantics (allow moving)
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	~Texture();

	// Create an empty texture suitable for framebuffer attachments.
	// 'internalFormat' should be a sized internal format (e.g. GL_RGBA8 or GL_DEPTH24_STENCIL8).
	// 'pixelFormat' is the format of the pixel data (e.g. GL_RGBA).
	// 'dataType' is the type of the pixel data (e.g. GL_UNSIGNED_BYTE or GL_UNSIGNED_INT_24_8).
	bool createEmpty(int width, int height, GLenum internalFormat);

	void        Bind(int unit) const;
	static void Unbind(int unit);

	const GLuint& getID() const
	{
		if (ID == 0)
			log::system_error("Texture", "Trying to get invalid ID == 0");
		return ID;
	}
	const int& getWidth() const
	{
		return width;
	}
	const int& getHeight() const
	{
		return height;
	}

      private:
	GLuint ID;
	int    width, height;
};
