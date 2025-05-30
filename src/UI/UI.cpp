#include "UI.h"
#include "glm/gtc/type_ptr.hpp"
#include <cstring>
#include <iostream>
//#define STB_IMAGE_IMPLEMENTATION
//#include "stb_image.h"
#include <cstring>
#include <GLFW/glfw3.h>
#include <exception>

UI::UI(int width, int height, Shader* ui_shader, std::filesystem::path fontPath, std::filesystem::path docPath) : viewport_width(width), viewport_height(height) {
    Rml::SetRenderInterface(this);
    Rml::SetSystemInterface(new SystemInterface());
    Rml::SetFileInterface(new FileInterface());
    shader = ui_shader;
    Rml::Initialise();

    context = Rml::CreateContext("main", {width, height});
#ifdef DEBUG
    Rml::Debugger::Initialise(context);
#endif
    Rml::LoadFontFace(fontPath.string());

    doc = context->LoadDocument(docPath.string());
    if (!doc) {
	std::cerr << "Failed to load RML document.\n";
    } else {
	doc->Show();
    }

}

UI::~UI() {
    for (auto& [handle, geo] : geometry_map) {
        glDeleteVertexArrays(1, &geo.vao);
        glDeleteBuffers(1, &geo.vbo);
        glDeleteBuffers(1, &geo.ebo);
    }
    for (auto& [handle, tex] : texture_map) {
        glDeleteTextures(1, &tex);
    }
    Rml::Shutdown();
    delete shader;
    delete context;
    delete doc;
}

Rml::CompiledGeometryHandle UI::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices) {
    GLuint vao, vbo, ebo;
    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glCreateBuffers(1, &ebo);

    glNamedBufferData(vbo, vertices.size() * sizeof(Rml::Vertex), vertices.data(), GL_STATIC_DRAW);
    glNamedBufferData(ebo, indices.size() * sizeof(int), indices.data(), GL_STATIC_DRAW);


    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(Rml::Vertex));
    glEnableVertexArrayAttrib(vao, 0); // pos
    glEnableVertexArrayAttrib(vao, 1); // color
    glEnableVertexArrayAttrib(vao, 2); // texcoord

    glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, offsetof(Rml::Vertex, position));
    glVertexArrayAttribFormat(vao, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(Rml::Vertex, colour));
    glVertexArrayAttribFormat(vao, 2, 2, GL_FLOAT, GL_FALSE, offsetof(Rml::Vertex, tex_coord));

    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);
    glVertexArrayAttribBinding(vao, 2, 0);
    glVertexArrayElementBuffer(vao, ebo);

    Geometry geo{ vao, vbo, ebo, static_cast<GLsizei>(indices.size()) };
    Rml::CompiledGeometryHandle handle = next_geometry_handle++;
    geometry_map[handle] = geo;
    return handle;
}

void UI::RenderGeometry(Rml::CompiledGeometryHandle handle, Rml::Vector2f translation, Rml::TextureHandle texture) {
    auto it = geometry_map.find(handle);
    if (it == geometry_map.end())
        return;

    // Setup shader uniforms, texture bind, etc. -- omitted for brevity
    projection = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
    // Combine the model matrix from SetTransform with the translation
    glm::mat4 translation_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(translation.x, translation.y, 0.0f));
    glm::mat4 final_model = model * translation_matrix; // Apply SetTransform's model first, then translation

    shader->use();
    shader->setMat4("uProjection", projection);
    shader->setMat4("uModel", final_model);

    bool hasTexture = texture != 0;
    shader->setInt("uHasTexture", hasTexture ? 1 : 0);

    if (texture != 0) {
	glBindTexture(GL_TEXTURE_2D, texture_map[texture]);
	shader->setInt("uTexture", 0);
    } else {
	glBindTexture(GL_TEXTURE_2D, 0);
    }

    Geometry& geo = it->second;
    glBindVertexArray(geo.vao);
    glDrawElements(GL_TRIANGLES, geo.index_count, GL_UNSIGNED_INT, nullptr);
}

void UI::ReleaseGeometry(Rml::CompiledGeometryHandle handle) {
    auto it = geometry_map.find(handle);
    if (it != geometry_map.end()) {
        Geometry& geo = it->second;
        glDeleteVertexArrays(1, &geo.vao);
        glDeleteBuffers(1, &geo.vbo);
        glDeleteBuffers(1, &geo.ebo);
        geometry_map.erase(it);
    }
}

Rml::TextureHandle UI::LoadTexture(Rml::Vector2i& out_dimensions, const Rml::String& source) {

    Texture tex(source, GL_RGBA, GL_REPEAT, GL_LINEAR);
    out_dimensions.x = tex.getWidth();
    out_dimensions.y = tex.getHeight();

    Rml::TextureHandle handle = next_texture_handle++;
    texture_map[handle] = tex.getID();
    return handle;
}

Rml::TextureHandle UI::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions) {
    GLuint tex;
    glCreateTextures(GL_TEXTURE_2D, 1, &tex);
    glTextureStorage2D(tex, 1, GL_RGBA8, source_dimensions.x, source_dimensions.y);
    glTextureSubImage2D(tex, 0, 0, 0, source_dimensions.x, source_dimensions.y, GL_RGBA, GL_UNSIGNED_BYTE, source.data());

    glTextureParameteri(tex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    Rml::TextureHandle handle = next_texture_handle++;
    texture_map[handle] = tex;
    return handle;
}

void UI::ReleaseTexture(Rml::TextureHandle handle) {
    auto it = texture_map.find(handle);
    if (it != texture_map.end()) {
        glDeleteTextures(1, &it->second);
        texture_map.erase(it);
    }
}

void UI::EnableScissorRegion(bool enable) {
    if (enable)
        glEnable(GL_SCISSOR_TEST);
    else
        glDisable(GL_SCISSOR_TEST);
}

void UI::SetScissorRegion(Rml::Rectanglei region) {
    glScissor(region.Left(), viewport_height - region.Top() - region.Height(), region.Width(), region.Height());
}

void UI::SetViewportSize(int width, int height) {
    viewport_width = width;
    viewport_height = height;
    projection = glm::ortho(0.0f, (float)viewport_width, (float)viewport_height, 0.0f, -1.0f, 1.0f);
    glViewport(0, 0, width, height);
    shader->use();
    shader->setMat4("uProjection", projection);
    context->SetDimensions({width, height});
    context->Update();
}
// Optional functions default implementations

void UI::EnableClipMask(bool enable) {
    // Default: no clip mask support, just disable OpenGL scissor test for clip mask.
    if (enable) {
        // You could enable stencil buffer here if you want real clip masks
        // but for now, just disable to be safe.
        glDisable(GL_SCISSOR_TEST);
    }
}

void UI::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation) {
    // No clip mask support implemented
    (void)operation; (void)geometry; (void)translation;
    // Do nothing
}

void UI::SetTransform(const Rml::Matrix4f* transform) {
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

Rml::LayerHandle UI::PushLayer() {
    // We don't support layers, so return 0 for base layer always
    return 0;
}

void UI::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters) {
    // No layers or filters support - just ignore
    (void)source; (void)destination; (void)blend_mode; (void)filters;
}

void UI::PopLayer() {
    // No layers supported, do nothing
}

Rml::TextureHandle UI::SaveLayerAsTexture() {
    // No layer texture support, return 0 (invalid texture handle)
    return 0;
}

Rml::CompiledFilterHandle UI::SaveLayerAsMaskImage() {
    // No filter support, return 0 (invalid filter handle)
    return 0;
}

Rml::CompiledFilterHandle UI::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters) {
    // No filters supported, return 0
    (void)name; (void)parameters;
    return 0;
}

void UI::ReleaseFilter(Rml::CompiledFilterHandle filter) {
    // Nothing to release since we don't compile filters
    (void)filter;
}

Rml::CompiledShaderHandle UI::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters) {
    // No shaders supported beyond the default shader
    (void)name; (void)parameters;
    return 0;
}

void UI::RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture) {
    // No custom shader support, fallback to RenderGeometry or do nothing
    (void)shader; (void)geometry; (void)translation; (void)texture;
}

void UI::ReleaseShader(Rml::CompiledShaderHandle shader) {
    // Nothing to release since we don't compile shaders
    (void)shader;
}

void UI::render(void) {
    context->Update();
    context->Render();
}

int UI::GetKeyModifiers(void) {
    int modifiers = 0;
    if (isShiftDown) modifiers |= Rml::Input::KM_SHIFT;
    if (isCtrlDown) modifiers |= Rml::Input::KM_CTRL;
    if (isAltDown) modifiers |= Rml::Input::KM_ALT;
    if (isMetaDown) modifiers |= Rml::Input::KM_META;
    return modifiers;
}

Rml::Input::KeyIdentifier UI::MapKey(int glfw_key) {

    switch (glfw_key) {
	case GLFW_KEY_A: return Rml::Input::KI_A;
	case GLFW_KEY_B: return Rml::Input::KI_B;
	case GLFW_KEY_ENTER: return Rml::Input::KI_RETURN;
	case GLFW_KEY_ESCAPE: return Rml::Input::KI_ESCAPE;
	case GLFW_KEY_LEFT: return Rml::Input::KI_LEFT;
	case GLFW_KEY_RIGHT: return Rml::Input::KI_RIGHT;
	case GLFW_KEY_UP:    return Rml::Input::KI_UP;
	case GLFW_KEY_DOWN:  return Rml::Input::KI_DOWN;
			     // ... Map more keys ...
	default: return Rml::Input::KI_UNKNOWN;
    }

}
