#pragma once
#include <string>
#include <GL/glew.h>  // or your appropriate OpenGL header
#include <filesystem>

class Texture {
public:
    Texture(void);
    // Constructor for loading a texture from file.
    Texture(const std::string& path, GLenum format, GLenum wrapMode, GLenum filterMode);
    // Constructor for loading a texture from file with std::filesystem::path.
    Texture(const std::filesystem::path &path, GLenum format, GLenum wrapMode, GLenum filterMode);
    
    // Create an empty texture suitable for framebuffer attachments.
    // 'internalFormat' should be a sized internal format (e.g. GL_RGBA8 or GL_DEPTH24_STENCIL8).
    // 'pixelFormat' is the format of the pixel data (e.g. GL_RGBA).
    // 'dataType' is the type of the pixel data (e.g. GL_UNSIGNED_BYTE or GL_UNSIGNED_INT_24_8).
    bool createEmpty(int width, int height, GLenum internalFormat);
    
    ~Texture(void);

    void Bind(int textureUnitIndex);
    void Unbind(void) const;

    GLuint getID(void) const { return ID; }
    int getWidth(void) const { return width; }
    int getHeight(void) const { return height; }
    GLenum getBoundUnit(void) const { return lastBoundUnit; }

private:
    GLuint ID;
    GLenum lastBoundUnit = GL_TEXTURE0; // Track the last used texture unit
    int width, height;
};
