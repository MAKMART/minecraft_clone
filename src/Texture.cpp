#include "Texture.h"
#include <stdexcept>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <iostream>

Texture::Texture(void) : ID(0), width(0), height(0) {}

Texture::Texture(const std::string& path, GLenum format, GLenum wrapMode, GLenum filterMode) {
    glCreateTextures(GL_TEXTURE_2D, 1, &ID);

    // Load image using stb_image
    stbi_set_flip_vertically_on_load(true);
    int channels = 0;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    if (data) {
        GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
        
        // Allocate immutable storage
        glTextureStorage2D(ID, 1, internalFormat, width, height);
        
        // Upload texture data
        glTextureSubImage2D(ID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
        
        // Generate mipmaps
        glGenerateTextureMipmap(ID);
        
        // Set texture parameters
        glTextureParameteri(ID, GL_TEXTURE_WRAP_S, wrapMode);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_T, wrapMode);
        glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, filterMode);
        glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, filterMode);
    } else {
	std::string error = "Failed to load texture: " + path;
        throw std::runtime_error(error);
    }
    stbi_image_free(data);	// IMPORTANT: Dont forget
}
Texture::Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode) {
    glCreateTextures(GL_TEXTURE_2D, 1, &ID);

    // Load image using stb_image
    stbi_set_flip_vertically_on_load(true);
    int channels = 0;
    std::string strPath = path.string();
    unsigned char* data = stbi_load(strPath.c_str(), &width, &height, &channels, 0);
    if (data) {
        GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;
        
        // Allocate immutable storage
        glTextureStorage2D(ID, 1, internalFormat, width, height);
        
        // Upload texture data
        glTextureSubImage2D(ID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
        
        // Generate mipmaps
        glGenerateTextureMipmap(ID);
        
        // Set texture parameters
        glTextureParameteri(ID, GL_TEXTURE_WRAP_S, wrapMode);
        glTextureParameteri(ID, GL_TEXTURE_WRAP_T, wrapMode);
        glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, filterMode);
        glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, filterMode);
    } else {
	std::string error = "Failed to load texture: " + path.string();
        throw std::runtime_error(error);
    }
    stbi_image_free(data);
}

bool Texture::createEmpty(int width, int height, GLenum internalFormat) {
    if (ID != 0) {
        glDeleteTextures(1, &ID);
        ID = 0;
    }

    if (width <= 0 || height <= 0) {
	std::string error = "Invalid texture dimensions: " + std::to_string(width) + " x " + std::to_string(height);
	throw std::runtime_error(error);
        return false;
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &ID);
    glTextureStorage2D(ID, 1, internalFormat, width, height);
    
    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    this->width = width;
    this->height = height;
    return true;
}

Texture::~Texture(void) {
    if (ID) glDeleteTextures(1, &ID);
}

void Texture::Bind(int textureUnitIndex) {
    lastBoundUnit = GL_TEXTURE0 + textureUnitIndex;
    if(ID == 0)
	std::cerr << "Warning ! trying to bind to texture ID == 0" << std::endl;
    glBindTextureUnit(textureUnitIndex, ID);
}
void Texture::Unbind(void) const {
    glBindTextureUnit(lastBoundUnit - GL_TEXTURE0, 0);  // Unbind from the last used unit
}
