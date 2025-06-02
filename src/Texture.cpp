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
    if (!data) {
	throw std::runtime_error("Failed to load texture: " + path);
    }
    GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;

    // Allocate immutable storage
    int levels = 1 + (int)std::floor(std::log2(std::max(width, height)));
    glTextureStorage2D(ID, levels, internalFormat, width, height);

    // Upload texture data
    glTextureSubImage2D(ID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

    // Generate mipmaps
    glGenerateTextureMipmap(ID);

    // Set texture parameters
    glTextureParameteri(ID, GL_TEXTURE_WRAP_S, wrapMode);
    glTextureParameteri(ID, GL_TEXTURE_WRAP_T, wrapMode);
    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, filterMode);
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, filterMode);
    stbi_image_free(data);

}
Texture::Texture(const std::filesystem::path& path, GLenum format, GLenum wrapMode, GLenum filterMode) {
    glCreateTextures(GL_TEXTURE_2D, 1, &ID);

    // Load image using stb_image
    stbi_set_flip_vertically_on_load(true);
    int channels = 0;
    std::string strPath = path.string();
    unsigned char* data = stbi_load(strPath.c_str(), &width, &height, &channels, 0);
    if (!data) {
	throw std::runtime_error("Failed to load texture: " + path.string());
    }
    GLenum internalFormat = (channels == 4) ? GL_RGBA8 : GL_RGB8;

    // Allocate immutable storage
    int levels = 1 + (int)std::floor(std::log2(std::max(width, height)));
    glTextureStorage2D(ID, levels, internalFormat, width, height);

    // Upload texture data
    glTextureSubImage2D(ID, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);

    // Generate mipmaps
    glGenerateTextureMipmap(ID);

    // Set texture parameters
    glTextureParameteri(ID, GL_TEXTURE_WRAP_S, wrapMode);
    glTextureParameteri(ID, GL_TEXTURE_WRAP_T, wrapMode);
    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, filterMode);
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, filterMode);
    stbi_image_free(data);
}

bool Texture::createEmpty(int width, int height, GLenum internalFormat) {

    if (ID != 0) {
	glDeleteTextures(1, &ID);
	ID = 0;
	this->width = 0;
	this->height = 0;
    }
    if (width <= 0 || height <= 0) {
	std::string error = "Invalid texture dimensions: " + std::to_string(width) + " x " + std::to_string(height);
	throw std::runtime_error(error);
    }   

    glCreateTextures(GL_TEXTURE_2D, 1, &ID);
    if (ID == 0)
	throw std::runtime_error("Failed to create OpenGL texture");
    glTextureStorage2D(ID, 1, internalFormat, width, height);

    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    this->width = width;
    this->height = height;
    return true;
}

Texture::~Texture(void) {
    if (ID != 0) {
	glDeleteTextures(1, &ID);
	ID = 0; // Mark it as "dead"
    }
}

void Texture::Bind(int unit) const {
    if (ID == 0) {
	std::cerr << "[Texture] Warning: Attempt to bind invalid texture ID 0 on unit " << unit << std::endl;
	glBindTextureUnit(unit, 0); // Unbind
	return;
    }
    glBindTextureUnit(unit, ID);
}
void Texture::Unbind(int unit) {
    glBindTextureUnit(unit, 0);  // Unbind from the last used unit
}
