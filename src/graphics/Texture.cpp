#include "graphics/Texture.h"
#include <stdexcept>
#include <string>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture() : ID(0), width(0), height(0) {}

Texture::Texture(const std::filesystem::path &path, GLenum format, GLenum wrapMode, GLenum filterMode) {

    static bool flipOnce = [](){
        stbi_set_flip_vertically_on_load(true);
        return true;
    }();


    glCreateTextures(GL_TEXTURE_2D, 1, &ID);

    // Load image using stb_image
    int channels = 0;
    std::string strPath = path.string();
    unsigned char *data = stbi_load(strPath.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (!data) {
	    log::system_error("Texture", "Failed to load texture: {}", path.string());
    }
    GLenum internalFormat = GL_RGBA8;
    GLenum dataFormat = GL_RGBA;

    // Allocate immutable storage
    int maxDim = std::max(width, height);
    int levels = 1;
    while ((maxDim >>= 1) > 0) ++levels;
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
// Move constructor
Texture::Texture(Texture &&other) noexcept
    : ID(other.ID), width(other.width), height(other.height) {
    other.ID = 0; // Take ownership, invalidate the old object
    other.width = 0;
    other.height = 0;
}

// Move assignment operator
Texture &Texture::operator=(Texture &&other) noexcept {
    if (this != &other) {
        if (ID != 0) {
            glDeleteTextures(1, &ID);
        }
        ID = other.ID;
        width = other.width;
        height = other.height;

        other.ID = 0; // Steal ownership, reset old
        other.width = 0;
        other.height = 0;
    }
    return *this;
}
Texture::~Texture() {
    if (ID != 0) {
        glDeleteTextures(1, &ID);
    }
}
bool Texture::createEmpty(int width, int height, GLenum internalFormat) {

    if (ID != 0) {
        glDeleteTextures(1, &ID);
        // ID = 0;
        // this->width = 0;
        // this->height = 0;
    }
    if (width <= 0 || height <= 0) {
	log::system_error("Texture", "Invalid texture dimensions: {} x {}", std::to_string(width), std::to_string(height));
    }

    glCreateTextures(GL_TEXTURE_2D, 1, &ID);
    if (ID == 0)
	    log::system_error("Texture", "Failed to create OpenGL texture");
    glTextureStorage2D(ID, 1, internalFormat, width, height);

    glTextureParameteri(ID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(ID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    this->width = width;
    this->height = height;
    return true;
}

void Texture::Bind(int unit) const {
    if (ID == 0) {
        log::system_warn("Texture", "Attempt to bind invalid texture ID 0 on unit {}", std::to_string(unit));
        return;
    }
    glBindTextureUnit(unit, ID);
}
void Texture::Unbind(int unit) {
    glBindTextureUnit(unit, 0); // Unbind from the last used unit
}
