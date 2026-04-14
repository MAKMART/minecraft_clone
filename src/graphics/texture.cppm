module;
#include <gl.h>
export module texture;

import std;

export class Texture
{
      public:
	Texture() noexcept;
	// Constructor for loading a texture from file.
	Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode) noexcept;

	// Delete copy semantics (prevent copying)
	Texture(const Texture&)            = delete;
	Texture& operator=(const Texture&) = delete;

	// Implement move semantics (allow moving)
	Texture(Texture&& other) noexcept;
	Texture& operator=(Texture&& other) noexcept;

	~Texture() noexcept;

	// Create an empty texture suitable for framebuffer attachments.
	// 'internalFormat' should be a sized internal format (e.g. GL_RGBA8 or GL_DEPTH24_STENCIL8).
	// 'pixelFormat' is the format of the pixel data (e.g. GL_RGBA).
	// 'dataType' is the type of the pixel data (e.g. GL_UNSIGNED_BYTE or GL_UNSIGNED_INT_24_8).
	bool createEmpty(int width, int height, GLenum internalFormat);

	void Bind(int unit) const noexcept;
	static void Unbind(int unit) noexcept { glBindTextureUnit(unit, 0); }

	[[nodiscard]] GLuint getID() const noexcept;

	[[nodiscard]] int getWidth() const noexcept;
	[[nodiscard]] int getHeight() const noexcept;
      private:
	GLuint ID = 0;
	int width, height;
};
