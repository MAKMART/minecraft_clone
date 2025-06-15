#pragma once
#include <RmlUi/Core.h>
#ifdef DEBUG
#include <RmlUi/Debugger.h>
#endif
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/FileInterface.h>
#include <memory>
#include <unordered_map>
#include <GL/glew.h> // or glad
#include <vector>
#include "Core/Input.h"
#include "Shader.h"
#include "SystemInterfaceGLFW.h"
#include <filesystem>
#include "Texture.h"

class FileInterface : public Rml::FileInterface {
  public:
    Rml::FileHandle Open(const Rml::String &path) override {
        return (Rml::FileHandle)fopen(path.c_str(), "rb");
    }
    void Close(Rml::FileHandle file) override {
        fclose((FILE *)file);
    }
    size_t Read(void *buffer, size_t size, Rml::FileHandle file) override {
        return fread(buffer, 1, size, (FILE *)file);
    }
    bool Seek(Rml::FileHandle file, long offset, int origin) override {
        return fseek((FILE *)file, offset, origin) == 0;
    }
    size_t Tell(Rml::FileHandle file) override {
        return ftell((FILE *)file);
    }
};

class UI : public Rml::RenderInterface {
  public:
    UI(int width, int height, Shader *ui_shader, std::filesystem::path fontPath, std::filesystem::path docPath);
    ~UI();

    Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;
    void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;
    void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

    Rml::TextureHandle LoadTexture(Rml::Vector2i &texture_dimensions, const Rml::String &source) override;
    Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;
    void ReleaseTexture(Rml::TextureHandle texture) override;

    void EnableScissorRegion(bool enable) override;
    void SetScissorRegion(Rml::Rectanglei region) override;

    void SetViewportSize(int width, int height);

    void EnableClipMask(bool enable);

    void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation);

    void SetTransform(const Rml::Matrix4f *transform);

    Rml::LayerHandle PushLayer();

    void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters);

    void PopLayer();

    Rml::TextureHandle SaveLayerAsTexture();

    Rml::CompiledFilterHandle SaveLayerAsMaskImage();

    Rml::CompiledFilterHandle CompileFilter(const Rml::String &name, const Rml::Dictionary &parameters);

    void ReleaseFilter(Rml::CompiledFilterHandle filter);

    Rml::CompiledShaderHandle CompileShader(const Rml::String &name, const Rml::Dictionary &parameters);

    void RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture);

    void ReleaseShader(Rml::CompiledShaderHandle shader);

    void render(void);

    bool isShiftDown = false;
    bool isCtrlDown = false;
    bool isAltDown = false;
    bool isMetaDown = false;

    Rml::Context *context = nullptr;
    int GetKeyModifiers(void);
    Rml::Input::KeyIdentifier MapKey(int glfw_key);

    std::unique_ptr<FileInterface> fileInterface;
    std::unique_ptr<SystemInterface> systemInterface;

    bool clip_mask_enabled = false;

  private:
    struct Geometry {
        GLuint vao, vbo, ebo;
        GLsizei index_count;
    };

    Rml::ElementDocument *doc = nullptr;
    Shader *shader = nullptr;
    glm::mat4 projection = glm::mat4(1.0f);
    glm::mat4 model = glm::mat4(1.0f);
    std::unordered_map<Rml::CompiledGeometryHandle, Geometry> geometry_map;
    std::unordered_map<Rml::TextureHandle, Texture> texture_map;

    Rml::CompiledGeometryHandle next_geometry_handle = 1;
    Rml::TextureHandle next_texture_handle = 1;
    int viewport_width, viewport_height;
};
