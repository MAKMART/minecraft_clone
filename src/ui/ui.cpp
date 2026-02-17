#include "ui/ui.hpp"
#include <cstddef>
#include <cstring>
#include <RmlUi/Core/Types.h>
#include "RmlUi/Debugger/Debugger.h"
#include "core/defines.hpp"
#include <GLFW/glfw3.h>
#include "graphics/shader.hpp"
#include "core/logger.hpp"

using namespace Rml::Input;
UI::UI(int width, int height, std::filesystem::path fontPath) noexcept : viewport_width(width), viewport_height(height)
{
	// System and file interfaces owned by UI (RAII)
	systemInterface = std::make_unique<SystemInterface>();
	fileInterface   = std::make_unique<FileInterface>();

	Rml::SetRenderInterface(this);
	Rml::SetSystemInterface(systemInterface.get());
	Rml::SetFileInterface(fileInterface.get());
	shader = std::make_unique<Shader>("UI", UI_VERTEX_SHADER_DIRECTORY, UI_FRAGMENT_SHADER_DIRECTORY);

	Rml::Initialise();

	context = Rml::CreateContext("main", {width, height});
	// TODO: FIX THIS
//#if defined(DEBUG)
	Rml::Debugger::SetContext(context);
	Rml::Debugger::Initialise(context);
	log::system_info("UI", "Rml::Debugger::Initialise should've run");
//#endif
	if (!Rml::LoadFontFace(fontPath.string())) {
		log::system_error("UI", "Failed to load font: {}", fontPath.string());
	}
	float quadVertices[] = {
		-1.0f,  1.0f,  0.0f, 1.0f,
		-1.0f, -1.0f,  0.0f, 0.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		-1.0f,  1.0f,  0.0f, 1.0f,
		1.0f, -1.0f,  1.0f, 0.0f,
		1.0f,  1.0f,  1.0f, 1.0f
	};
	glCreateVertexArrays(1, &quad_vao);
	quad_vbo = VB::Immutable(quadVertices, sizeof(quadVertices));
	glVertexArrayVertexBuffer(quad_vao, 0, quad_vbo.id(), 0, 4 * sizeof(float));
	glEnableVertexArrayAttrib(quad_vao, 0);
	glVertexArrayAttribFormat(quad_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
	glVertexArrayAttribBinding(quad_vao, 0, 0);
	glEnableVertexArrayAttrib(quad_vao, 1);
	glVertexArrayAttribFormat(quad_vao, 1, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float));
	glVertexArrayAttribBinding(quad_vao, 1, 0);
	shader->setInt("uTexture", 0);
}
Rml::ElementDocument* UI::LoadDocument(const std::string& name, const std::filesystem::path& path) noexcept
{
    auto doc = context->LoadDocument(path.string());
    if (!doc) {
        log::system_error("UI", "Failed to load document: {}", path.string());
        return nullptr;
    }
    documents[name] = doc;
    return doc;
}

void UI::ShowDocument(const std::string& name, bool show) noexcept
{
    auto it = documents.find(name);
    if (it != documents.end()) {
        if (show) it->second->Show();
        else it->second->Hide();
    }
}

void UI::CloseDocument(const std::string& name) noexcept
{
    auto it = documents.find(name);
    if (it != documents.end()) {
        it->second->Close();
        documents.erase(it);
    }
}
int UI::GetKeyModifiers() noexcept
{
	int modifiers = 0;
	if (isShiftDown)
		modifiers |= Rml::Input::KM_SHIFT;
	if (isCtrlDown)
		modifiers |= Rml::Input::KM_CTRL;
	if (isAltDown)
		modifiers |= Rml::Input::KM_ALT;
	if (isMetaDown)
		modifiers |= Rml::Input::KM_META;
	return modifiers;
}
void UI::SetViewportSize(int width, int height) noexcept
{
	viewport_width  = width;
	viewport_height = height;
	projection      = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
	glViewport(0, 0, width, height);
	shader->setMat4("uProjection", projection);
	context->SetDimensions({width, height});
	context->Update();
}
void UI::render() noexcept
{
	next_stencil_value = 1;               // ← crucial
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);      // optional if you want clean slate
	context->Update();
	context->Render();
}
UI::~UI() noexcept
{
	// No texture_map clearing needed since RmlUI handles it internally
	for (auto& [name, doc] : documents)
        doc->Close();
	documents.clear();
	geometry_map.clear();
	Rml::RemoveContext("main");
	context = nullptr;
	Rml::SetRenderInterface(nullptr);
	Rml::SetSystemInterface(nullptr);
	Rml::SetFileInterface(nullptr);
	Rml::Shutdown();
}

Rml::CompiledGeometryHandle UI::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	GLuint vao;
	glCreateVertexArrays(1, &vao);
	VB vbo = VB::Dynamic(vertices.data(), vertices.size() * sizeof(Rml::Vertex));
	IB ebo = IB::Dynamic(indices.data(), indices.size());

	glVertexArrayVertexBuffer(vao, 0, vbo.id(), 0, sizeof(Rml::Vertex));
	glEnableVertexArrayAttrib(vao, 0); // pos
	glEnableVertexArrayAttrib(vao, 1); // color
	glEnableVertexArrayAttrib(vao, 2); // texcoord

	glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Rml::Vertex, position));
	glVertexArrayAttribFormat(vao, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(Rml::Vertex, colour));
	glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Rml::Vertex, tex_coord));

	glVertexArrayAttribBinding(vao, 0, 0);
	glVertexArrayAttribBinding(vao, 1, 0);
	glVertexArrayAttribBinding(vao, 2, 0);
	glVertexArrayElementBuffer(vao, ebo.id());

	Rml::CompiledGeometryHandle handle = next_geometry_handle++;
	geometry_map.try_emplace(handle, vao, std::move(vbo), std::move(ebo), static_cast<GLsizei>(indices.size()));
	return handle;
}

void UI::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture)
{
	auto it = geometry_map.find(handle);
	if (it == geometry_map.end())
		return;

	// Setup shader uniforms, texture bind, etc. -- omitted for brevity
	projection = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
	// Combine the model matrix from SetTransform with the translation
	glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));
	glm::mat4 final_model = model * translation_matrix; // Apply SetTransform's model first, then translation

	shader->setMat4("uModel", final_model);
	shader->setInt("uHasTexture", texture != 0 ? 1 : 0);

	// Enable stencil testing for masking
	if (clip_mask_enabled) {
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_EQUAL, 1, 0xFF);
		glStencilMask(0x00);
	} else {
		glDisable(GL_STENCIL_TEST);
	}
	glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);
	shader->use();
	if (texture != 0) {
		auto tex = texture_map.find(texture);
		if (tex != texture_map.end()) {
			tex->second.Bind(0);
		} else {
			log::system_error("UI", "Invalid texture handle {}", texture);
			return;
		}
		Geometry& geo = it->second;
		glBindVertexArray(geo.vao);
		DrawElementsWrapper(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
		tex->second.Unbind(0);
	} else {
		Geometry& geo = it->second;
		glBindVertexArray(geo.vao);
		DrawElementsWrapper(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
	}
	glBindVertexArray(0);
	if (BLENDING)
		glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	if (DEPTH_TEST)
		glEnable(GL_DEPTH_TEST); glDepthFunc(DEPTH_FUNC);
	
	glDisable(GL_STENCIL_TEST);
}

void UI::ReleaseGeometry(Rml::CompiledGeometryHandle handle)
{
	geometry_map.erase(handle);
}

Rml::TextureHandle UI::LoadTexture(Rml::Vector2i& out_dimensions, const Rml::String& source)
{
    std::filesystem::path source_path(source); // absolute path
    std::filesystem::path real_source;

	/*
	// TODO: Fix this
	// currently you can't load assets from UI/ because the path just gets deleted
    bool skip_next = false;
    for (auto& part : source_path)
    {
        if (skip_next) { skip_next = false; continue; }

        std::string str = part.string();
        if (str == "UI")
        {
            // skip this folder
            continue;
        }
        real_source /= part;
    }
	*/
	log::system_info("UI", "Path: {}", source_path.string());
	real_source = ASSETS_DIRECTORY / source_path.filename();

	Texture tex(real_source, GL_RGBA, GL_REPEAT, GL_LINEAR);
	out_dimensions.x = tex.getWidth();
	out_dimensions.y = tex.getHeight();

	Rml::TextureHandle handle = next_texture_handle++;
	texture_map.emplace(handle, std::move(tex));
	log::system_info("UI", "Loaded texture handle {} from {}", handle, real_source.string());
	return handle;
}

Rml::TextureHandle UI::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	// Create an empty texture using your Texture class
	Texture tex;
	tex.createEmpty(source_dimensions.x, source_dimensions.y, GL_RGBA8);

	// Upload the pixel data (source.data())
	glTextureSubImage2D(tex.getID(), 0, 0, 0, source_dimensions.x, source_dimensions.y, GL_RGBA, GL_UNSIGNED_BYTE, source.data());

	// Set filtering parameters
	glTextureParameteri(tex.getID(), GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTextureParameteri(tex.getID(), GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	Rml::TextureHandle handle = next_texture_handle++;
	texture_map.emplace(handle, std::move(tex)); // Store the full Texture object
	log::system_info("UI", "Generated texture handle {}", handle);
	return handle;
}

void UI::ReleaseTexture(Rml::TextureHandle handle)
{
	auto it = texture_map.find(handle);
	if (it != texture_map.end()) {
		log::system_info("UI", "Releasing texture handle {}", handle);
		texture_map.erase(it);
	} else {
		log::system_error("UI", "Warning: Attempt to release invalid texture handle {}", handle);
	}
}
void UI::SetScissorRegion(Rml::Rectanglei region)
{
	if (region.Valid())
		glEnable(GL_SCISSOR_TEST);
	else
		glDisable(GL_SCISSOR_TEST);

	if (region.Valid()) {
		// Some render APIs don't like offscreen positions (WebGL in particular), so clamp them to the viewport.
		const int x = Rml::Math::Clamp(region.Left(), 0, viewport_width);
		const int y = Rml::Math::Clamp(viewport_height - region.Bottom(), 0, viewport_height);

		glScissor(x, y, region.Width(), region.Height());
	}
}

void UI::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation)
{
	auto it = geometry_map.find(geometry);
	if (it == geometry_map.end())
		return;

	Geometry& geo = it->second;

	// Setup transform
	glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));
	glm::mat4 final_model        = model * translation_matrix;

	shader->use();
	shader->setMat4("uModel", final_model);
	shader->setMat4("uProjection", projection);
	shader->setInt("uHasTexture", 0); // Mask geometry doesn't use textures

	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
    glDepthMask(GL_FALSE);
    glEnable(GL_STENCIL_TEST);

    uint8_t ref_value = next_stencil_value;

    switch (operation) {
        case Rml::ClipMaskOperation::Set:
            if (true) {  // always clear + write new value for Set
                glClearStencil(ref_value);
                glClear(GL_STENCIL_BUFFER_BIT);
            }
            glStencilFunc(GL_ALWAYS, ref_value, 0xFF);
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
            glStencilMask(0xFF);
            next_stencil_value = (next_stencil_value + 1) & 0x7F;
            break;

        case Rml::ClipMaskOperation::SetInverse:
            if (true) {
                glClearStencil(0);               // or ref_value if you want inverse logic
            }
            glStencilFunc(GL_ALWAYS, 0, 0xFF);
            glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
            glStencilMask(0xFF);
            next_stencil_value = (next_stencil_value + 1) & 0x7F;
            break;

        case Rml::ClipMaskOperation::Intersect:
            // Test against previous (most recently written) value
            glStencilFunc(GL_EQUAL, next_stencil_value - 1, 0xFF);
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
            glStencilMask(0x00);
            break;

        default:
            // fallback / log unknown operation
            break;
    }

    glBindVertexArray(geo.vao);
    DrawElementsWrapper(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);

    // Restore
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glDepthMask(GL_TRUE);
    glStencilMask(0x00);
}

void UI::SetTransform(const Rml::Matrix4f* transform)
{
	// Store or reset the current transform matrix for use during RenderGeometry.
	if (transform) {
		// Convert Rml::Matrix4f to glm::mat4 (both column-major)
		std::memcpy(&model[0][0], transform->data(), 16 * sizeof(float));
		shader->setMat4("uModel", model);
	} else {
		model = glm::mat4(1.0f); // Identity matrix
		shader->setMat4("uModel", model);
	}
}

Rml::LayerHandle UI::PushLayer()
{
	Layer layer;
	layer.width  = viewport_width;
	layer.height = viewport_height;

	glGenFramebuffers(1, &layer.fbo);
	glGenTextures(1, &layer.color_texture);

	glBindTexture(GL_TEXTURE_2D, layer.color_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, layer.width, layer.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, layer.color_texture, 0);

	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		log::system_error("UI", "Framebuffer incomplete when pushing layer");
		glDeleteFramebuffers(1, &layer.fbo);
		glDeleteTextures(1, &layer.color_texture);
		return 0;
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	// Store current FBO to return to it later
	GLint current_fbo;
	glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &current_fbo);
	fbo_stack.emplace_back((GLuint)current_fbo);

	glBindFramebuffer(GL_FRAMEBUFFER, layer.fbo);
	glClearColor(0,0,0,0);
	glClear(GL_COLOR_BUFFER_BIT);

	Rml::LayerHandle handle = next_layer_handle++;
	layers[handle] = std::move(layer);
	return handle;
}

void UI::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters)
{
	auto src_it = layers.find(source);
    if (src_it == layers.end()) return;

    Layer& src = src_it->second;

    // Bind destination (0 is backbuffer)
    glBindFramebuffer(GL_FRAMEBUFFER, destination == 0 ? 0 : layers[destination].fbo);

    glEnable(GL_BLEND);
    if (blend_mode == Rml::BlendMode::Replace) {
        glBlendFunc(GL_ONE, GL_ZERO);
    } else {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    shader->use();
    shader->setMat4("uModel", glm::mat4(1.0f));
    shader->setMat4("uProjection", glm::mat4(1.0f)); // Identity for NDC quad
    shader->setInt("uHasTexture", 1);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, src.color_texture);
    
    glBindVertexArray(quad_vao);
    DrawArraysWrapper(GL_TRIANGLES, 0, 6);

    // Cleanup: Delete the layer resources after compositing
    glDeleteFramebuffers(1, &src.fbo);
    glDeleteTextures(1, &src.color_texture);
    layers.erase(src_it);
}
void UI::PopLayer()
{
	if (!fbo_stack.empty()) {
		glBindFramebuffer(GL_FRAMEBUFFER, fbo_stack.back());
		fbo_stack.pop_back();
	}
}

Rml::TextureHandle UI::SaveLayerAsTexture()
{
	// No layer texture support, return 0 (invalid texture handle)
	return 0;
}

Rml::CompiledFilterHandle UI::SaveLayerAsMaskImage()
{
	// No filter support, return 0 (invalid filter handle)
	return 0;
}

Rml::CompiledFilterHandle UI::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	// No filters supported, return 0
	(void)parameters;
	// For now we don't support custom filters
    log::system_info("UI", "CompileFilter called for '{}', not supported", name);
    return 0;
}

void UI::ReleaseFilter(Rml::CompiledFilterHandle filter)
{
	// Nothing to release since we don't compile filters
	(void)filter;
}

Rml::CompiledShaderHandle UI::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters)
{
	// No shaders supported beyond the default shader
	(void)parameters;
	log::system_info("UI", "CompileShader called for '{}', not supported", name);
    return 0;
}

void UI::RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{

	// No custom shader support, fallback to RenderGeometry or do nothing
	(void)shader;
	//(void)geometry;
	//(void)translation;
	//(void)texture;
	// Fallback to default rendering
    RenderGeometry(geometry, translation, texture);
}

void UI::ReleaseShader(Rml::CompiledShaderHandle shader)
{
	// Nothing to release since we don't compile shaders
	(void)shader;
}



Rml::Input::KeyIdentifier UI::MapKey(int glfw_key) noexcept
{
    switch (glfw_key)
    {
        // Letters (A–Z)
        case GLFW_KEY_A: return Rml::Input::KI_A;
        case GLFW_KEY_B: return Rml::Input::KI_B;
        case GLFW_KEY_C: return Rml::Input::KI_C;
        case GLFW_KEY_D: return Rml::Input::KI_D;
        case GLFW_KEY_E: return Rml::Input::KI_E;
        case GLFW_KEY_F: return Rml::Input::KI_F;
        case GLFW_KEY_G: return Rml::Input::KI_G;
        case GLFW_KEY_H: return Rml::Input::KI_H;
        case GLFW_KEY_I: return Rml::Input::KI_I;
        case GLFW_KEY_J: return Rml::Input::KI_J;
        case GLFW_KEY_K: return Rml::Input::KI_K;
        case GLFW_KEY_L: return Rml::Input::KI_L;
        case GLFW_KEY_M: return Rml::Input::KI_M;
        case GLFW_KEY_N: return Rml::Input::KI_N;
        case GLFW_KEY_O: return Rml::Input::KI_O;
        case GLFW_KEY_P: return Rml::Input::KI_P;
        case GLFW_KEY_Q: return Rml::Input::KI_Q;
        case GLFW_KEY_R: return Rml::Input::KI_R;
        case GLFW_KEY_S: return Rml::Input::KI_S;
        case GLFW_KEY_T: return Rml::Input::KI_T;
        case GLFW_KEY_U: return Rml::Input::KI_U;
        case GLFW_KEY_V: return Rml::Input::KI_V;
        case GLFW_KEY_W: return Rml::Input::KI_W;
        case GLFW_KEY_X: return Rml::Input::KI_X;
        case GLFW_KEY_Y: return Rml::Input::KI_Y;
        case GLFW_KEY_Z: return Rml::Input::KI_Z;

        // Numbers (top row)
        case GLFW_KEY_0: return Rml::Input::KI_0;
        case GLFW_KEY_1: return Rml::Input::KI_1;
        case GLFW_KEY_2: return Rml::Input::KI_2;
        case GLFW_KEY_3: return Rml::Input::KI_3;
        case GLFW_KEY_4: return Rml::Input::KI_4;
        case GLFW_KEY_5: return Rml::Input::KI_5;
        case GLFW_KEY_6: return Rml::Input::KI_6;
        case GLFW_KEY_7: return Rml::Input::KI_7;
        case GLFW_KEY_8: return Rml::Input::KI_8;
        case GLFW_KEY_9: return Rml::Input::KI_9;

        // Numpad numbers
        case GLFW_KEY_KP_0: return Rml::Input::KI_NUMPAD0;
        case GLFW_KEY_KP_1: return Rml::Input::KI_NUMPAD1;
        case GLFW_KEY_KP_2: return Rml::Input::KI_NUMPAD2;
        case GLFW_KEY_KP_3: return Rml::Input::KI_NUMPAD3;
        case GLFW_KEY_KP_4: return Rml::Input::KI_NUMPAD4;
        case GLFW_KEY_KP_5: return Rml::Input::KI_NUMPAD5;
        case GLFW_KEY_KP_6: return Rml::Input::KI_NUMPAD6;
        case GLFW_KEY_KP_7: return Rml::Input::KI_NUMPAD7;
        case GLFW_KEY_KP_8: return Rml::Input::KI_NUMPAD8;
        case GLFW_KEY_KP_9: return Rml::Input::KI_NUMPAD9;

        // Numpad operators
        case GLFW_KEY_KP_ADD:      return Rml::Input::KI_ADD;
        case GLFW_KEY_KP_SUBTRACT: return Rml::Input::KI_SUBTRACT;
        case GLFW_KEY_KP_MULTIPLY: return Rml::Input::KI_MULTIPLY;
        case GLFW_KEY_KP_DIVIDE:   return Rml::Input::KI_DIVIDE;
        case GLFW_KEY_KP_DECIMAL:  return Rml::Input::KI_DECIMAL;
        case GLFW_KEY_KP_ENTER:    return Rml::Input::KI_NUMPADENTER;

        // Function keys
        case GLFW_KEY_F1:  return Rml::Input::KI_F1;
        case GLFW_KEY_F2:  return Rml::Input::KI_F2;
        case GLFW_KEY_F3:  return Rml::Input::KI_F3;
        case GLFW_KEY_F4:  return Rml::Input::KI_F4;
        case GLFW_KEY_F5:  return Rml::Input::KI_F5;
        case GLFW_KEY_F6:  return Rml::Input::KI_F6;
        case GLFW_KEY_F7:  return Rml::Input::KI_F7;
        case GLFW_KEY_F8:  return Rml::Input::KI_F8;
        case GLFW_KEY_F9:  return Rml::Input::KI_F9;
        case GLFW_KEY_F10: return Rml::Input::KI_F10;
        case GLFW_KEY_F11: return Rml::Input::KI_F11;
        case GLFW_KEY_F12: return Rml::Input::KI_F12;

        // Navigation & editing
        case GLFW_KEY_ENTER:     return Rml::Input::KI_RETURN;
        case GLFW_KEY_ESCAPE:    return Rml::Input::KI_ESCAPE;
        case GLFW_KEY_TAB:       return Rml::Input::KI_TAB;
        case GLFW_KEY_BACKSPACE: return Rml::Input::KI_BACK;
        case GLFW_KEY_DELETE:    return Rml::Input::KI_DELETE;
        case GLFW_KEY_INSERT:    return Rml::Input::KI_INSERT;
        case GLFW_KEY_HOME:      return Rml::Input::KI_HOME;
        case GLFW_KEY_END:       return Rml::Input::KI_END;
        case GLFW_KEY_PAGE_UP:   return Rml::Input::KI_PRIOR;
        case GLFW_KEY_PAGE_DOWN: return Rml::Input::KI_NEXT;

        // Arrow keys
        case GLFW_KEY_LEFT:  return Rml::Input::KI_LEFT;
        case GLFW_KEY_RIGHT: return Rml::Input::KI_RIGHT;
        case GLFW_KEY_UP:    return Rml::Input::KI_UP;
        case GLFW_KEY_DOWN:  return Rml::Input::KI_DOWN;

        // Modifiers (usually handled via modifiers bitfield, but can be mapped)
        case GLFW_KEY_LEFT_SHIFT:    return Rml::Input::KI_LSHIFT;
        case GLFW_KEY_RIGHT_SHIFT:   return Rml::Input::KI_RSHIFT;
        case GLFW_KEY_LEFT_CONTROL:  return Rml::Input::KI_LCONTROL;
        case GLFW_KEY_RIGHT_CONTROL: return Rml::Input::KI_RCONTROL;
        case GLFW_KEY_LEFT_ALT:      return Rml::Input::KI_LMENU;
        case GLFW_KEY_RIGHT_ALT:     return Rml::Input::KI_RMENU;
        case GLFW_KEY_LEFT_SUPER:    return Rml::Input::KI_LMETA;
        case GLFW_KEY_RIGHT_SUPER:   return Rml::Input::KI_RMETA;

        // Punctuation & symbols (most common ones)
        case GLFW_KEY_SPACE:      return Rml::Input::KI_SPACE;
        case GLFW_KEY_APOSTROPHE: return Rml::Input::KI_OEM_7;     // '
        case GLFW_KEY_COMMA:      return Rml::Input::KI_OEM_COMMA;  // ,
        case GLFW_KEY_MINUS:      return Rml::Input::KI_OEM_MINUS;  // -
        case GLFW_KEY_PERIOD:     return Rml::Input::KI_OEM_PERIOD; // .
        case GLFW_KEY_SLASH:      return Rml::Input::KI_OEM_2;      // /
        case GLFW_KEY_SEMICOLON:  return Rml::Input::KI_OEM_1;      // ;
        case GLFW_KEY_EQUAL:      return Rml::Input::KI_OEM_PLUS;   // =
        case GLFW_KEY_LEFT_BRACKET:  return Rml::Input::KI_OEM_4;   // [
        case GLFW_KEY_BACKSLASH:     return Rml::Input::KI_OEM_5;   // \
        case GLFW_KEY_RIGHT_BRACKET: return Rml::Input::KI_OEM_6;   // ]
        case GLFW_KEY_GRAVE_ACCENT:  return Rml::Input::KI_OEM_3;   // `

        // Less common but sometimes useful
        case GLFW_KEY_PAUSE:      return Rml::Input::KI_PAUSE;
        case GLFW_KEY_SCROLL_LOCK:return Rml::Input::KI_SCROLL;
        case GLFW_KEY_PRINT_SCREEN: return Rml::Input::KI_SNAPSHOT;
        case GLFW_KEY_CAPS_LOCK:  return Rml::Input::KI_CAPITAL;

        default:
            return Rml::Input::KI_UNKNOWN;
    }
}
