#pragma once
#include <string>
#include <GL/glew.h> // or your appropriate OpenGL header
#include <filesystem>
#include <stdexcept>

class Texture {
  public:
    Texture(void);
    // Constructor for loading a texture from file.
    Texture(const std::string &path, GLenum format, GLenum wrapMode, GLenum filterMode);
    // Constructor for loading a texture from file with std::filesystem::path.
    Texture(const std::filesystem::path &path, GLenum format, GLenum wrapMode, GLenum filterMode);

    // Create an empty texture suitable for framebuffer attachments.
    // 'internalFormat' should be a sized internal format (e.g. GL_RGBA8 or GL_DEPTH24_STENCIL8).
    // 'pixelFormat' is the format of the pixel data (e.g. GL_RGBA).
    // 'dataType' is the type of the pixel data (e.g. GL_UNSIGNED_BYTE or GL_UNSIGNED_INT_24_8).
    bool createEmpty(int width, int height, GLenum internalFormat);

    ~Texture(void);

    void Bind(int unit) const;
    static void Unbind(int unit);

    GLuint getID(void) const {
        if (ID == 0)
            throw std::runtime_error("Trying to get a invalid ID == 0");
        else
            return ID;
    }
    int getWidth(void) const { return width; }
    int getHeight(void) const { return height; }

    // Delete copy semantics (prevent copying)
    Texture(const Texture &) = delete;
    Texture &operator=(const Texture &) = delete;

    // Implement move semantics (allow moving)
    Texture(Texture &&other) noexcept;
    Texture &operator=(Texture &&other) noexcept;

  private:
    GLuint ID;
    int width, height;
};
