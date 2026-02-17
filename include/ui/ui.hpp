#pragma once
#include "RmlUi/Core/Context.h"
#include <RmlUi/Core.h>
#if defined(DEBUG)
#include <RmlUi/Debugger.h>
#endif
#include <RmlUi/Core/RenderInterface.h>
#include <RmlUi/Core/FileInterface.h>
#include <memory>
#include <unordered_map>
#include <glad/glad.h>
#include <RmlUi/Core/Input.h>
#include "graphics/shader.hpp"
#include "ui/system_interface_glfw.hpp"
#include <filesystem>
#include "graphics/texture.hpp"
#include "graphics/renderer/vertex_buffer.hpp"
#include "graphics/renderer/index_buffer.hpp"
#include "graphics/renderer/framebuffer_manager.hpp"
#include <functional>

class LambdaEventListener : public Rml::EventListener {
public:
    using Callback = std::function<void(Rml::Event&)>;
    LambdaEventListener(Callback callback) : callback(std::move(callback)) {}

    // This is the function RmlUI calls when the event fires
    void ProcessEvent(Rml::Event& event) override {
        if (callback) callback(event);
    }

    // This ensures the object deletes itself when the UI element is destroyed
    void OnDetach(Rml::Element* /*element*/) override {
        delete this;
    }

private:
    Callback callback;
};

class FileInterface : public Rml::FileInterface
{
      public:
	Rml::FileHandle Open(const Rml::String& path) override
	{
		return (Rml::FileHandle)fopen(path.c_str(), "rb");
	}
	void Close(Rml::FileHandle file) override
	{
		fclose((FILE*)file);
	}
	size_t Read(void* buffer, size_t size, Rml::FileHandle file) override
	{
		return fread(buffer, 1, size, (FILE*)file);
	}
	bool Seek(Rml::FileHandle file, long offset, int origin) override
	{
		return fseek((FILE*)file, offset, origin) == 0;
	}
	size_t Tell(Rml::FileHandle file) override
	{
		return ftell((FILE*)file);
	}
};

class UI : public Rml::RenderInterface
{
	public:
	UI(int width, int height, std::filesystem::path fontPath) noexcept;
	Rml::Input::KeyIdentifier MapKey(int glfw_key) noexcept;
	Rml::ElementDocument* LoadDocument(const std::string& name, const std::filesystem::path& path) noexcept;
    void ShowDocument(const std::string& name, bool show = true) noexcept;
	void HideAllDocuments() noexcept {
		for(const auto& [name, doc] : documents)
			doc->Hide();
	}
    void CloseDocument(const std::string& name) noexcept;
	Rml::ElementDocument* GetDocument(const std::string& name) noexcept { return documents[name]; };
	int GetKeyModifiers() noexcept;
	void SetViewportSize(int width, int height) noexcept;
	void render() noexcept;

	[[nodiscard]] Rml::Context *get_context() const noexcept { return context; };

	~UI() noexcept;

	private:
	Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) override;

	void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;

	void ReleaseGeometry(Rml::CompiledGeometryHandle geometry) override;

	Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source) override;

	Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) override;

	void ReleaseTexture(Rml::TextureHandle texture) override;

	void EnableScissorRegion(bool enable) override {
		if (enable)
			glEnable(GL_SCISSOR_TEST);
		else
			glDisable(GL_SCISSOR_TEST);
	}
	void SetScissorRegion(Rml::Rectanglei region) override;


	void EnableClipMask(bool enable) override {
		clip_mask_enabled = enable;
		if (enable)
			glEnable(GL_STENCIL_TEST);
		else
			glDisable(GL_STENCIL_TEST);
	}

	void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) override;

	void SetTransform(const Rml::Matrix4f* transform) override;

	Rml::LayerHandle PushLayer() override;

	void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters) override;

	void PopLayer() override;

	Rml::TextureHandle SaveLayerAsTexture() override;

	Rml::CompiledFilterHandle SaveLayerAsMaskImage() override;

	Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) override;

	void ReleaseFilter(Rml::CompiledFilterHandle filter) override;

	Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) override;

	void RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) override;

	void ReleaseShader(Rml::CompiledShaderHandle shader) override;

	bool isShiftDown = false;
	bool isCtrlDown  = false;
	bool isAltDown   = false;
	bool isMetaDown  = false;

	Rml::Context* context = nullptr;

	std::unique_ptr<FileInterface>   fileInterface;
	std::unique_ptr<SystemInterface> systemInterface;

	bool clip_mask_enabled = false;

	struct Geometry {
		GLuint  vao;
		VB vbo;
		IB ibo;
		GLsizei index_count;
		// VB / IB destructors should handle their own glDeleteBuffers
		~Geometry() { if (vao) glDeleteVertexArrays(1, &vao); }
	};
	struct Layer
	{
		GLuint fbo = 0;
		GLuint color_texture = 0;
		int width = 0;
		int height = 0;
	};
	GLuint quad_vao = 0;
	VB quad_vbo;
	std::vector<GLuint> fbo_stack{100}; // To keep track of nested layers

	std::unordered_map<std::string, Rml::ElementDocument*> documents;
	std::unique_ptr<Shader> shader = nullptr;
	glm::mat4 projection = glm::mat4(1.0f);
	glm::mat4 model = glm::mat4(1.0f);

	std::unordered_map<Rml::CompiledGeometryHandle, Geometry> geometry_map;
	Rml::CompiledGeometryHandle next_geometry_handle = 1;

	std::unordered_map<Rml::LayerHandle, Layer> layers;
	Rml::LayerHandle next_layer_handle = 1;

	std::unordered_map<Rml::TextureHandle, Texture> texture_map;
	Rml::TextureHandle next_texture_handle  = 1;

	int viewport_width, viewport_height;
	uint8_t next_stencil_value = 1;
};
